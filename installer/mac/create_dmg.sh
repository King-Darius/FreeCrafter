#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-0.0.0}
APP="dist/FreeCrafter.app"
DMG="FreeCrafter-${VERSION}.dmg"

if ! command -v hdiutil >/dev/null 2>&1; then
  echo "Error: hdiutil is required to create DMG images. Run this script on macOS with hdiutil available." >&2
  exit 1
fi

if [ ! -d "$APP" ]; then
  echo "App bundle $APP not found" >&2
  exit 1
fi

hdiutil create -volname "FreeCrafter" -srcfolder "$APP" -ov -format UDZO "$DMG"
