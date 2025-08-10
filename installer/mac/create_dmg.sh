#!/usr/bin/env bash
set -euo pipefail

VERSION=${1:-0.0.0}
APP="dist/FreeCrafter.app"
DMG="FreeCrafter-${VERSION}.dmg"

if [ ! -d "$APP" ]; then
  echo "App bundle $APP not found" >&2
  exit 1
fi

hdiutil create -volname "FreeCrafter" -srcfolder "$APP" -ov -format UDZO "$DMG"
