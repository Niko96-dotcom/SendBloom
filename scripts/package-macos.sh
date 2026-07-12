#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/Builds-v1}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
VERSION="$(tr -d '[:space:]' < "$ROOT/VERSION")"
RELEASE_LABEL="${RELEASE_LABEL:-rc0}"
OUT_DIR="${OUT_DIR:-$ROOT/dist/v${VERSION}-${RELEASE_LABEL}}"
REQUIRE_SIGNED="${REQUIRE_SIGNED:-1}"
ALLOW_DIRTY="${ALLOW_DIRTY:-0}"

[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "ERROR: VERSION must be numeric" >&2; exit 1; }
[[ "$RELEASE_LABEL" =~ ^[0-9A-Za-z][0-9A-Za-z.-]*$ ]] || { echo "ERROR: invalid RELEASE_LABEL" >&2; exit 1; }
[[ "$REQUIRE_SIGNED" == "0" || "$REQUIRE_SIGNED" == "1" ]] || { echo "ERROR: REQUIRE_SIGNED must be 0 or 1" >&2; exit 1; }
[[ "$ALLOW_DIRTY" == "0" || "$ALLOW_DIRTY" == "1" ]] || { echo "ERROR: ALLOW_DIRTY must be 0 or 1" >&2; exit 1; }

VST3="$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/VST3/SendBloom.vst3"
AU="$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/AU/SendBloom.component"
[[ -d "$VST3" ]] || { echo "ERROR: missing $VST3" >&2; exit 1; }
[[ -d "$AU" ]] || { echo "ERROR: missing $AU" >&2; exit 1; }

SOURCE_DIRTY=0
if [[ -n "$(git -C "$ROOT" status --porcelain)" ]]; then
  SOURCE_DIRTY=1
fi

if [[ "$ALLOW_DIRTY" != "1" && "$SOURCE_DIRTY" == "1" ]]; then
  echo "ERROR: refusing to package a dirty working tree (set ALLOW_DIRTY=1 only for a local rehearsal)" >&2
  exit 1
fi

for bundle in "$VST3" "$AU"; do
  if [[ "$REQUIRE_SIGNED" != "1" ]]; then
    echo "WARNING: applying an ad-hoc signature for local packaging rehearsal only: $bundle" >&2
    codesign --force --sign - --timestamp=none "$bundle"
  fi

  codesign --verify --deep --strict --verbose=2 "$bundle"

  if [[ "$REQUIRE_SIGNED" == "1" ]]; then
    signature_info="$(codesign --display --verbose=4 "$bundle" 2>&1)"
    if grep -q '^Signature=adhoc$' <<<"$signature_info" \
      || ! grep -Eq '^Authority=Developer ID Application:' <<<"$signature_info" \
      || ! grep -Eq '^TeamIdentifier=[A-Z0-9]+$' <<<"$signature_info" \
      || ! grep -Eq '^CodeDirectory .*\(runtime\)' <<<"$signature_info"; then
      echo "ERROR: $bundle is not Developer ID signed" >&2
      exit 1
    fi
  fi
done

vst3_binary="$VST3/Contents/MacOS/SendBloom"
au_binary="$AU/Contents/MacOS/SendBloom"
[[ -f "$vst3_binary" && -f "$au_binary" ]] || { echo "ERROR: plugin executables missing" >&2; exit 1; }

normalize_archs() {
  lipo -archs "$1" | tr ' ' '\n' | sort | paste -sd+ -
}

VST3_ARCHS="$(normalize_archs "$vst3_binary")"
AU_ARCHS="$(normalize_archs "$au_binary")"
[[ "$VST3_ARCHS" == "$AU_ARCHS" ]] || { echo "ERROR: AU/VST3 architectures differ" >&2; exit 1; }

if [[ "$VST3_ARCHS" == "arm64+x86_64" ]]; then
  ARCH_LABEL="universal2"
else
  ARCH_LABEL="${VST3_ARCHS//+/-}"
fi

for bundle in "$VST3" "$AU"; do
  plist="$bundle/Contents/Info.plist"
  actual_version="$(plutil -extract CFBundleShortVersionString raw -o - "$plist")"
  actual_id="$(plutil -extract CFBundleIdentifier raw -o - "$plist")"
  [[ "$actual_version" == "$VERSION" ]] || { echo "ERROR: version mismatch in $bundle" >&2; exit 1; }
  [[ "$actual_id" == "com.nikoaudiolabs.sendbloom" ]] || { echo "ERROR: bundle ID mismatch in $bundle" >&2; exit 1; }
done

mkdir -p "$OUT_DIR"
STAGE_DIR="$(mktemp -d "$OUT_DIR/stage.XXXXXX")"
trap 'rm -rf "$STAGE_DIR"' EXIT
mkdir -p "$STAGE_DIR/VST3" "$STAGE_DIR/Components"
ditto "$VST3" "$STAGE_DIR/VST3/SendBloom.vst3"
ditto "$AU" "$STAGE_DIR/Components/SendBloom.component"
cp "$ROOT/LICENSE" "$STAGE_DIR/LICENSE"
cp "$ROOT/docs/THIRD_PARTY_LICENSES.md" "$STAGE_DIR/THIRD_PARTY_LICENSES.txt"
cp "$ROOT/RELEASE_NOTES.md" "$STAGE_DIR/RELEASE_NOTES.md"

{
  echo "product=SendBloom"
  echo "version=$VERSION"
  echo "release_label=$RELEASE_LABEL"
  echo "source_commit=$(git -C "$ROOT" rev-parse HEAD)"
  echo "source_dirty=$SOURCE_DIRTY"
  echo "architectures=$VST3_ARCHS"
} > "$STAGE_DIR/BUILD_INFO.txt"

ARCHIVE="$OUT_DIR/SendBloom-${VERSION}-${RELEASE_LABEL}-macOS-${ARCH_LABEL}.zip"
rm -f "$ARCHIVE"
(
  cd "$STAGE_DIR"
  ditto -c -k --norsrc . "$ARCHIVE"
)
(cd "$OUT_DIR" && shasum -a 256 "$(basename "$ARCHIVE")" > SHA256SUMS)
echo "$ARCHIVE"
