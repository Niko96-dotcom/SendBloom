#!/usr/bin/env bash
# Shared helpers for the SendBloom release scripts.
#
# Every release script sources this file. It provides the canonical version
# parse, the derived artifact naming, logging, and the fail-closed helpers.
# It never reads a credential value and never echoes one.

# shellcheck disable=SC2034  # several exports are consumed by sourcing scripts

set -euo pipefail

RELEASE_LIB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$RELEASE_LIB_DIR/../.." && pwd)"
export ROOT

# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------

if [[ -t 1 && -z "${NO_COLOR:-}" ]]; then
  _c_red=$'\033[31m'; _c_grn=$'\033[32m'; _c_yel=$'\033[33m'
  _c_bld=$'\033[1m';  _c_off=$'\033[0m'
else
  _c_red=""; _c_grn=""; _c_yel=""; _c_bld=""; _c_off=""
fi

step() { printf '%s==> %s%s\n' "$_c_bld" "$*" "$_c_off"; }
info() { printf '    %s\n' "$*"; }
ok()   { printf '    %sOK%s   %s\n' "$_c_grn" "$_c_off" "$*"; }
warn() { printf '    %sWARN%s %s\n' "$_c_yel" "$_c_off" "$*" >&2; }
die()  { printf '%sERROR%s %s\n' "$_c_red" "$_c_off" "$*" >&2; exit 1; }

# Loud banner for the local-only, non-publishable rehearsal mode.
unsigned_banner() {
  cat >&2 <<'BANNER'

################################################################################
#                                                                              #
#   LOCAL REHEARSAL BUILD — NOT A RELEASE                                      #
#                                                                              #
#   RELEASE_MODE=local-unsigned is set. Artifacts produced by this run are     #
#   ad-hoc signed, NOT notarized, NOT stapled, and MUST NOT be published,      #
#   uploaded, shared, or installed on any machine other than this one.         #
#   Every artifact is name-tagged LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE.            #
#                                                                              #
################################################################################

BANNER
}

# ---------------------------------------------------------------------------
# Canonical version (single truth source: the top-level VERSION file)
# ---------------------------------------------------------------------------

VERSION_FILE="$ROOT/VERSION"

load_version() {
  [[ -f "$VERSION_FILE" ]] || die "canonical version source missing: $VERSION_FILE"

  RELEASE_VERSION="$(head -n 1 "$VERSION_FILE" | tr -d '[:space:]')"
  [[ -n "$RELEASE_VERSION" ]] || die "VERSION file is empty"

  if [[ ! "$RELEASE_VERSION" =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)(-([0-9A-Za-z]+(\.[0-9A-Za-z]+)*))?$ ]]; then
    die "VERSION must be MAJOR.MINOR.PATCH[-prerelease], got: '$RELEASE_VERSION'"
  fi

  VERSION_MAJOR="${BASH_REMATCH[1]}"
  VERSION_MINOR="${BASH_REMATCH[2]}"
  VERSION_PATCH="${BASH_REMATCH[3]}"
  VERSION_CORE="$VERSION_MAJOR.$VERSION_MINOR.$VERSION_PATCH"
  VERSION_PRERELEASE="${BASH_REMATCH[5]:-}"

  RELEASE_TAG="v$RELEASE_VERSION"
  if [[ -n "$VERSION_PRERELEASE" ]]; then
    RELEASE_IS_PRERELEASE=1
  else
    RELEASE_IS_PRERELEASE=0
  fi

  export RELEASE_VERSION VERSION_CORE VERSION_PRERELEASE RELEASE_TAG RELEASE_IS_PRERELEASE
  export VERSION_MAJOR VERSION_MINOR VERSION_PATCH
}

# ---------------------------------------------------------------------------
# Build identity
# ---------------------------------------------------------------------------

load_build_identity() {
  SOURCE_COMMIT="$(git -C "$ROOT" rev-parse HEAD)"
  SOURCE_COMMIT_SHORT="$(git -C "$ROOT" rev-parse --short=12 HEAD)"

  if [[ -n "$(git -C "$ROOT" status --porcelain)" ]]; then
    SOURCE_DIRTY=1
  else
    SOURCE_DIRTY=0
  fi

  # Immutable build identity: canonical version + commit. A dirty tree is
  # marked in the identity itself so a rehearsal artifact can never be
  # mistaken for a clean one.
  if [[ "$SOURCE_DIRTY" == "1" ]]; then
    BUILD_ID="${RELEASE_VERSION}+${SOURCE_COMMIT_SHORT}.dirty"
  else
    BUILD_ID="${RELEASE_VERSION}+${SOURCE_COMMIT_SHORT}"
  fi

  export SOURCE_COMMIT SOURCE_COMMIT_SHORT SOURCE_DIRTY BUILD_ID
}

# ---------------------------------------------------------------------------
# Release mode / artifact contract
# ---------------------------------------------------------------------------

# RELEASE_MODE
#   public          — the real thing. Developer ID signing, notarization,
#                     stapling and validation are all REQUIRED. Missing
#                     credentials are a hard failure.
#   local-unsigned  — local rehearsal only. Ad-hoc signature, no notarization,
#                     artifacts renamed so they cannot be confused with a
#                     release, publishing hard-blocked.
#
# SENDBLOOM_ARTIFACT_CONTRACT
#   pkg-in-dmg      — signed+notarized+stapled .pkg inside a
#                     signed+notarized+stapled .dmg. Requires a
#                     "Developer ID Installer" identity.
#   bundles-in-dmg  — signed+notarized+stapled .vst3 and .component inside a
#                     signed+notarized+stapled .dmg. Requires only a
#                     "Developer ID Application" identity.

load_release_mode() {
  RELEASE_MODE="${RELEASE_MODE:-local-unsigned}"
  case "$RELEASE_MODE" in
    public|local-unsigned) ;;
    *) die "RELEASE_MODE must be 'public' or 'local-unsigned', got '$RELEASE_MODE'" ;;
  esac

  ARTIFACT_CONTRACT="${SENDBLOOM_ARTIFACT_CONTRACT:-pkg-in-dmg}"
  case "$ARTIFACT_CONTRACT" in
    pkg-in-dmg|bundles-in-dmg) ;;
    *) die "SENDBLOOM_ARTIFACT_CONTRACT must be 'pkg-in-dmg' or 'bundles-in-dmg', got '$ARTIFACT_CONTRACT'" ;;
  esac

  if [[ "$RELEASE_MODE" == "public" ]]; then
    REQUIRE_SIGNED=1
    ARTIFACT_SUFFIX=""
  else
    REQUIRE_SIGNED=0
    ARTIFACT_SUFFIX="-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE"
  fi

  export RELEASE_MODE ARTIFACT_CONTRACT REQUIRE_SIGNED ARTIFACT_SUFFIX
}

# ---------------------------------------------------------------------------
# Derived paths and artifact names — never hand-written anywhere else
# ---------------------------------------------------------------------------

load_release_paths() {
  RELEASE_BUILD_DIR="${RELEASE_BUILD_DIR:-$ROOT/build-release}"
  DIST_DIR="${DIST_DIR:-$ROOT/dist/$RELEASE_VERSION}"
  BUILD_TYPE="${BUILD_TYPE:-Release}"

  ARTEFACTS_DIR="$RELEASE_BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE"
  VST3_BUNDLE="$ARTEFACTS_DIR/VST3/SendBloom.vst3"
  AU_BUNDLE="$ARTEFACTS_DIR/AU/SendBloom.component"

  PKG_NAME="SendBloom-${RELEASE_VERSION}${ARTIFACT_SUFFIX}.pkg"
  DMG_NAME="SendBloom-${RELEASE_VERSION}-macOS${ARTIFACT_SUFFIX}.dmg"
  NOTARIZE_ZIP_NAME="SendBloom-${RELEASE_VERSION}-bundles-for-notarization.zip"
  CHECKSUM_NAME="SHA256SUMS.txt"
  MANIFEST_NAME="release-manifest.json"
  PROVENANCE_NAME="provenance.json"
  SBOM_NAME="SendBloom-${RELEASE_VERSION}-sbom.json"

  PKG_PATH="$DIST_DIR/$PKG_NAME"
  DMG_PATH="$DIST_DIR/$DMG_NAME"
  CHECKSUM_PATH="$DIST_DIR/$CHECKSUM_NAME"
  MANIFEST_PATH="$DIST_DIR/$MANIFEST_NAME"
  PROVENANCE_PATH="$DIST_DIR/$PROVENANCE_NAME"
  SBOM_PATH="$DIST_DIR/$SBOM_NAME"

  BUNDLE_ID="com.nikoaudiolabs.sendbloom"
  PKG_IDENTIFIER="com.nikoaudiolabs.sendbloom.installer"
  VST3_INSTALL_DIR="/Library/Audio/Plug-Ins/VST3"
  AU_INSTALL_DIR="/Library/Audio/Plug-Ins/Components"

  export RELEASE_BUILD_DIR DIST_DIR BUILD_TYPE ARTEFACTS_DIR VST3_BUNDLE AU_BUNDLE
  export PKG_NAME DMG_NAME NOTARIZE_ZIP_NAME CHECKSUM_NAME MANIFEST_NAME PROVENANCE_NAME SBOM_NAME
  export PKG_PATH DMG_PATH CHECKSUM_PATH MANIFEST_PATH PROVENANCE_PATH SBOM_PATH
  export BUNDLE_ID PKG_IDENTIFIER VST3_INSTALL_DIR AU_INSTALL_DIR
}

# The single artifact users are expected to download for a given contract.
primary_artifact_path() {
  # Both contracts ship the DMG as the public download; pkg-in-dmg nests the
  # installer inside it.
  printf '%s' "$DMG_PATH"
}

release_init() {
  load_version
  load_build_identity
  load_release_mode
  load_release_paths
}

# ---------------------------------------------------------------------------
# Credential presence checks (presence only — values are never printed)
# ---------------------------------------------------------------------------

# Never pipe `security` straight into `grep -q`: grep exits on the first match,
# `security` takes SIGPIPE, and under `set -o pipefail` the whole pipeline
# reports failure. That turns a perfectly valid identity into an intermittent
# "identity not found" abort. Capture first, match second.
keychain_lists_identity() {
  local needle="$1" listing
  listing="$(security find-identity -v -p codesigning 2>/dev/null || true)"
  [[ "$listing" == *"$needle"* ]]
}

require_application_identity() {
  [[ -n "${DEVELOPER_ID_APPLICATION:-}" ]] \
    || die "DEVELOPER_ID_APPLICATION is not set. A public release cannot be signed. See docs/release.md."
  keychain_lists_identity "$DEVELOPER_ID_APPLICATION" \
    || die "Developer ID Application identity not found in the keychain: $DEVELOPER_ID_APPLICATION"
}

require_installer_identity() {
  [[ -n "${DEVELOPER_ID_INSTALLER:-}" ]] \
    || die "DEVELOPER_ID_INSTALLER is not set, but SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg requires a Developer ID Installer identity.
       Either obtain that certificate (see docs/release.md) or choose the
       explicitly-documented alternate contract:
         SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg"
  local listing
  listing="$(security find-identity -v 2>/dev/null || true)"
  [[ "$listing" == *"$DEVELOPER_ID_INSTALLER"* ]] \
    || die "Developer ID Installer identity not found in the keychain: $DEVELOPER_ID_INSTALLER"
}

require_notary_profile() {
  [[ -n "${NOTARY_PROFILE:-}" ]] \
    || die "NOTARY_PROFILE is not set. A public release must be notarized. See docs/release.md."
}

# ---------------------------------------------------------------------------
# Signature verification that tolerates real codesign output variants
# ---------------------------------------------------------------------------

# Hardened runtime is reported by codesign in more than one shape depending on
# the tool version and the bundle:
#
#   CodeDirectory v=20500 size=... flags=0x10000(runtime) ...
#   CodeDirectory v=20500 size=... flags=0x10002(adhoc,runtime) ...
#   Runtime Version=<os version>
#
# Accept any of them. Rejecting the ones we happened not to see first is how
# a correct release gets blocked, and how a wrong one gets waved through.
codesign_has_hardened_runtime() {
  local info="$1"
  grep -Eq '^CodeDirectory .*flags=0x[0-9a-fA-F]+\(([^)]*,)?runtime(,[^)]*)?\)' <<<"$info" && return 0
  grep -Eq '^[[:space:]]*Runtime Version=' <<<"$info" && return 0
  grep -Eq '^[[:space:]]*flags=0x[0-9a-fA-F]+\(([^)]*,)?runtime(,[^)]*)?\)' <<<"$info" && return 0
  return 1
}

codesign_is_adhoc() {
  local info="$1"
  grep -Eq '^Signature=adhoc$' <<<"$info"
}

codesign_authority_is_developer_id_application() {
  local info="$1"
  grep -Eq '^Authority=Developer ID Application:' <<<"$info"
}

codesign_has_team_identifier() {
  local info="$1"
  grep -Eq '^TeamIdentifier=[A-Z0-9]{6,}$' <<<"$info"
}

codesign_has_secure_timestamp() {
  local info="$1"
  grep -Eq '^Timestamp=' <<<"$info"
}

# Full Developer ID assertion for one signed path.
assert_developer_id_signed() {
  local target="$1"
  local info
  info="$(codesign --display --verbose=4 "$target" 2>&1)"

  codesign_is_adhoc "$info" \
    && die "$target carries an ad-hoc signature; a public release requires Developer ID"
  codesign_authority_is_developer_id_application "$info" \
    || die "$target is not signed by a Developer ID Application authority"
  codesign_has_team_identifier "$info" \
    || die "$target has no Team Identifier"
  codesign_has_secure_timestamp "$info" \
    || die "$target was signed without a secure timestamp"
  codesign_has_hardened_runtime "$info" \
    || die "$target was not signed with the hardened runtime"
}

# ---------------------------------------------------------------------------
# Misc
# ---------------------------------------------------------------------------

require_macos() {
  [[ "$(uname -s)" == "Darwin" ]] || die "this script only runs on macOS"
}

require_cmd() {
  for c in "$@"; do
    command -v "$c" >/dev/null 2>&1 || die "required tool not found on PATH: $c"
  done
}

sha256_of() {
  shasum -a 256 "$1" | awk '{print $1}'
}

# Print a JSON string literal for arbitrary shell text.
json_str() {
  printf '%s' "$1" | python3 -c 'import json,sys; print(json.dumps(sys.stdin.read()))'
}
