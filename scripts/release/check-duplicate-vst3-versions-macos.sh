#!/usr/bin/env bash
# Fail closed before delivering a VST3 beside another installation with the
# same SendBloom product identity. Hosts may encounter both the system and
# user-local bundle, and an equal (or older) candidate version gives them no
# reliable version-ordering signal for the stable VST3 class ID.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

require_macos
release_init
require_cmd plutil

CANDIDATE="${1:-$VST3_BUNDLE}"
SYSTEM_VST3="${SENDBLOOM_SYSTEM_VST3_PATH:-/Library/Audio/Plug-Ins/VST3/SendBloom.vst3}"
USER_VST3="${SENDBLOOM_USER_VST3_PATH:-${HOME:?}/Library/Audio/Plug-Ins/VST3/SendBloom.vst3}"

bundle_field() {
  local bundle="$1" key="$2"
  plutil -extract "$key" raw -o - "$bundle/Contents/Info.plist" 2>/dev/null || true
}

valid_core_version() {
  [[ "$1" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]
}

strictly_newer() {
  local candidate="$1" installed="$2"
  local c_major c_minor c_patch i_major i_minor i_patch
  IFS=. read -r c_major c_minor c_patch <<<"$candidate"
  IFS=. read -r i_major i_minor i_patch <<<"$installed"

  local c i
  for c in "$c_major" "$c_minor" "$c_patch"; do
    case "$c" in (*[!0-9]*|'') return 1 ;; esac
  done
  for i in "$i_major" "$i_minor" "$i_patch"; do
    case "$i" in (*[!0-9]*|'') return 1 ;; esac
  done

  if (( 10#$c_major != 10#$i_major )); then
    (( 10#$c_major > 10#$i_major ))
    return
  fi
  if (( 10#$c_minor != 10#$i_minor )); then
    (( 10#$c_minor > 10#$i_minor ))
    return
  fi
  (( 10#$c_patch > 10#$i_patch ))
}

step "Duplicate VST3 delivery guard"
info "candidate: $CANDIDATE"

[[ -d "$CANDIDATE" ]] || die "candidate VST3 is missing: $CANDIDATE"
[[ -f "$CANDIDATE/Contents/Info.plist" ]] \
  || die "candidate VST3 has no Info.plist: $CANDIDATE"

candidate_id="$(bundle_field "$CANDIDATE" CFBundleIdentifier)"
candidate_version="$(bundle_field "$CANDIDATE" CFBundleShortVersionString)"

[[ "$candidate_id" == "$BUNDLE_ID" ]] \
  || die "candidate bundle id '$candidate_id' does not match $BUNDLE_ID"
valid_core_version "$candidate_version" \
  || die "candidate version '$candidate_version' is not a numeric MAJOR.MINOR.PATCH"
[[ "$candidate_version" == "$VERSION_CORE" ]] \
  || die "candidate version $candidate_version does not match canonical $VERSION_CORE"

checked=0
declare -a seen=()
for installed in "$SYSTEM_VST3" "$USER_VST3"; do
  [[ "$installed" == "$CANDIDATE" ]] && continue

  duplicate_path=0
  for previous in "${seen[@]:-}"; do
    [[ -n "$previous" && "$previous" == "$installed" ]] && duplicate_path=1
  done
  [[ "$duplicate_path" == "1" ]] && continue
  seen+=("$installed")

  [[ -e "$installed" ]] || { info "not installed: $installed"; continue; }
  [[ -d "$installed" && -f "$installed/Contents/Info.plist" ]] \
    || die "ambiguous installed VST3 has no readable bundle metadata: $installed"

  installed_id="$(bundle_field "$installed" CFBundleIdentifier)"
  installed_version="$(bundle_field "$installed" CFBundleShortVersionString)"
  if [[ "$installed_id" != "$BUNDLE_ID" ]]; then
    warn "ignoring $installed: bundle id is '$installed_id', not $BUNDLE_ID"
    continue
  fi

  valid_core_version "$installed_version" \
    || die "installed SendBloom version '$installed_version' is not comparable: $installed"
  checked=$((checked + 1))

  if strictly_newer "$candidate_version" "$installed_version"; then
    ok "candidate $candidate_version is strictly newer than $installed_version at $installed"
  else
    die "candidate $candidate_version is not strictly newer than installed SendBloom $installed_version at $installed; a stable-CID duplicate would remain ambiguous"
  fi
done

if [[ "$checked" == "0" ]]; then
  ok "no installed SendBloom VST3 duplicate found"
fi

warn "This gate proves version ordering only. The target DAW must still prove which duplicate it instantiates."
printf '%sduplicate-vst3-version-guard: PASS%s (candidate %s; compared %d installed duplicate(s))\n' \
  "$_c_grn" "$_c_off" "$candidate_version" "$checked"
