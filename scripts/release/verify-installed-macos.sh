#!/usr/bin/env bash
# Installed truth.
#
# The repo build output is not evidence that a user has the new version. This
# inspects what is actually installed on this machine, going to the source of
# truth in each case rather than to a cached index:
#
#   * the installed bundle Info.plist (not `mdls`, not Spotlight, not the
#     AudioComponent registry cache — those go stale and will happily report
#     a version that is no longer on disk)
#   * the installed binary's architectures
#   * the code signature and stapled ticket on the installed copy
#   * the installer receipt, when the pkg contract was used
#   * whether the AU host layer can actually see and instantiate the
#     component (auval), which is the only check that proves a host agrees
#
# Read-only. It installs nothing. Use install-smoke-macos.sh for that.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd plutil

RUN_AUVAL="${RUN_AUVAL:-1}"
failures=0
skipped=0
fail() { printf '%sFAIL%s %s\n' "$_c_red" "$_c_off" "$*" >&2; failures=$((failures + 1)); }
skip() { printf '%sSKIP%s %s\n' "$_c_yel" "$_c_off" "$*" >&2; skipped=$((skipped + 1)); }

step "Installed SendBloom on this machine — expecting $RELEASE_VERSION ($BUILD_ID)"

INSTALLED_VST3="$VST3_INSTALL_DIR/SendBloom.vst3"
INSTALLED_AU="$AU_INSTALL_DIR/SendBloom.component"

EXPECTED_HASHES=""
if [[ -f "$MANIFEST_PATH" ]]; then
  info "manifest: $MANIFEST_PATH"
fi

check_installed_bundle() {
  local label="$1" bundle="$2"

  step "$label — $bundle"

  if [[ ! -d "$bundle" ]]; then
    skip "$label is not installed at $bundle"
    return
  fi

  local plist="$bundle/Contents/Info.plist"
  [[ -f "$plist" ]] || { fail "$label has no Info.plist"; return; }

  local short ident
  short="$(plutil -extract CFBundleShortVersionString raw -o - "$plist" 2>/dev/null || echo '<missing>')"
  ident="$(plutil -extract CFBundleIdentifier raw -o - "$plist" 2>/dev/null || echo '<missing>')"

  info "installed at: $(stat -f '%Sm' -t '%Y-%m-%d %H:%M:%S' "$bundle")"
  if [[ "$short" == "$VERSION_CORE" ]]; then
    ok "Info.plist reports $short — matches the release"
  else
    fail "Info.plist reports $short but the release is $VERSION_CORE. An older build is still installed."
  fi
  [[ "$ident" == "$BUNDLE_ID" ]] && ok "bundle id $ident" || fail "bundle id $ident, expected $BUNDLE_ID"

  local bin="$bundle/Contents/MacOS/SendBloom"
  if [[ -f "$bin" ]]; then
    local archs
    archs="$(lipo -archs "$bin" | tr ' ' '\n' | sort | paste -sd+ -)"
    ok "architectures: $archs"
    info "binary sha256: $(sha256_of "$bin")"
  else
    fail "$label has no executable at Contents/MacOS/SendBloom"
  fi

  if [[ "$RELEASE_MODE" == "public" ]]; then
    if codesign --verify --deep --strict "$bundle" >/dev/null 2>&1; then
      local binfo
      binfo="$(codesign --display --verbose=4 "$bundle" 2>&1)"
      codesign_authority_is_developer_id_application "$binfo" \
        && ok "$(grep -m1 '^Authority=' <<<"$binfo")" \
        || fail "installed $label is not Developer ID signed"
      codesign_has_hardened_runtime "$binfo" \
        && ok "hardened runtime present" \
        || fail "installed $label lacks the hardened runtime"
    else
      fail "installed $label signature does not verify"
    fi

    if xcrun stapler validate "$bundle" >/dev/null 2>&1; then
      ok "notarization ticket stapled to the installed copy"
    else
      fail "installed $label has no valid stapled notarization ticket"
    fi
  else
    skip "signature and notarization checks (RELEASE_MODE=$RELEASE_MODE)"
  fi
}

check_installed_bundle "VST3" "$INSTALLED_VST3"
check_installed_bundle "Audio Unit" "$INSTALLED_AU"

# ---------------------------------------------------------------------------
step "Installer receipts"
# ---------------------------------------------------------------------------

if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  for pkgid in "$BUNDLE_ID.vst3" "$BUNDLE_ID.au"; do
    if pkgutil --pkg-info "$pkgid" >/dev/null 2>&1; then
      receipt_version="$(pkgutil --pkg-info "$pkgid" | awk '/^version:/ {print $2}')"
      if [[ "$receipt_version" == "$VERSION_CORE" ]]; then
        ok "receipt $pkgid at version $receipt_version"
      else
        fail "receipt $pkgid reports $receipt_version, expected $VERSION_CORE"
      fi
      pkgutil --pkg-info "$pkgid" | sed 's/^/      /'
    else
      skip "no installer receipt for $pkgid (not installed via the pkg, or never installed)"
    fi
  done
else
  info "contract is bundles-in-dmg; no installer receipts are expected"
fi

# ---------------------------------------------------------------------------
step "Does the AU host layer see it? (auval)"
# ---------------------------------------------------------------------------

# Manufacturer NkMo / plugin SbLm, from CMakeLists.txt. Read the AU type from
# the installed component: MIDI-capable effects are `aumf`, while ordinary
# effects are `aufx`. The bundle metadata is the source of truth here.
if [[ "$RUN_AUVAL" != "1" ]]; then
  skip "auval disabled by RUN_AUVAL=$RUN_AUVAL"
elif [[ ! -d "$INSTALLED_AU" ]]; then
  skip "auval — the Audio Unit is not installed"
elif ! command -v auval >/dev/null 2>&1; then
  skip "auval is not available on this system"
else
  au_type="$(plutil -extract 'AudioComponents.0.type' raw -o - \
    "$INSTALLED_AU/Contents/Info.plist" 2>/dev/null || true)"
  if [[ ! "$au_type" =~ ^[[:alnum:]]{4}$ ]]; then
    fail "installed Audio Unit has no valid four-character type in Info.plist"
    au_type=""
  fi

  # The AudioComponent registry is a cache. Ask for the component directly and
  # treat a miss as a real finding rather than re-reading the cached list.
  if [[ -n "$au_type" ]] && auval -v "$au_type" SbLm NkMo >/tmp/sendbloom-auval.$$.log 2>&1; then
    ok "auval validated $au_type SbLm NkMo"
    grep -E 'AU Validation|PASS|Version' /tmp/sendbloom-auval.$$.log | head -20 | sed 's/^/      /' || true
  else
    tail -40 /tmp/sendbloom-auval.$$.log | sed 's/^/      /' >&2 || true
    fail "auval could not validate ${au_type:-<missing-type>} SbLm NkMo — the host layer does not accept the installed component"
  fi
  rm -f /tmp/sendbloom-auval.$$.log
fi

# ---------------------------------------------------------------------------
step "Live process check"
# ---------------------------------------------------------------------------

if pgrep -q -f AudioComponentRegistrar 2>/dev/null; then
  info "AudioComponentRegistrar is running; if a host was open during install, restart it before trusting what it lists"
fi

running_hosts="$(pgrep -l -f 'Logic Pro|Cubase|REAPER|Ableton|Studio One|Pro Tools' 2>/dev/null || true)"
if [[ -n "$running_hosts" ]]; then
  warn "DAW hosts are running now; they may hold an older copy of the plug-in in memory:"
  printf '%s\n' "$running_hosts" | sed 's/^/      /' >&2
fi

# ---------------------------------------------------------------------------

echo
printf 'installed checks: %d failure(s), %d skipped\n' "$failures" "$skipped"
if [[ "$failures" -ne 0 ]]; then
  die "installed product does not match the release"
fi
if [[ "$skipped" -ne 0 ]]; then
  warn "$skipped check(s) were SKIPPED. Skipped is not passed — name them in the release report."
fi
printf '%sverify-installed-macos: PASS%s (%d skipped)\n' "$_c_grn" "$_c_off" "$skipped"
