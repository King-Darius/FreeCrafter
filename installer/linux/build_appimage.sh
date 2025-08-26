#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-0.0.0}
BINARY="dist/FreeCrafter"
APPDIR="AppDir"
APPIMAGE="FreeCrafter-${VERSION}.AppImage"
LINUXDEPLOY=${LINUXDEPLOY:-linuxdeploy}
APPIMAGETOOL=${APPIMAGETOOL:-appimagetool}

if ! command -v "$LINUXDEPLOY" >/dev/null 2>&1; then
  echo "Error: $LINUXDEPLOY is required to build AppImages. Install linuxdeploy or set LINUXDEPLOY." >&2
  exit 1
fi

if ! command -v "$APPIMAGETOOL" >/dev/null 2>&1; then
  echo "Error: $APPIMAGETOOL is required to build AppImages. Install appimagetool or set APPIMAGETOOL." >&2
  exit 1
fi

if [ ! -f "$BINARY" ]; then
  echo "Binary $BINARY not found" >&2
  exit 1
fi

rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
cp "$BINARY" "$APPDIR/usr/bin/FreeCrafter"
chmod +x "$APPDIR/usr/bin/FreeCrafter"

$LINUXDEPLOY --appdir "$APPDIR" -e "$APPDIR/usr/bin/FreeCrafter"
$APPIMAGETOOL "$APPDIR" "$APPIMAGE"
chmod +x "$APPIMAGE"
