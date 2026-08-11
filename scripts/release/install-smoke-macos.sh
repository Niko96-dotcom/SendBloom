#!/usr/bin/env bash
# Destructive install smoke test — writes to system plug-in locations.
#
# This is the only script in the release system that changes the machine it
# runs on. It is therefore opt-in twice: you must pass --i-understand AND set
# SENDBLOOM_INSTALL_SMOKE=1. Without both it explains itself and exits 0
# having done nothing, so it is safe to leave wired into the pipeline.
#
# What it does, in order:
#   1. records what is currently installed (so an upgrade is a real upgrade)
#   2. installs the release artifact the way a user would
#   3. runs verify-installed-macos.sh against the result
#   4. optionally uninstalls again and proves the removal was complete
#
# Run it on a release machine you are willing to modify. Never in CI on a
# shared runner without understanding what it leaves behind.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init

MODE="install"
CONFIRMED=0
for arg in "$@"; do
  case "$arg" in
    --i-understand) CONFIRMED=1 ;;
    --uninstall)    MODE="uninstall" ;;
    --upgrade)      MODE="upgrade" ;;
    *) die "unknown argument: $arg (expected --i-understand, --uninstall, --upgrade)" ;;
  esac
done

if [[ "$CONFIRMED" != "1" || "${SENDBLOOM_INSTALL_SMOKE:-0}" != "1" ]]; then
  cat <<EOF

install-smoke-macos.sh did NOT run. This is the intended default.

It writes to:
  $VST3_INSTALL_DIR/SendBloom.vst3
  $AU_INSTALL_DIR/SendBloom.component
and, with the pkg contract, creates installer receipts.

To run it deliberately on a machine you are willing to modify:

  SENDBLOOM_INSTALL_SMOKE=1 scripts/release/install-smoke-macos.sh --i-understand

Add --upgrade to test installing over an existing copy, or --uninstall to
remove SendBloom and verify the removal was complete.

Because this run did nothing, install smoke is a SKIPPED check. Record it as
skipped in the release report — not as passed.

EOF
  exit 0
fi

require_cmd hdiutil ditto sudo

INSTALLED_BEFORE=""
if [[ -d "$VST3_INSTALL_DIR/SendBloom.vst3" ]]; then
  INSTALLED_BEFORE="$(plutil -extract CFBundleShortVersionString raw -o - \
    "$VST3_INSTALL_DIR/SendBloom.vst3/Contents/Info.plist" 2>/dev/null || echo unknown)"
fi

step "Install smoke — mode: $MODE"
info "already installed: ${INSTALLED_BEFORE:-<nothing>}"
info "release under test: $RELEASE_VERSION ($BUILD_ID)"

if [[ "$MODE" == "upgrade" && -z "$INSTALLED_BEFORE" ]]; then
  die "--upgrade requires an existing installation to upgrade from; nothing is installed"
fi

# ---------------------------------------------------------------------------
uninstall_sendbloom() {
  step "Uninstall"
  sudo rm -rf "$VST3_INSTALL_DIR/SendBloom.vst3"
  sudo rm -rf "$AU_INSTALL_DIR/SendBloom.component"
  for pkgid in "$BUNDLE_ID.vst3" "$BUNDLE_ID.au"; do
    if pkgutil --pkg-info "$pkgid" >/dev/null 2>&1; then
      sudo pkgutil --forget "$pkgid" >/dev/null
      ok "forgot receipt $pkgid"
    fi
  done

  local leftovers=0
  [[ -e "$VST3_INSTALL_DIR/SendBloom.vst3" ]] && { fail_msg="VST3 still present"; leftovers=1; }
  [[ -e "$AU_INSTALL_DIR/SendBloom.component" ]] && { fail_msg="AU still present"; leftovers=1; }
  for pkgid in "$BUNDLE_ID.vst3" "$BUNDLE_ID.au"; do
    pkgutil --pkg-info "$pkgid" >/dev/null 2>&1 && { fail_msg="receipt $pkgid still present"; leftovers=1; }
  done
  [[ "$leftovers" == "0" ]] || die "uninstall was incomplete: ${fail_msg:-unknown leftover}"
  ok "uninstall complete: no bundles, no receipts"
}

if [[ "$MODE" == "uninstall" ]]; then
  uninstall_sendbloom
  echo
  printf '%sinstall-smoke (uninstall): PASS%s\n' "$_c_grn" "$_c_off"
  exit 0
fi

# ---------------------------------------------------------------------------
step "Install from the shipped artifact"

[[ -f "$DMG_PATH" ]] || die "no disk image to install from: $DMG_PATH"

# A stable VST3 class ID is required for session compatibility. Before adding
# another copy in a default scan location, require the candidate to carry a
# strictly newer version than every installed SendBloom duplicate. This does
# not claim how a particular host resolves duplicates; that remains a DAW
# observation after installation.
bash "$(dirname "${BASH_SOURCE[0]}")/check-duplicate-vst3-versions-macos.sh" "$VST3_BUNDLE"

MOUNTPOINT="$(mktemp -d -t sendbloom-install.XXXXXX)"
cleanup() { hdiutil detach "$MOUNTPOINT" -quiet -force >/dev/null 2>&1 || true; }
trap cleanup EXIT

hdiutil attach "$DMG_PATH" -mountpoint "$MOUNTPOINT" -nobrowse -readonly -quiet
ok "mounted $(basename "$DMG_PATH")"

if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  [[ -f "$MOUNTPOINT/$PKG_NAME" ]] || die "no installer inside the disk image"
  info "running the installer exactly as a user would (requires admin)"
  sudo installer -pkg "$MOUNTPOINT/$PKG_NAME" -target / -verbose
  ok "installer completed"
else
  sudo mkdir -p "$VST3_INSTALL_DIR" "$AU_INSTALL_DIR"
  sudo ditto "$MOUNTPOINT/VST3/SendBloom.vst3" "$VST3_INSTALL_DIR/SendBloom.vst3"
  sudo ditto "$MOUNTPOINT/Components/SendBloom.component" "$AU_INSTALL_DIR/SendBloom.component"
  ok "copied both plug-ins into the system locations"
fi

cleanup
trap - EXIT

# ---------------------------------------------------------------------------
step "Verify what is now installed"
bash "$(dirname "${BASH_SOURCE[0]}")/verify-installed-macos.sh"

if [[ "$MODE" == "upgrade" ]]; then
  step "Upgrade check"
  now="$(plutil -extract CFBundleShortVersionString raw -o - \
    "$VST3_INSTALL_DIR/SendBloom.vst3/Contents/Info.plist")"
  [[ "$now" == "$VERSION_CORE" ]] \
    || die "upgrade did not take: still reporting $now"
  ok "upgraded $INSTALLED_BEFORE -> $now with no leftover of the old version"
fi

echo
printf '%sinstall-smoke: PASS%s (mode=%s, machine WAS modified)\n' "$_c_grn" "$_c_off" "$MODE"
