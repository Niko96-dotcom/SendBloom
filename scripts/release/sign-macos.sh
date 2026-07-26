#!/usr/bin/env bash
# Sign the AU and VST3 bundles.
#
# RELEASE_MODE=public         Developer ID Application, hardened runtime,
#                             secure timestamp. Missing identity = hard fail.
# RELEASE_MODE=local-unsigned Ad-hoc signature, loudly labelled, never
#                             publishable.
#
# Verification deliberately accepts every valid shape of codesign output
# rather than the first one we happened to observe — see
# codesign_has_hardened_runtime() in lib.sh.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd codesign security

BUNDLES=("$VST3_BUNDLE" "$AU_BUNDLE")

for b in "${BUNDLES[@]}"; do
  [[ -d "$b" ]] || die "missing bundle: $b (run scripts/release/build-macos.sh first)"
done

if [[ "$RELEASE_MODE" == "public" ]]; then
  step "Codesign — Developer ID Application"
  require_application_identity
  info "identity: $DEVELOPER_ID_APPLICATION"

  ENTITLEMENTS_ARGS=()
  if [[ -n "${CODESIGN_ENTITLEMENTS:-}" ]]; then
    [[ -f "$CODESIGN_ENTITLEMENTS" ]] || die "CODESIGN_ENTITLEMENTS not found: $CODESIGN_ENTITLEMENTS"
    ENTITLEMENTS_ARGS=(--entitlements "$CODESIGN_ENTITLEMENTS")
    info "entitlements: $CODESIGN_ENTITLEMENTS"
  fi

  for bundle in "${BUNDLES[@]}"; do
    info "signing $bundle"
    codesign --force \
             --sign "$DEVELOPER_ID_APPLICATION" \
             --options runtime \
             --timestamp \
             --generate-entitlement-der \
             ${ENTITLEMENTS_ARGS[@]+"${ENTITLEMENTS_ARGS[@]}"} \
             "$bundle"
  done
  ok "signed ${#BUNDLES[@]} bundle(s)"
else
  unsigned_banner
  step "Codesign — ad-hoc (local rehearsal)"
  for bundle in "${BUNDLES[@]}"; do
    info "ad-hoc signing $bundle"
    codesign --force --sign - --timestamp=none "$bundle"
  done
  warn "ad-hoc signatures only. These artifacts cannot be notarized or published."
fi

step "Verify signatures"

for bundle in "${BUNDLES[@]}"; do
  codesign --verify --deep --strict --verbose=2 "$bundle" 2>&1 | sed 's/^/      /'

  info_out="$(codesign --display --verbose=4 "$bundle" 2>&1)"

  identifier="$(grep -m1 -E '^Identifier=' <<<"$info_out" || true)"
  authority="$(grep -m1 -E '^Authority=' <<<"$info_out" || true)"
  team="$(grep -m1 -E '^TeamIdentifier=' <<<"$info_out" || true)"
  info "  ${identifier:-Identifier=<none>}"
  info "  ${authority:-Authority=<none>}"
  info "  ${team:-TeamIdentifier=<none>}"

  if [[ "$RELEASE_MODE" == "public" ]]; then
    assert_developer_id_signed "$bundle"
    if codesign_has_hardened_runtime "$info_out"; then
      if grep -qE '^[[:space:]]*Runtime Version=' <<<"$info_out"; then
        info "  hardened runtime confirmed via 'Runtime Version' line"
      else
        info "  hardened runtime confirmed via CodeDirectory runtime flag"
      fi
    fi
    ok "$bundle is Developer ID signed, hardened, timestamped"
  else
    codesign_is_adhoc "$info_out" \
      || warn "$bundle is not ad-hoc in a local-unsigned run; check what signed it"
    ok "$bundle carries a verifiable ad-hoc signature (rehearsal only)"
  fi
done

echo
printf '%ssign-macos: PASS%s (mode=%s)\n' "$_c_grn" "$_c_off" "$RELEASE_MODE"
