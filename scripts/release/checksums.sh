#!/usr/bin/env bash
# Generate SHA256SUMS.txt for the finalised release artifacts.
#
# Two rules this script exists to enforce:
#
#   1. Checksums are generated only after signing, notarization, stapling and
#      packaging are complete. Stapling REWRITES the artifact, so a checksum
#      taken before it is wrong. This script refuses to run unless the
#      artifacts look finalised for the current release mode.
#
#   2. The file lists artifact BASENAMES only. A checksum file containing
#      machine-specific absolute paths is useless to the person who downloads
#      it, because `shasum -c` resolves the recorded path. Downloaders must be
#      able to run `shasum -a 256 -c SHA256SUMS.txt` in whatever directory
#      they saved the files to.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init
require_cmd shasum

[[ -d "$DIST_DIR" ]] || die "no dist directory: $DIST_DIR"

step "Checksums for $RELEASE_VERSION"

# Collect the artifacts users receive. Intermediates and metadata are excluded.
ARTIFACTS=()
while IFS= read -r _a; do
  [[ -n "$_a" ]] && ARTIFACTS+=("$_a")
done < <(
  cd "$DIST_DIR" && find . -maxdepth 1 -type f \
    \( -name '*.dmg' -o -name '*.pkg' -o -name '*.zip' \) \
    ! -name '*-for-notarization.zip' \
    -exec basename {} \; | sort
)

[[ ${#ARTIFACTS[@]} -gt 0 ]] || die "no distributable artifacts found in $DIST_DIR"

# --- finalisation gate -----------------------------------------------------

step "Confirm artifacts are finalised before hashing"

if [[ "$RELEASE_MODE" == "public" ]]; then
  require_macos
  for a in "${ARTIFACTS[@]}"; do
    case "$a" in
      *.dmg|*.pkg)
        xcrun stapler validate "$DIST_DIR/$a" >/dev/null 2>&1 \
          || die "$a has no valid notarization ticket stapled. Checksums must not be generated before stapling."
        ok "$a: notarization ticket stapled and valid"
        ;;
    esac
  done
else
  warn "RELEASE_MODE=$RELEASE_MODE — artifacts are not notarized, so no staple check is possible."
  warn "These checksums describe a local rehearsal artifact, not a release."
fi

# --- generate --------------------------------------------------------------

step "Write $CHECKSUM_NAME"

rm -f "$CHECKSUM_PATH"
(
  cd "$DIST_DIR"
  # `cd` first so shasum records bare basenames, never a path prefix.
  shasum -a 256 "${ARTIFACTS[@]}" >"$CHECKSUM_NAME"
)

# Prove the rule rather than trusting it: no entry may contain a path
# separator, and none may reference this machine.
while IFS= read -r line; do
  name="${line#* }"
  name="${name# }"
  name="${name#\*}"
  [[ "$name" != */* ]] || die "checksum entry contains a path, not a basename: '$line'"
  [[ "$name" != "$ROOT"* ]] || die "checksum entry leaks an absolute path: '$line'"
done <"$CHECKSUM_PATH"

sed 's/^/      /' "$CHECKSUM_PATH"
ok "${#ARTIFACTS[@]} artifact(s) hashed with basenames only"

# --- prove it verifies from a directory that is not the build directory ----

step "Verify checksums from a fresh directory"

VERIFY_DIR="$(mktemp -d -t sendbloom-cksum.XXXXXX)"
trap 'rm -rf "$VERIFY_DIR"' EXIT
for a in "${ARTIFACTS[@]}"; do
  cp "$DIST_DIR/$a" "$VERIFY_DIR/$a"
done
cp "$CHECKSUM_PATH" "$VERIFY_DIR/$CHECKSUM_NAME"

(cd "$VERIFY_DIR" && shasum -a 256 -c "$CHECKSUM_NAME") | sed 's/^/      /'
ok "checksum file verifies after the artifacts are moved elsewhere"

echo
printf '%schecksums: PASS%s  %s\n' "$_c_grn" "$_c_off" "$CHECKSUM_PATH"
