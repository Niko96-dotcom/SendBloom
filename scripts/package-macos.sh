#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/Builds-v1}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
OUT_DIR="${OUT_DIR:-$ROOT/dist/v1.0.0-rc0}"
VERSION="$(tr -d '[:space:]' < "$ROOT/VERSION")"

[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "ERROR: VERSION must be numeric" >&2; exit 1; }

VST3="$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/VST3/SendBloom.vst3"
AU="$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/AU/SendBloom.component"
[[ -d "$VST3" ]] || { echo "ERROR: missing $VST3" >&2; exit 1; }
[[ -d "$AU" ]] || { echo "ERROR: missing $AU" >&2; exit 1; }

mkdir -p "$OUT_DIR/stage/VST3" "$OUT_DIR/stage/Components"
ditto "$VST3" "$OUT_DIR/stage/VST3/SendBloom.vst3"
ditto "$AU" "$OUT_DIR/stage/Components/SendBloom.component"
cp "$ROOT/LICENSE" "$OUT_DIR/stage/LICENSE"
cp "$ROOT/docs/THIRD_PARTY_LICENSES.md" "$OUT_DIR/stage/THIRD_PARTY_LICENSES.txt"

ARCH="$(uname -m)"
ARCHIVE="$OUT_DIR/SendBloom-${VERSION}-rc0-macOS-${ARCH}.zip"
rm -f "$ARCHIVE"
ditto -c -k --sequesterRsrc --keepParent "$OUT_DIR/stage" "$ARCHIVE"
(cd "$OUT_DIR" && shasum -a 256 "$(basename "$ARCHIVE")" > SHA256SUMS)
echo "$ARCHIVE"
