#!/usr/bin/env bash
# Print the canonical release version fields derived from the VERSION file.
#
# Every other script, CI job, doc example and artifact name must obtain the
# version from here rather than restating it.
#
#   scripts/release/version.sh              # all fields, KEY=VALUE
#   scripts/release/version.sh version      # full canonical, e.g. X.Y.Z-rcN
#   scripts/release/version.sh core         # numeric core, e.g. X.Y.Z
#   scripts/release/version.sh prerelease   # rcN  (empty when stable)
#   scripts/release/version.sh tag          # vX.Y.Z-rcN
#   scripts/release/version.sh build-id     # canonical version plus short commit
#   scripts/release/version.sh previous     # previous released version, if any

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

load_version
load_build_identity

# The previous released version is derived from git tags, never hand-written.
previous_version() {
  git -C "$ROOT" tag --list 'v*' \
    | sed 's/^v//' \
    | grep -E '^[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z.]+)?$' \
    | grep -vFx "$RELEASE_VERSION" \
    | sort -V \
    | tail -n 1
}

case "${1:-all}" in
  version)    printf '%s\n' "$RELEASE_VERSION" ;;
  core)       printf '%s\n' "$VERSION_CORE" ;;
  prerelease) printf '%s\n' "$VERSION_PRERELEASE" ;;
  tag)        printf '%s\n' "$RELEASE_TAG" ;;
  build-id)   printf '%s\n' "$BUILD_ID" ;;
  commit)     printf '%s\n' "$SOURCE_COMMIT" ;;
  previous)   previous_version ;;
  all)
    cat <<EOF
RELEASE_VERSION=$RELEASE_VERSION
VERSION_CORE=$VERSION_CORE
VERSION_PRERELEASE=$VERSION_PRERELEASE
RELEASE_TAG=$RELEASE_TAG
RELEASE_IS_PRERELEASE=$RELEASE_IS_PRERELEASE
BUILD_ID=$BUILD_ID
SOURCE_COMMIT=$SOURCE_COMMIT
SOURCE_DIRTY=$SOURCE_DIRTY
PREVIOUS_VERSION=$(previous_version)
EOF
    ;;
  *) die "unknown field: $1" ;;
esac
