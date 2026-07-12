#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/Builds-v1}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
: "${DEVELOPER_ID_APPLICATION:?Set DEVELOPER_ID_APPLICATION to the exact keychain identity}"

if ! security find-identity -v -p codesigning | grep -Fq "$DEVELOPER_ID_APPLICATION"; then
  echo "ERROR: Developer ID Application identity is not available: $DEVELOPER_ID_APPLICATION" >&2
  exit 1
fi

for bundle in \
  "$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/VST3/SendBloom.vst3" \
  "$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/AU/SendBloom.component"; do
  [[ -d "$bundle" ]] || { echo "ERROR: missing $bundle" >&2; exit 1; }
  codesign --force --options runtime --timestamp --sign "$DEVELOPER_ID_APPLICATION" "$bundle"
  codesign --verify --deep --strict --verbose=2 "$bundle"
  codesign --display --verbose=4 "$bundle" 2>&1 | grep -E '^(Identifier|Authority|TeamIdentifier|Runtime Version)='
done
