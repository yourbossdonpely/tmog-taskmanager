#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="${TMOG_VERSION:?TMOG_VERSION is required}"
qt="${QT_ROOT_DIR:?QT_ROOT_DIR is required}"
dist="$root/dist/linux"
work="$root/build/packaging/linux"
appdir="$work/AppDir"
pkgroot="$work/pkgroot"

rm -rf "$dist" "$work"
mkdir -p "$dist" "$work" "$appdir"

export APPIMAGE_EXTRACT_AND_RUN=1
export QMAKE="$qt/bin/qmake"
export PATH="$qt/bin:$PATH"

linuxdeploy="$work/linuxdeploy"
qt_plugin="$work/linuxdeploy-plugin-qt-x86_64.AppImage"
appimagetool="$work/appimagetool"
curl -fL --retry 3 -o "$linuxdeploy" \
    https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage
curl -fL --retry 3 -o "$qt_plugin" \
    https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage
curl -fL --retry 3 -o "$appimagetool" \
    https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage
chmod +x "$linuxdeploy" "$qt_plugin" "$appimagetool"
ln -sf "$qt_plugin" "$work/linuxdeploy-plugin-qt"
export PATH="$work:$qt/bin:$PATH"

"$linuxdeploy" \
    --appdir "$appdir" \
    --executable "$root/build/TaskManagerOG" \
    --desktop-file "$root/packaging/linux/task-manager-og.desktop" \
    --icon-file "$root/packaging/linux/task-manager-og.svg" \
    --plugin qt

cp "$root/packaging/linux/task-manager-og.png" "$appdir/task-manager-og.png"
cp "$root/packaging/linux/task-manager-og.png" "$appdir/.DirIcon"

appimage="$dist/TaskManagerOG-$version-linux-x86_64.AppImage"
ARCH=x86_64 "$appimagetool" "$appdir" "$appimage"

rm -rf "$pkgroot"
mkdir -p "$pkgroot/opt/task-manager-og" \
    "$pkgroot/usr/bin" \
    "$pkgroot/usr/share/applications" \
    "$pkgroot/usr/share/icons/hicolor/scalable/apps" \
    "$pkgroot/usr/share/icons/hicolor/256x256/apps" \
    "$pkgroot/usr/share/doc/task-manager-og"
cp -a "$appdir/." "$pkgroot/opt/task-manager-og/"
cp "$root/packaging/linux/task-manager-og.desktop" "$pkgroot/usr/share/applications/"
cp "$root/packaging/linux/task-manager-og.svg" "$pkgroot/usr/share/icons/hicolor/scalable/apps/"
cp "$root/packaging/linux/task-manager-og.png" "$pkgroot/usr/share/icons/hicolor/256x256/apps/"
cp "$root/LICENSE" "$root/docs/NOTICE.md" "$pkgroot/usr/share/doc/task-manager-og/"
cat > "$pkgroot/usr/bin/TaskManagerOG" <<'EOF'
#!/bin/sh
exec /opt/task-manager-og/AppRun "$@"
EOF
chmod 0755 "$pkgroot/usr/bin/TaskManagerOG"

install_size="$(du -sk "$pkgroot" | cut -f1)"
mkdir -p "$pkgroot/DEBIAN"
cat > "$pkgroot/DEBIAN/control" <<EOF
Package: task-manager-og
Version: $version
Section: utils
Priority: optional
Architecture: amd64
Maintainer: Don PELY <noreply@donpely.com>
Installed-Size: $install_size
Depends: libc6 (>= 2.35), libstdc++6, libgl1, libxkbcommon0, libxcb-cursor0, libxcb1, libx11-6, libxext6, libxrender1
Description: Task Manager OG by Don PELY
 Native, privacy-first system monitor for Windows, macOS and Linux.
EOF
dpkg-deb --build "$pkgroot" "$dist/TaskManagerOG-$version-linux-amd64.deb"
rm -rf "$pkgroot/DEBIAN"

mkdir -p "$work/rpmbuild/BUILD" "$work/rpmbuild/RPMS" "$work/rpmbuild/SOURCES" \
    "$work/rpmbuild/SPECS" "$work/rpmbuild/SRPMS"
cat > "$work/rpmbuild/SPECS/task-manager-og.spec" <<EOF
%global debug_package %{nil}
Name: task-manager-og
Version: $version
Release: 1%{?dist}
Summary: Task Manager OG by Don PELY
License: MIT
BuildArch: x86_64
AutoReqProv: no
Requires: glibc, libstdc++, mesa-libGL, libxkbcommon, xcb-util-cursor
%description
Native, privacy-first system monitor for Windows, macOS and Linux.
%install
rm -rf %{buildroot}
mkdir -p %{buildroot}
cp -a "$pkgroot/." %{buildroot}/
%files
/opt/task-manager-og
/usr/bin/TaskManagerOG
/usr/share/applications/task-manager-og.desktop
/usr/share/icons/hicolor/scalable/apps/task-manager-og.svg
/usr/share/icons/hicolor/256x256/apps/task-manager-og.png
/usr/share/doc/task-manager-og
%changelog
* Thu Nov 26 2026 Don PELY <noreply@donpely.com> - $version-1
- Initial prerelease build.
EOF
rpmbuild -bb --define "_topdir $work/rpmbuild" \
    "$work/rpmbuild/SPECS/task-manager-og.spec"
cp "$work"/rpmbuild/RPMS/x86_64/*.rpm "$dist/TaskManagerOG-$version-linux-x86_64.rpm"

tar -C "$appdir" -czf "$dist/TaskManagerOG-$version-linux-x86_64.tar.gz" .
ls -lh "$dist"
