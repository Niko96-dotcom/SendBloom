#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

fail() { echo "ERROR: $*" >&2; exit 1; }

[[ -f CLAIM_STATUS.md ]] || fail "CLAIM_STATUS.md missing"
[[ -f docs/reference-capture-protocol.md ]] || fail "capture protocol missing"

status_count="$(grep -Ec '^\*\*Status:\*\* `(original-inspired|hardware-compared|fidelity-claim-approved)`$' CLAIM_STATUS.md || true)"
[[ "$status_count" == 1 ]] || fail "CLAIM_STATUS.md must assign exactly one ADR-V1-17 status"
grep -q '^\*\*Status:\*\* `original-inspired`$' CLAIM_STATUS.md || fail "current evidence supports only original-inspired"
grep -q 'comparison grids: `human_needed`' CLAIM_STATUS.md || fail "hardware grids must remain human_needed"
grep -q 'listening verdict: `human_needed`' CLAIM_STATUS.md || fail "listening must remain human_needed"
grep -q 'original software implementation inspired by publicly described gated ambience behavior' README.md || fail "README public wording drifted"
grep -q 'CLAIM_STATUS.md' README.md || fail "README must link the active claim status"

while IFS= read -r path; do
  lower_path="$(printf '%s' "$path" | tr '[:upper:]' '[:lower:]')"
  case "$lower_path" in
    *.eep|*.eeprom|*.hex|*.bit|*.svf|*.jed|*.sch|*.brd) fail "prohibited extracted hardware/firmware artifact tracked: $path" ;;
  esac
done < <(git ls-files)

python3 -m unittest discover -s tests/reference -v
echo "Reference claim verification passed: status=original-inspired; human evidence remains human_needed."
