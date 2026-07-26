#!/usr/bin/env bash
# Validate the artifact a user actually receives.
#
#   scripts/release/validate-artifacts.sh [artifact-dir]
#
# Defaults to the local dist directory. Point it at a directory of freshly
# downloaded files to revalidate a hosted release — the checks are identical
# by design, which is the whole point: "it was fine locally" is not evidence.
#
# What it proves for each artifact:
#   * the checksum file verifies, from this directory, using basenames
#   * the recorded hash matches the manifest
#   * the signature is a Developer ID signature (public mode)
#   * the notarization ticket is stapled and validates (public mode)
#   * Gatekeeper accepts it (public mode)
#   * the disk image mounts and its visible layout matches the contract
#   * the inner installer or plug-in bundles carry the expected version,
#     bundle id, architectures, signature and staple
#
# Nothing here inspects the build directory. Only the shipped container.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd shasum hdiutil plutil python3

ARTIFACT_DIR="${1:-$DIST_DIR}"
ARTIFACT_DIR="$(cd "$ARTIFACT_DIR" && pwd)"
MANIFEST_IN="${MANIFEST_IN:-$ARTIFACT_DIR/$MANIFEST_NAME}"

failures=0
fail() { printf '%sFAIL%s %s\n' "$_c_red" "$_c_off" "$*" >&2; failures=$((failures + 1)); }

step "Validate artifacts in $ARTIFACT_DIR"
info "contract: $ARTIFACT_CONTRACT   mode: $RELEASE_MODE   version: $RELEASE_VERSION"

MOUNTPOINT=""
cleanup() {
  if [[ -n "$MOUNTPOINT" && -d "$MOUNTPOINT" ]]; then
    hdiutil detach "$MOUNTPOINT" -quiet -force >/dev/null 2>&1 || true
  fi
}
trap cleanup EXIT

# ---------------------------------------------------------------------------
step "Expected assets are present"
# ---------------------------------------------------------------------------

EXPECTED=("$DMG_NAME" "$CHECKSUM_NAME")
for e in "${EXPECTED[@]}"; do
  if [[ -f "$ARTIFACT_DIR/$e" ]]; then
    ok "present: $e"
  else
    fail "missing expected asset: $e"
  fi
done

# Anything version-shaped that is NOT this version is a leftover from an older
# release and must not sit next to the new one.
while IFS= read -r stray; do
  base="$(basename "$stray")"
  case "$base" in
    *"$RELEASE_VERSION"*) continue ;;
    "$CHECKSUM_NAME"|"$MANIFEST_NAME"|"$PROVENANCE_NAME"|notarization.txt|build-environment.txt|release-report.md) continue ;;
  esac
  if [[ "$base" =~ [0-9]+\.[0-9]+\.[0-9]+ ]]; then
    fail "obsolete artifact from another release is present: $base"
  fi
done < <(find "$ARTIFACT_DIR" -maxdepth 1 -type f)

# ---------------------------------------------------------------------------
step "Checksums verify from this directory"
# ---------------------------------------------------------------------------

if [[ -f "$ARTIFACT_DIR/$CHECKSUM_NAME" ]]; then
  while IFS= read -r line; do
    name="${line#* }"; name="${name# }"; name="${name#\*}"
    [[ "$name" != */* ]] || fail "checksum entry is not a basename: '$line'"
  done <"$ARTIFACT_DIR/$CHECKSUM_NAME"

  if (cd "$ARTIFACT_DIR" && shasum -a 256 -c "$CHECKSUM_NAME" >/dev/null 2>&1); then
    ok "$CHECKSUM_NAME verifies"
  else
    (cd "$ARTIFACT_DIR" && shasum -a 256 -c "$CHECKSUM_NAME" 2>&1 | sed 's/^/      /') >&2 || true
    fail "$CHECKSUM_NAME does not verify in $ARTIFACT_DIR"
  fi
else
  fail "no $CHECKSUM_NAME to verify"
fi

# ---------------------------------------------------------------------------
step "Hashes match the manifest"
# ---------------------------------------------------------------------------

if [[ -f "$MANIFEST_IN" ]]; then
  compared=0
  while IFS=$'\t' read -r m_name m_sha; do
    [[ -n "$m_name" ]] || continue
    if [[ ! -f "$ARTIFACT_DIR/$m_name" ]]; then
      fail "manifest lists $m_name but it is not in $ARTIFACT_DIR"
      continue
    fi
    actual="$(sha256_of "$ARTIFACT_DIR/$m_name")"
    compared=$((compared + 1))
    if [[ "$actual" == "$m_sha" ]]; then
      ok "$m_name matches the manifest hash"
    else
      fail "$m_name hash mismatch: manifest=$m_sha actual=$actual"
    fi
  done < <(python3 -c '
import json, sys
m = json.load(open(sys.argv[1]))
for a in m.get("artifacts", []):
    print(a["name"] + "\t" + a["sha256"])
' "$MANIFEST_IN")

  [[ "$compared" -gt 0 ]] || fail "the manifest listed no artifacts; nothing was actually compared"

  m_version="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["release_version"])' "$MANIFEST_IN")"
  m_commit="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source_commit"])' "$MANIFEST_IN")"
  [[ "$m_version" == "$RELEASE_VERSION" ]] \
    && ok "manifest release_version = $m_version" \
    || fail "manifest release_version=$m_version, expected $RELEASE_VERSION"
  info "manifest source_commit = $m_commit"
else
  fail "no manifest at $MANIFEST_IN — cannot compare recorded hashes"
fi

# ---------------------------------------------------------------------------
step "Disk image signature, notarization and Gatekeeper"
# ---------------------------------------------------------------------------

DMG_IN="$ARTIFACT_DIR/$DMG_NAME"

if [[ -f "$DMG_IN" ]]; then
  if [[ "$RELEASE_MODE" == "public" ]]; then
    if codesign --verify --strict --verbose=2 "$DMG_IN" >/dev/null 2>&1; then
      ok "disk image signature verifies"
    else
      fail "disk image signature does not verify"
    fi

    dmg_info="$(codesign --display --verbose=4 "$DMG_IN" 2>&1)"
    codesign_is_adhoc "$dmg_info" && fail "disk image is ad-hoc signed"
    codesign_authority_is_developer_id_application "$dmg_info" \
      && ok "authority: $(grep -m1 '^Authority=' <<<"$dmg_info")" \
      || fail "disk image is not Developer ID Application signed"

    if xcrun stapler validate "$DMG_IN" >/dev/null 2>&1; then
      ok "notarization ticket stapled and valid"
    else
      fail "disk image has no valid stapled notarization ticket"
    fi

    if spctl --assess --type open --context context:primary-signature "$DMG_IN" >/dev/null 2>&1; then
      ok "Gatekeeper accepts the disk image"
    else
      spctl --assess --type open --context context:primary-signature --verbose=4 "$DMG_IN" 2>&1 | sed 's/^/      /' >&2 || true
      fail "Gatekeeper rejects the disk image"
    fi
  else
    warn "RELEASE_MODE=$RELEASE_MODE — signature, notarization and Gatekeeper checks are NOT performed"
    warn "this artifact is a local rehearsal build and would be refused on another machine"
  fi
else
  fail "no disk image at $DMG_IN"
fi

# ---------------------------------------------------------------------------
step "Mount the disk image and inspect what a user sees"
# ---------------------------------------------------------------------------

if [[ -f "$DMG_IN" ]]; then
  MOUNTPOINT="$(mktemp -d -t sendbloom-dmg.XXXXXX)"
  hdiutil attach "$DMG_IN" -mountpoint "$MOUNTPOINT" -nobrowse -readonly -quiet \
    || die "could not mount $DMG_IN"
  ok "mounted at $MOUNTPOINT"

  info "visible contents:"
  (cd "$MOUNTPOINT" && ls -la) | sed 's/^/      /'

  for required in INSTALL.txt LICENSE.txt THIRD_PARTY_LICENSES.txt RELEASE_NOTES.md; do
    [[ -f "$MOUNTPOINT/$required" ]] \
      && ok "disk image contains $required" \
      || fail "disk image is missing $required"
  done

  grep -q "$RELEASE_VERSION" "$MOUNTPOINT/INSTALL.txt" 2>/dev/null \
    && ok "INSTALL.txt names $RELEASE_VERSION" \
    || fail "INSTALL.txt does not name $RELEASE_VERSION"

  # --- contract-specific layout -------------------------------------------
  if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
    INNER_PKG="$MOUNTPOINT/$PKG_NAME"
    if [[ -f "$INNER_PKG" ]]; then
      ok "disk image contains the installer: $PKG_NAME"

      if [[ "$RELEASE_MODE" == "public" ]]; then
        if pkgutil --check-signature "$INNER_PKG" 2>/dev/null | grep -q 'Developer ID Installer'; then
          ok "installer is Developer ID Installer signed"
        else
          fail "installer is not Developer ID Installer signed"
        fi
        xcrun stapler validate "$INNER_PKG" >/dev/null 2>&1 \
          && ok "installer notarization ticket stapled and valid" \
          || fail "installer has no valid stapled notarization ticket"
        spctl --assess --type install "$INNER_PKG" >/dev/null 2>&1 \
          && ok "Gatekeeper accepts the installer as an install source" \
          || fail "Gatekeeper rejects the installer"
      fi

      step "Installer payload (expanded, nothing written to the system)"
      EXPAND_DIR="$(mktemp -d -t sendbloom-pkg.XXXXXX)"
      rm -rf "$EXPAND_DIR"
      pkgutil --expand-full "$INNER_PKG" "$EXPAND_DIR" >/dev/null 2>&1 \
        || fail "could not expand the installer payload"

      if [[ -d "$EXPAND_DIR" ]]; then
        payload_vst3="$(find "$EXPAND_DIR" -name 'SendBloom.vst3' -maxdepth 6 -print -quit)"
        payload_au="$(find "$EXPAND_DIR" -name 'SendBloom.component' -maxdepth 6 -print -quit)"
        [[ -n "$payload_vst3" ]] && ok "payload contains SendBloom.vst3" || fail "installer payload has no SendBloom.vst3"
        [[ -n "$payload_au" ]]   && ok "payload contains SendBloom.component" || fail "installer payload has no SendBloom.component"

        for b in "$payload_vst3" "$payload_au"; do
          [[ -n "$b" && -f "$b/Contents/Info.plist" ]] || continue
          v="$(plutil -extract CFBundleShortVersionString raw -o - "$b/Contents/Info.plist" 2>/dev/null || echo '<missing>')"
          [[ "$v" == "$VERSION_CORE" ]] \
            && ok "$(basename "$b") in payload reports version $v" \
            || fail "$(basename "$b") in payload reports version $v, expected $VERSION_CORE"
        done

        # Confirm the install locations the user will actually get.
        for loc in "$VST3_INSTALL_DIR" "$AU_INSTALL_DIR"; do
          if grep -Rqs "install-location=\"$loc\"\|<install-location>$loc" "$EXPAND_DIR" \
            || find "$EXPAND_DIR" -name PackageInfo -exec grep -l "install-location=\"$loc\"" {} + >/dev/null 2>&1; then
            ok "installer targets $loc"
          else
            warn "could not confirm install location $loc from the expanded payload"
          fi
        done
        rm -rf "$EXPAND_DIR"
      fi
    else
      fail "contract is pkg-in-dmg but the disk image contains no $PKG_NAME"
    fi
  else
    for rel in "VST3/SendBloom.vst3" "Components/SendBloom.component"; do
      b="$MOUNTPOINT/$rel"
      if [[ -d "$b" ]]; then
        ok "disk image contains $rel"
        v="$(plutil -extract CFBundleShortVersionString raw -o - "$b/Contents/Info.plist" 2>/dev/null || echo '<missing>')"
        i="$(plutil -extract CFBundleIdentifier raw -o - "$b/Contents/Info.plist" 2>/dev/null || echo '<missing>')"
        [[ "$v" == "$VERSION_CORE" ]] && ok "  version $v" || fail "  $rel reports version $v, expected $VERSION_CORE"
        [[ "$i" == "$BUNDLE_ID" ]]    && ok "  id $i"      || fail "  $rel reports id $i, expected $BUNDLE_ID"

        archs="$(lipo -archs "$b/Contents/MacOS/SendBloom" 2>/dev/null | tr ' ' '\n' | sort | paste -sd+ - || echo unknown)"
        info "  architectures: $archs"

        if [[ "$RELEASE_MODE" == "public" ]]; then
          if codesign --verify --deep --strict "$b" >/dev/null 2>&1; then
            binfo="$(codesign --display --verbose=4 "$b" 2>&1)"
            codesign_authority_is_developer_id_application "$binfo" \
              && ok "  Developer ID Application signed" \
              || fail "  $rel is not Developer ID Application signed"
            codesign_has_hardened_runtime "$binfo" \
              && ok "  hardened runtime present" \
              || fail "  $rel lacks the hardened runtime"
          else
            fail "  $rel signature does not verify"
          fi
          xcrun stapler validate "$b" >/dev/null 2>&1 \
            && ok "  notarization ticket stapled" \
            || fail "  $rel has no stapled notarization ticket"
        fi
      else
        fail "contract is bundles-in-dmg but $rel is missing from the disk image"
      fi
    done
  fi

  hdiutil detach "$MOUNTPOINT" -quiet -force >/dev/null 2>&1 || true
  MOUNTPOINT=""
  ok "unmounted"
fi

# ---------------------------------------------------------------------------

echo
if [[ "$failures" -ne 0 ]]; then
  die "artifact validation: $failures failure(s) in $ARTIFACT_DIR"
fi
printf '%svalidate-artifacts: PASS%s  %s\n' "$_c_grn" "$_c_off" "$ARTIFACT_DIR"
