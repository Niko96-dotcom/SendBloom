#!/usr/bin/env bash
# Submit an artifact for notarization, wait for the verdict, then staple.
#
#   scripts/release/notarize-macos.sh <artifact> [staple-target ...]
#
# <artifact>       .zip, .pkg or .dmg to submit.
# [staple-target]  Optional paths to staple instead of the artifact. Used when
#                  submitting a .zip of plugin bundles: the zip itself cannot
#                  be stapled, but the bundles inside it can.
#
# Failure of any kind — missing credentials, "Invalid", "Rejected", a staple
# that does not validate — exits non-zero. There is no partial success here:
# an unnotarized artifact must never reach checksums or publication.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init

ARTIFACT="${1:?usage: notarize-macos.sh <artifact.zip|pkg|dmg> [staple-target ...]}"
shift || true
STAPLE_TARGETS=()
while [[ $# -gt 0 ]]; do STAPLE_TARGETS+=("$1"); shift; done

[[ -e "$ARTIFACT" ]] || die "artifact not found: $ARTIFACT"

if [[ "$RELEASE_MODE" != "public" ]]; then
  die "notarization requires RELEASE_MODE=public. A local-unsigned rehearsal must not submit to Apple."
fi

require_notary_profile
require_cmd xcrun plutil

case "$ARTIFACT" in
  *.zip|*.pkg|*.dmg) ;;
  *) die "notarization input must be .zip, .pkg or .dmg, got: $ARTIFACT" ;;
esac

step "Notarize $(basename "$ARTIFACT")"
info "profile: \$NOTARY_PROFILE (value not printed)"
info "sha256:  $(sha256_of "$ARTIFACT")"

RESULT_JSON="$(mktemp -t sendbloom-notary.XXXXXX)"
trap 'rm -f "$RESULT_JSON"' EXIT

set +e
xcrun notarytool submit "$ARTIFACT" \
  --keychain-profile "$NOTARY_PROFILE" \
  --wait \
  --output-format json >"$RESULT_JSON" 2>"$RESULT_JSON.err"
submit_rc=$?
set -e

if [[ "$submit_rc" -ne 0 ]]; then
  cat "$RESULT_JSON" "$RESULT_JSON.err" >&2 || true
  rm -f "$RESULT_JSON.err"
  die "notarytool submit failed (exit $submit_rc)"
fi
rm -f "$RESULT_JSON.err"

STATUS="$(plutil -extract status raw -o - -- "$RESULT_JSON" 2>/dev/null || echo '<unparseable>')"
SUBMISSION_ID="$(plutil -extract id raw -o - -- "$RESULT_JSON" 2>/dev/null || echo '<unknown>')"

info "submission: $SUBMISSION_ID"
info "status:     $STATUS"

if [[ "$STATUS" != "Accepted" ]]; then
  warn "fetching the notarization log for $SUBMISSION_ID"
  xcrun notarytool log "$SUBMISSION_ID" --keychain-profile "$NOTARY_PROFILE" >&2 || true
  die "notarization finished with status '$STATUS' — release blocked"
fi
ok "notarization Accepted ($SUBMISSION_ID)"

# --- staple ----------------------------------------------------------------

if [[ ${#STAPLE_TARGETS[@]} -eq 0 ]]; then
  case "$ARTIFACT" in
    *.zip)
      die "a .zip cannot be stapled; pass the bundle paths to staple as extra arguments"
      ;;
    *) STAPLE_TARGETS=("$ARTIFACT") ;;
  esac
fi

step "Staple and validate"
for target in "${STAPLE_TARGETS[@]}"; do
  [[ -e "$target" ]] || die "staple target not found: $target"
  info "stapling $target"
  xcrun stapler staple "$target" || die "stapler staple failed for $target"
  xcrun stapler validate "$target" || die "stapler validate failed for $target"
  ok "stapled and validated: $target"
done

# Record the notarization fact for the manifest. The submission id is not a
# credential; the profile and its contents are never written.
NOTARY_RECORD_DIR="${NOTARY_RECORD_DIR:-$DIST_DIR}"
mkdir -p "$NOTARY_RECORD_DIR"
{
  printf 'artifact=%s\n' "$(basename "$ARTIFACT")"
  printf 'artifact_sha256=%s\n' "$(sha256_of "$ARTIFACT")"
  printf 'submission_id=%s\n' "$SUBMISSION_ID"
  printf 'status=%s\n' "$STATUS"
  printf 'stapled=%s\n' "$(for _t in "${STAPLE_TARGETS[@]}"; do printf '%s ' "$(basename "$_t")"; done | xargs)"
  printf 'notarized_at=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} >>"$NOTARY_RECORD_DIR/notarization.txt"

echo
printf '%snotarize-macos: PASS%s %s\n' "$_c_grn" "$_c_off" "$(basename "$ARTIFACT")"
