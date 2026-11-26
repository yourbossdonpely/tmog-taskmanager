#!/usr/bin/env bash
set -euo pipefail

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
version="${TMOG_VERSION:?TMOG_VERSION is required}"
qt="${QT_ROOT_DIR:?QT_ROOT_DIR is required}"
dist="$root/dist/macos"
work="$root/build/packaging/macos"

rm -rf "$dist" "$work"
mkdir -p "$dist" "$work"

app="$(find "$root/build" -maxdepth 3 -name 'TaskManagerOG.app' -type d | head -n 1)"
if [[ -z "$app" ]]; then
    echo "TaskManagerOG.app was not found" >&2
    exit 1
fi

"$qt/bin/macdeployqt" "$app" -always-overwrite

archs="$(lipo -archs "$app/Contents/MacOS/TaskManagerOG")"
echo "macOS binary architectures: $archs"
[[ "$archs" == *x86_64* ]] || { echo "universal build is missing x86_64" >&2; exit 1; }
[[ "$archs" == *arm64* ]] || { echo "universal build is missing arm64" >&2; exit 1; }

# Without an Apple Developer ID this is an ad-hoc signature. It makes the bundle
# internally consistent but does not satisfy Gatekeeper notarization.
codesign --force --deep --sign - \
    --identifier "com.donpely.taskmanagerog" \
    "$app"
codesign --verify --deep --strict --verbose=2 "$app"
codesign -dv --verbose=4 "$app" 2>&1 | sed -n '1,12p'

dmg_root="$work/dmg"
mkdir -p "$dmg_root"
cp -R "$app" "$dmg_root/"
ln -s /Applications "$dmg_root/Applications"
cp "$root/LICENSE" "$dmg_root/LICENSE"
cp "$root/docs/NOTICE.md" "$dmg_root/NOTICE.md"

dmg="$dist/TaskManagerOG-$version-macos-universal.dmg"
hdiutil create \
    -volname "Task Manager OG by Don PELY" \
    -srcfolder "$dmg_root" \
    -ov \
    -format UDZO \
    "$dmg"
codesign --force --sign - "$dmg"
codesign --verify --verbose=2 "$dmg"

zip="$dist/TaskManagerOG-$version-macos-universal-portable.zip"
ditto -c -k --sequesterRsrc --keepParent "$app" "$zip"

ls -lh "$dist"
