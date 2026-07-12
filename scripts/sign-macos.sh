#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/Builds-v1}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
: "${DEVELOPER_ID_APPLICATION:?Set DEVELOPER_ID_APPLICATION to the exact keychain identity}"

for bundle in \
  "$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/VST3/SendBloom.vst3" \
  "$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/AU/SendBloom.component"; do
  [[ -d "$bundle" ]] || { echo "ERROR: missing $bundle" >&2; exit 1; }
  codesign --force --deep --options runtime --timestamp --sign "$DEVELOPER_ID_APPLICATION" "$bundle"
  codesign --verify --deep --strict --verbose=2 "$bundle"
done
