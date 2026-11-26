# Releasing Task Manager OG by Don PELY

This is the operational runbook for the tag-driven release pipeline. The product name is
always **Task Manager OG by Don PELY**. The release version comes from `CMakeLists.txt`
and must match the pushed tag without its `v` prefix.

## Current release

- Tag: [`v0.1.0`](https://github.com/yourbossdonpely/tmog-taskmanager/releases/tag/v0.1.0)
- Channel: prerelease (`v0.x` is pre-1.0)
- Workflow run: `45803444183` — Windows x64, macOS universal, Linux x64 and publish all green
- Release assets: **15**

## Asset contract

The publish job refuses to publish unless all fifteen files exist and the checksums verify:

1. `TaskManagerOG-<version>-windows-x64-setup.exe` — Inno Setup installer
2. `TaskManagerOG-<version>-windows-x64.msi` — WiX MSI
3. `TaskManagerOG-<version>-windows-x64-portable.zip`
4. `TaskManagerOG-<version>-macos-universal.dmg`
5. `TaskManagerOG-<version>-macos-universal-portable.zip`
6. `TaskManagerOG-<version>-linux-x86_64.AppImage`
7. `TaskManagerOG-<version>-linux-amd64.deb`
8. `TaskManagerOG-<version>-linux-x86_64.rpm`
9. `TaskManagerOG-<version>-linux-x86_64.tar.gz`
10. `TaskManagerOG-<version>-windows-selfsigned.cer`
11. `TaskManagerOG-<version>-linux-signing-key.asc`
12. `SHA256SUMS.txt`
13. `SHA256SUMS.txt.asc`
14. `latest.json`
15. `latest.json.asc`

The pipeline builds Windows x64 and Linux x64 only. It does not build Windows ARM64 or
Linux ARM64 yet. The macOS artifact is universal (`x86_64 arm64`).

## How publishing works

`.github/workflows/release.yml` is triggered by a tag matching `v*`:

```bash
git tag -a v0.1.0 -m "Task Manager OG by Don PELY v0.1.0 prerelease"
git push origin v0.1.0
```

It runs four jobs:

1. **Windows x64** — builds/tests, deploys Qt, builds setup EXE, MSI and portable ZIP,
   signs the application/installers with a generated self-signed certificate.
2. **macOS universal** — cross-builds `x86_64;arm64`, verifies both architectures,
   ad-hoc signs the app and DMG, creates the DMG and portable ZIP.
3. **Linux x64** — builds/tests, bundles Qt with linuxdeploy, creates AppImage, deb,
   RPM and portable tar archive.
4. **Sign and publish** — downloads all build artifacts, creates checksums and
   `latest.json`, signs those with a generated GPG key, verifies exactly fifteen files,
   creates the GitHub Release and uploads all assets. Only after the count and signatures
   pass does it remove the draft flag and mark the release prerelease.

The publish job is intentionally blocked when the event is a manual build. To test the
three packaging jobs without creating a release:

```bash
gh workflow run release.yml -f version=0.1.0
```

The `publish` job has `if: github.event_name == 'push'`, so this is build-only. Download
the resulting artifacts from the workflow run and inspect them before tagging.

## Local packaging

All three packaging scripts require a Release build of `TaskManagerOG` and the Qt
`QT_ROOT_DIR`. They are platform-native; do not expect to produce macOS or Linux
packages on Windows.

### Windows

Prerequisites: Inno Setup 6, WiX Toolset 3.14 and the Windows SDK `signtool.exe`.

```powershell
$env:TMOG_VERSION = "0.1.0"
$env:QT_ROOT_DIR = "C:/Qt/6.12.1/msvc2022_64"
cmake -S . -B build -DCMAKE_PREFIX_PATH="$env:QT_ROOT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
pwsh -NoProfile -File packaging/windows/build-windows.ps1
```

Output goes to `dist/windows/`. The script creates `installer` and `portable` staging
directories, signs `TaskManagerOG.exe`, builds the setup EXE and MSI, signs both, and
removes WiX `.wixpdb` debug symbols so they cannot enter the release.

### macOS

Prerequisites: Xcode command-line tools, Qt 6.12.1 macOS kit, `macdeployqt`, `hdiutil`,
and `codesign`.

```bash
export TMOG_VERSION=0.1.0
export QT_ROOT_DIR=/path/to/Qt/6.12.1/macos
cmake -S . -B build -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="x86_64;arm64" -DCMAKE_OSX_DEPLOYMENT_TARGET=14.4
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
bash packaging/macos/build-macos.sh
```

The script fails if `lipo -archs` does not report both `x86_64` and `arm64`. It verifies
the ad-hoc code signature and signs the DMG too.

### Linux

Prerequisites: Qt 6.12.1 Linux kit, `linuxdeploy`, its Qt plugin, `appimagetool`,
`libfuse2`, `patchelf`, `rpm`, `zsync`, `dpkg-deb` and the XCB runtime development
libraries installed by the release workflow.

```bash
export TMOG_VERSION=0.1.0
export QT_ROOT_DIR=/path/to/Qt/6.12.1/gcc_64
cmake -S . -B build -DCMAKE_PREFIX_PATH="$QT_ROOT_DIR" -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
bash packaging/linux/build-linux.sh
```

The script downloads the continuous linuxdeploy/appimagetool builds, bundles Qt and the
XCB libraries, creates the AppImage, packages the same AppDir tree for deb/RPM, and
creates a portable tar archive. It expects `packaging/linux/task-manager-og.png` at the
AppDir root so appimagetool can create `.DirIcon`.

## Signing: what is actually true

These signatures prove integrity against the published keys. They do **not** establish a
trusted publisher identity.

- **Windows:** the workflow generates a new self-signed Authenticode certificate per run,
  signs the app EXE, setup EXE and MSI, and publishes the public `.cer`. Windows
  SmartScreen will still warn because the certificate is not trusted by a public CA.
- **macOS:** the app and DMG are ad-hoc signed (`codesign --sign -`). They are not signed
  with a Developer ID and are not notarized, so Gatekeeper will warn on first launch.
  Producing a trusted build requires an Apple Developer Program account and
  `notarytool`; no such credentials are stored in this repository.
- **Linux:** the packages themselves are not code-signed. `SHA256SUMS.txt` and
  `latest.json` are GPG-signed with a key generated inside the publish job; the public key
  is uploaded in the release. The key is not trusted by a public keyserver or CA.
- **Updater:** the application does not yet contain an update client. `latest.json` is a
  signed release manifest scaffold for future updater wiring, not a working automatic
  updater.

To move to trusted signing later, replace the generated certificate and ad-hoc signing
steps with repository secrets (Authenticode certificate, Apple Developer ID and
notarization credentials, and a stable GPG release key). Never commit private keys. Keep
the public certificate/key assets in the release for verification.

## Verify a published release

```bash
gh release view v0.1.0 --json isDraft,isPrerelease,assets \
  --jq '{isDraft,isPrerelease,count:(.assets|length)}'
# expected: isDraft=false, isPrerelease=true, count=15

mkdir -p /tmp/tmog-release && cd /tmp/tmog-release
gh release download v0.1.0 \
  -p 'latest.json*' -p 'SHA256SUMS.txt*' -p '*signing-key.asc'
gpg --import TaskManagerOG-0.1.0-linux-signing-key.asc
gpg --verify SHA256SUMS.txt.asc SHA256SUMS.txt
gpg --verify latest.json.asc latest.json
```

On Windows, inspect the Authenticode state of the setup EXE and MSI:

```powershell
Get-AuthenticodeSignature .\TaskManagerOG-0.1.0-windows-x64-setup.exe |
  Format-List Status, SignerCertificate
Get-AuthenticodeSignature .\TaskManagerOG-0.1.0-windows-x64.msi |
  Format-List Status, SignerCertificate
```

`UnknownError` is expected for the included self-signed certificate. It is still a
signature, but it is not a trusted identity.

## Cutting the next release

1. Bump `project(... VERSION ...)` in `CMakeLists.txt`, update release notes/docs, and
   commit.
2. Run the build-only workflow and inspect the artifacts as above.
3. Tag the exact commit with `vX.Y.Z` and push the tag.
4. Confirm the publish job reports exactly 15 assets, `isDraft=false`, and the expected
   prerelease/stable state.
5. Verify the checksum and manifest signatures with the downloaded public key.

Do not force-move an already published tag. If a build fails, fix the code, run the
build-only workflow, then publish a new patch version (or delete the failed release/tag
before it has been distributed).
