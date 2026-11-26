#!/usr/bin/env python3
"""Install a Qt release from the official online repository.

aqt/install-qt-action cannot resolve the 6.11+ repository layout, so this
script reads the official Updates.xml, downloads the requested package's
archives, verifies each archive's .sha1, and extracts it with 7-Zip.

It prints the Qt prefix on stdout. Progress and diagnostics go to stderr so
the workflow can capture the prefix without parsing log lines.
"""

from __future__ import annotations

import argparse
import hashlib
import os
import shutil
import subprocess
import sys
import time
import urllib.request
import xml.etree.ElementTree as ET
from pathlib import Path

REPOSITORY_ROOT = "https://download.qt.io/online/qtsdkrepository"


def log(message: str) -> None:
    print(message, file=sys.stderr, flush=True)


def fetch_bytes(url: str, attempts: int = 3) -> bytes:
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "tmog-ci/1.0"})
            with urllib.request.urlopen(request, timeout=120) as response:
                return response.read()
        except Exception as error:  # noqa: BLE001 - retry any transport failure
            last_error = error
            if attempt < attempts:
                log(f"retry {attempt}/{attempts} for {url}: {error}")
                time.sleep(2 * attempt)
    raise RuntimeError(f"could not download {url}: {last_error}")


def download_file(url: str, destination: Path, attempts: int = 3) -> None:
    last_error: Exception | None = None
    for attempt in range(1, attempts + 1):
        try:
            request = urllib.request.Request(url, headers={"User-Agent": "tmog-ci/1.0"})
            with urllib.request.urlopen(request, timeout=120) as response:
                with destination.open("wb") as output:
                    shutil.copyfileobj(response, output, 1024 * 1024)
            return
        except Exception as error:  # noqa: BLE001 - retry any transport failure
            last_error = error
            destination.unlink(missing_ok=True)
            if attempt < attempts:
                log(f"retry {attempt}/{attempts} for {url}: {error}")
                time.sleep(2 * attempt)
    raise RuntimeError(f"could not download {url}: {last_error}")


def sha1(path: Path) -> str:
    digest = hashlib.sha1()
    with path.open("rb") as file:
        for chunk in iter(lambda: file.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def find_7zip() -> str:
    for name in ("7z", "7zz", "7za"):
        found = shutil.which(name)
        if found:
            return found

    if os.name == "nt":
        for candidate in (
            Path(r"C:\Program Files\7-Zip\7z.exe"),
            Path(r"C:\Program Files (x86)\7-Zip\7z.exe"),
        ):
            if candidate.is_file():
                return str(candidate)

    raise RuntimeError(
        "7-Zip was not found. Install p7zip/sevenzip (7z or 7zz) or add 7-Zip to PATH."
    )


def verify_prefix(prefix: Path) -> bool:
    return (prefix / "lib" / "cmake" / "Qt6" / "Qt6Config.cmake").is_file() and (
        prefix / "lib" / "cmake" / "Qt6Widgets" / "Qt6WidgetsConfig.cmake"
    ).is_file()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", default="6.12.1")
    parser.add_argument("--repo", required=True, help="Repository path below qtsdkrepository")
    parser.add_argument("--package", required=True, help="Package name in Updates.xml")
    parser.add_argument("--cache-dir", required=True, type=Path)
    parser.add_argument("--dry-run", action="store_true", help="Print archives and prefix only")
    args = parser.parse_args()

    cache = args.cache_dir.expanduser().resolve()
    cache.mkdir(parents=True, exist_ok=True)
    marker = cache / ".complete"
    prefix_file = cache / "prefix.txt"
    root = cache / "root"

    if marker.is_file() and prefix_file.is_file():
        cached_prefix = Path(prefix_file.read_text(encoding="utf-8").strip())
        if verify_prefix(cached_prefix):
            log(f"Using cached Qt {args.version} at {cached_prefix.as_posix()}")
            print(cached_prefix.as_posix())
            return 0
        log("Cached Qt prefix is incomplete; reinstalling.")

    base_url = f"{REPOSITORY_ROOT}/{args.repo}"
    archive_base = f"{base_url}/{args.package}"
    log(f"Reading {base_url}/Updates.xml")
    updates = ET.fromstring(fetch_bytes(f"{base_url}/Updates.xml"))

    package = None
    for candidate in updates.iter("PackageUpdate"):
        if (candidate.findtext("Name") or "").strip() == args.package:
            package = candidate
            break
    if package is None:
        raise RuntimeError(f"package {args.package} was not found in Updates.xml")

    package_version = (package.findtext("Version") or "").strip()
    archives = [
        archive.strip()
        for archive in (package.findtext("DownloadableArchives") or "").split(",")
        if archive.strip()
    ]
    selected = [
        archive
        for archive in archives
        if "debug" not in archive.lower()
        and (
            "qtbase" in archive.lower()
            or "qtsvg" in archive.lower()
            or archive.lower().startswith("icu-")
        )
    ]
    if not any("qtbase" in archive.lower() for archive in selected):
        raise RuntimeError("Updates.xml did not list a qtbase archive")

    targets: dict[str, str] = {}
    for operation in package.findall("Operations/Operation"):
        if operation.get("name") != "Extract":
            continue
        arguments = [argument.text or "" for argument in operation.findall("Argument")]
        if len(arguments) < 2:
            continue
        archive_name = arguments[1].strip()
        if archive_name in selected:
            targets[archive_name] = arguments[0].replace("@TargetDir@/", "", 1)

    qtbase_archive = next(
        (archive for archive in selected if "qtbase" in archive.lower()), None
    )
    target_relative = targets.get(qtbase_archive or "")
    if not target_relative:
        raise RuntimeError("Updates.xml did not describe the qtbase extraction target")

    if args.dry_run:
        for archive in selected:
            archive_file = archive if archive.startswith(package_version) else f"{package_version}{archive}"
            print(f"{archive_base}/{archive_file}")
        print(f"target={target_relative}")
        return 0

    downloads = cache / "downloads"
    downloads.mkdir(exist_ok=True)
    seven_zip = find_7zip()

    for archive in selected:
        # The repository file name is prefixed with the package version
        # (for example 6.12.1-0-202611180427qtbase-...7z), while
        # DownloadableArchives lists the suffix only.
        archive_file = archive if archive.startswith(package_version) else f"{package_version}{archive}"
        destination = downloads / archive_file
        checksum_url = f"{archive_base}/{archive_file}.sha1"
        expected = fetch_bytes(checksum_url).decode("ascii").split()[0].strip().lower()
        if destination.is_file() and sha1(destination) == expected:
            log(f"Using cached archive {archive_file}")
            continue
        log(f"Downloading {archive_file}")
        download_file(f"{archive_base}/{archive_file}", destination)
        actual = sha1(destination)
        if actual != expected:
            destination.unlink(missing_ok=True)
            raise RuntimeError(
                f"checksum mismatch for {archive_file}: expected {expected}, got {actual}"
            )

    if root.exists():
        shutil.rmtree(root)
    root.mkdir(parents=True)

    for archive in selected:
        archive_file = archive if archive.startswith(package_version) else f"{package_version}{archive}"
        target = targets.get(archive)
        if not target:
            raise RuntimeError(f"Updates.xml did not describe an extraction target for {archive}")
        destination = root / target
        destination.mkdir(parents=True, exist_ok=True)
        log(f"Extracting {archive_file} into {target}")
        subprocess.run(
            [seven_zip, "x", "-y", f"-o{destination}", str(downloads / archive_file)],
            check=True,
            stdout=sys.stderr,
        )

    prefix = root / target_relative
    if not verify_prefix(prefix):
        raise RuntimeError(
            f"Qt prefix {prefix} is missing Qt6Config.cmake or Qt6WidgetsConfig.cmake"
        )

    prefix_file.write_text(prefix.as_posix(), encoding="utf-8")
    marker.write_text(args.version, encoding="utf-8")
    log(f"Installed Qt {args.version} at {prefix.as_posix()}")
    print(prefix.as_posix())
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:  # noqa: BLE001 - CI should show the failure clearly
        log(f"error: {error}")
        raise SystemExit(1)
