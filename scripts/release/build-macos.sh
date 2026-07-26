#!/usr/bin/env bash
# Clean macOS release build.
#
# Always builds into a dedicated release tree that is removed first, so a
# stale artifact from an earlier version can never be picked up by the
# packaging step. This is the only build the release pipeline trusts; the
# developer's day-to-day Builds/ directory is never packaged.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd cmake git lipo plutil

KEEP_BUILD_DIR="${KEEP_BUILD_DIR:-0}"
OSX_ARCHS="${OSX_ARCHS:-arm64;x86_64}"
DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET:-11.0}"
JOBS="${JOBS:-$(sysctl -n hw.ncpu)}"

step "Clean release build — SendBloom $RELEASE_VERSION ($BUILD_ID)"
info "build dir:   $RELEASE_BUILD_DIR"
info "arch:        $OSX_ARCHS"
info "deployment:  macOS $DEPLOYMENT_TARGET"
info "jobs:        $JOBS"

# --- submodules ------------------------------------------------------------
step "Submodules"
git -C "$ROOT" submodule update --init --recursive
ok "submodules initialised"

# --- clean -----------------------------------------------------------------
step "Removing stale release build tree"
if [[ -d "$RELEASE_BUILD_DIR" ]]; then
  if [[ "$KEEP_BUILD_DIR" == "1" ]]; then
    warn "KEEP_BUILD_DIR=1 — reusing $RELEASE_BUILD_DIR. This is NOT a clean build."
  else
    rm -rf "$RELEASE_BUILD_DIR"
    ok "removed $RELEASE_BUILD_DIR"
  fi
else
  ok "no previous release build tree"
fi

step "Removing stale artifacts for this version"
# Clear the artifacts and their metadata, but not the whole directory: the
# orchestrator has already opened its logs and its in-progress report in here,
# and deleting those mid-run would destroy the record of the release as it is
# being made. Everything a previous build could leave behind is named
# explicitly, so nothing stale can survive into packaging.
mkdir -p "$DIST_DIR"
removed=0
while IFS= read -r stale; do
  rm -rf "$stale"
  removed=$((removed + 1))
done < <(find "$DIST_DIR" -maxdepth 1 \
  \( -name '*.dmg' -o -name '*.pkg' -o -name '*.zip' \
     -o -name 'SHA256SUMS.txt' -o -name 'release-manifest.json' \
     -o -name 'provenance.json' -o -name '*-sbom.json' \
     -o -name 'notarization.txt' -o -name 'build-environment.txt' \
     -o -name 'release-body.md' -o -name 'stage.*' -o -name 'dmg-stage.*' \
     -o -name 'pkg-work.*' -o -name 'notary-stage.*' \) -print)
ok "cleared $removed stale artifact(s) from $DIST_DIR"

# --- configure -------------------------------------------------------------
step "CMake configure"
GENERATOR_ARGS=()
if command -v ninja >/dev/null 2>&1; then
  GENERATOR_ARGS=(-G Ninja)
  info "generator: Ninja"
else
  info "generator: default (Ninja not installed)"
fi

cmake -B "$RELEASE_BUILD_DIR" -S "$ROOT" \
  ${GENERATOR_ARGS[@]+"${GENERATOR_ARGS[@]}"} \
  -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
  -DCMAKE_OSX_ARCHITECTURES="$OSX_ARCHS" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="$DEPLOYMENT_TARGET"
ok "configured"

# The configured version must be the canonical one, verified from the copy
# CMake made rather than from our own parse.
configured_version="$(head -n1 "$RELEASE_BUILD_DIR/VERSION" | tr -d '[:space:]')"
[[ "$configured_version" == "$RELEASE_VERSION" ]] \
  || die "build tree configured version '$configured_version' != canonical '$RELEASE_VERSION'"
ok "build tree carries canonical version $RELEASE_VERSION"

# --- build -----------------------------------------------------------------
step "Build ($BUILD_TYPE)"
cmake --build "$RELEASE_BUILD_DIR" --config "$BUILD_TYPE" --parallel "$JOBS"
ok "build complete"

# --- artifact presence and shape ------------------------------------------
step "Build output"

[[ -d "$VST3_BUNDLE" ]] || die "missing VST3 bundle: $VST3_BUNDLE"
[[ -d "$AU_BUNDLE" ]]   || die "missing AU bundle: $AU_BUNDLE"
ok "VST3: $VST3_BUNDLE"
ok "AU:   $AU_BUNDLE"

vst3_bin="$VST3_BUNDLE/Contents/MacOS/SendBloom"
au_bin="$AU_BUNDLE/Contents/MacOS/SendBloom"
[[ -f "$vst3_bin" ]] || die "missing VST3 executable: $vst3_bin"
[[ -f "$au_bin" ]]   || die "missing AU executable: $au_bin"

normalise_archs() { lipo -archs "$1" | tr ' ' '\n' | sort | paste -sd+ -; }
vst3_archs="$(normalise_archs "$vst3_bin")"
au_archs="$(normalise_archs "$au_bin")"

[[ "$vst3_archs" == "$au_archs" ]] \
  || die "AU and VST3 architectures differ: AU=$au_archs VST3=$vst3_archs"

expected_archs="$(printf '%s' "$OSX_ARCHS" | tr ';' '\n' | sort | paste -sd+ -)"
[[ "$vst3_archs" == "$expected_archs" ]] \
  || die "built architectures '$vst3_archs' do not match requested '$expected_archs'"
ok "architectures: $vst3_archs"

for bundle in "$VST3_BUNDLE" "$AU_BUNDLE"; do
  plist="$bundle/Contents/Info.plist"
  short="$(plutil -extract CFBundleShortVersionString raw -o - "$plist")"
  ident="$(plutil -extract CFBundleIdentifier raw -o - "$plist")"
  [[ "$short" == "$VERSION_CORE" ]] || die "$bundle reports version $short, expected $VERSION_CORE"
  [[ "$ident" == "$BUNDLE_ID" ]]    || die "$bundle reports id $ident, expected $BUNDLE_ID"
done
ok "bundle metadata: version $VERSION_CORE, id $BUNDLE_ID"

# Record the build inputs next to the artifacts.
cat >"$DIST_DIR/build-environment.txt" <<EOF
release_version=$RELEASE_VERSION
version_core=$VERSION_CORE
build_id=$BUILD_ID
source_commit=$SOURCE_COMMIT
source_dirty=$SOURCE_DIRTY
build_type=$BUILD_TYPE
architectures=$vst3_archs
deployment_target=$DEPLOYMENT_TARGET
cmake_version=$(cmake --version | head -n1)
xcode=$(xcodebuild -version 2>/dev/null | paste -sd' ' - || echo unknown)
sdk=$(xcrun --show-sdk-version 2>/dev/null || echo unknown)
host_os=$(sw_vers -productVersion)
built_at=$(date -u +%Y-%m-%dT%H:%M:%SZ)
EOF
ok "recorded $DIST_DIR/build-environment.txt"

echo
printf '%sbuild-macos: PASS%s  %s\n' "$_c_grn" "$_c_off" "$BUILD_ID"
