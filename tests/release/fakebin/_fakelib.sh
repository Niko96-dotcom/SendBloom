#!/usr/bin/env bash
# Shared helpers for the faked platform tools.
#
# Every fake appends the command it was asked to perform to $FAKE_LOG. The
# order tests read that log to assert that build, sign, notarize, staple,
# package, checksum and publish happened in the only safe sequence.

fake_log() {
  [[ -n "${FAKE_LOG:-}" ]] || return 0
  printf '%s\n' "$*" >>"$FAKE_LOG"
}

# Marker directory used to remember which paths have been "stapled".
fake_state_dir() {
  printf '%s' "${FAKE_STATE:-${TMPDIR:-/tmp}/sendbloom-fake-state}"
}

# Keyed on the BASENAME, not the full path. In reality a signature and a
# stapled ticket live inside the file, so they survive being copied into a
# staging folder or a disk image. Keying on the path would make the fakes
# forget, and the tests would then assert something the real tools do not do.
fake_marker() {
  local kind="$1" path="$2"
  local dir; dir="$(fake_state_dir)"
  mkdir -p "$dir"
  printf '%s/%s.%s' "$dir" "$kind" "$(printf '%s' "$(basename "$path")" | shasum -a 256 | cut -c1-32)"
}
