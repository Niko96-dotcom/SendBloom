#!/usr/bin/env bash
# SendBloom durable v1 automated gate runner (BASE-05 / BASE-06 / BASE-08).
# Collects per-gate statuses, prints a truthful red/green table, and exits
# non-zero when any automated gate fails. Human-only gates are never PASS.
#
# Intentionally not blanket `set -e`: we record failures and continue to a
# final status table. Use `set -uo pipefail` for unbound-var / pipe safety.
set -uo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/Builds}"
BUILD_TYPE="${BUILD_TYPE:-Release}"
RUN_PLUGINVAL="${RUN_PLUGINVAL:-1}"
PLUGINVAL_BIN="${PLUGINVAL_BIN:-}"
STRICTNESS_LEVEL="${STRICTNESS_LEVEL:-10}"

if [[ -z "$PLUGINVAL_BIN" ]]; then
  if command -v pluginval >/dev/null 2>&1; then
    PLUGINVAL_BIN="$(command -v pluginval)"
  elif [[ -x "/Applications/pluginval.app/Contents/MacOS/pluginval" ]]; then
    PLUGINVAL_BIN="/Applications/pluginval.app/Contents/MacOS/pluginval"
  fi
fi

declare -a GATE_NAMES=()
declare -a GATE_STATUSES=()
declare -a GATE_DETAILS=()

record_gate() {
  local name="$1"
  local status="$2"
  local detail="${3:-}"
  GATE_NAMES+=("$name")
  GATE_STATUSES+=("$status")
  GATE_DETAILS+=("$detail")
}

automated_failed=0

mark_fail() {
  automated_failed=1
}

echo "========================================"
echo "SendBloom verify-v1 (automated gates)"
echo "ROOT=$ROOT"
echo "BUILD_DIR=$BUILD_DIR"
echo "BUILD_TYPE=$BUILD_TYPE"
echo "========================================"
echo

# --- (a) Legal metadata -------------------------------------------------------
echo "==> [1/7] Legal metadata audit"
if bash "$ROOT/scripts/check-legal-metadata.sh"; then
  record_gate "legal-metadata" "PASS" "check-legal-metadata.sh"
else
  record_gate "legal-metadata" "FAIL" "check-legal-metadata.sh failed"
  mark_fail
fi
echo

# --- (b) Reference claim truth -----------------------------------------------
echo "==> [2/7] Reference claim truth"
if bash "$ROOT/scripts/verify-reference-claims.sh"; then
  record_gate "reference-claims" "PASS" "verify-reference-claims.sh"
else
  record_gate "reference-claims" "FAIL" "verify-reference-claims.sh failed"
  mark_fail
fi
echo

# --- (c) Configure if needed --------------------------------------------------
echo "==> [3/7] CMake configure (if needed)"
if [[ ! -f "$BUILD_DIR/CMakeCache.txt" ]]; then
  echo "No CMakeCache.txt — configuring into $BUILD_DIR"
  if cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" "$ROOT"; then
    record_gate "cmake-configure" "PASS" "cmake -B $BUILD_DIR"
  else
    record_gate "cmake-configure" "FAIL" "cmake configure failed"
    mark_fail
  fi
else
  echo "CMakeCache.txt present — skipping configure"
  record_gate "cmake-configure" "PASS" "already configured ($BUILD_DIR)"
fi
echo

# --- (d) Build every shipping target plus tests -------------------------------
echo "==> [4/7] cmake --build all ($BUILD_TYPE)"
if cmake --build "$BUILD_DIR" --config "$BUILD_TYPE"; then
  record_gate "build-shipping" "PASS" "all configured targets / $BUILD_TYPE"
else
  record_gate "build-shipping" "FAIL" "cmake --build failed"
  mark_fail
fi
echo

VST3_PATH="${VST3_PATH:-$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/VST3/SendBloom.vst3}"
AU_PATH="${AU_PATH:-$BUILD_DIR/SendBloom_artefacts/$BUILD_TYPE/AU/SendBloom.component}"

if [[ -d "$VST3_PATH" ]]; then
  record_gate "artifact-vst3" "PASS" "$VST3_PATH"
else
  record_gate "artifact-vst3" "FAIL" "missing $VST3_PATH"
  mark_fail
fi

if [[ "$(uname -s)" == "Darwin" ]]; then
  if [[ -d "$AU_PATH" ]]; then
    record_gate "artifact-au" "PASS" "$AU_PATH"
  else
    record_gate "artifact-au" "FAIL" "missing $AU_PATH"
    mark_fail
  fi
fi
echo

# --- Discover suite count at runtime (BASE-06) --------------------------------
DISCOVERED_TOTAL=""
DISCOVER_OUT="$(ctest --test-dir "$BUILD_DIR" -N 2>/dev/null || true)"
if echo "$DISCOVER_OUT" | grep -qE 'Total Tests:[[:space:]]*[0-9]+'; then
  DISCOVERED_TOTAL="$(echo "$DISCOVER_OUT" | sed -nE 's/.*Total Tests:[[:space:]]*([0-9]+).*/\1/p' | tail -1)"
fi
if [[ -n "$DISCOVERED_TOTAL" ]]; then
  echo "Discovered ctest suite size (runtime): $DISCOVERED_TOTAL"
else
  echo "WARN: could not discover Total Tests via ctest -N"
fi
echo

# --- (e) Full ctest suite -----------------------------------------------------
echo "==> [5/7] ctest full suite ($BUILD_TYPE)"
CTEST_LOG="$(mktemp -t verify-v1-ctest.XXXXXX)"
ctest --test-dir "$BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure >"$CTEST_LOG" 2>&1
CTEST_EC=$?
cat "$CTEST_LOG"

CTEST_DETAIL="exit=$CTEST_EC"
if [[ -n "$DISCOVERED_TOTAL" ]]; then
  CTEST_DETAIL+="; discovered_total=$DISCOVERED_TOTAL"
fi

# Parse runtime pass/fail summary when present (never a hard-coded expectation).
CTEST_SKIPPED="$(grep -c '\*\*\*Skipped' "$CTEST_LOG" || true)"
if grep -qE '[0-9]+ tests failed out of [0-9]+' "$CTEST_LOG"; then
  CTEST_FAILED="$(sed -nE 's/.* ([0-9]+) tests failed out of ([0-9]+).*/\1/p' "$CTEST_LOG" | tail -1)"
  CTEST_OUT_OF="$(sed -nE 's/.* ([0-9]+) tests failed out of ([0-9]+).*/\2/p' "$CTEST_LOG" | tail -1)"
  if [[ -n "$CTEST_FAILED" && -n "$CTEST_OUT_OF" ]]; then
    CTEST_PASSED=$((CTEST_OUT_OF - CTEST_FAILED - CTEST_SKIPPED))
    CTEST_DETAIL+="; passed=$CTEST_PASSED skipped=$CTEST_SKIPPED failed=$CTEST_FAILED (runtime)"
  fi
elif grep -qE '100% tests passed' "$CTEST_LOG"; then
  CTEST_DETAIL+="; all discovered tests passed (runtime)"
fi

if [[ "$CTEST_EC" -eq 0 ]]; then
  record_gate "ctest-full-suite" "PASS" "$CTEST_DETAIL"
else
  record_gate "ctest-full-suite" "FAIL" "$CTEST_DETAIL (expected red while [v1][contract] remain)"
  mark_fail
fi
rm -f "$CTEST_LOG"
echo

# --- (f) ENAB acceptance gates ------------------------------------------------
echo "==> [6/7] ENAB-01 acceptance gates"
if BUILD_DIR="$BUILD_DIR" bash "$ROOT/scripts/enab-acceptance-gates.sh"; then
  record_gate "enab-acceptance" "PASS" "enab-acceptance-gates.sh"
else
  record_gate "enab-acceptance" "FAIL" "enab-acceptance-gates.sh failed"
  mark_fail
fi
echo

# --- (g) Required pluginval (VST3) --------------------------------------------
echo "==> [7/7] Required VST3 pluginval"
if [[ "$RUN_PLUGINVAL" == "1" ]]; then
  if [[ -z "$PLUGINVAL_BIN" || ! -x "$PLUGINVAL_BIN" ]]; then
    record_gate "pluginval-vst3" "FAIL" "PLUGINVAL_BIN not found: $PLUGINVAL_BIN"
    mark_fail
  elif [[ ! -e "$VST3_PATH" ]]; then
    record_gate "pluginval-vst3" "FAIL" "VST3 missing: $VST3_PATH"
    mark_fail
  else
    if "$PLUGINVAL_BIN" --strictness-level "$STRICTNESS_LEVEL" --verbose --validate "$VST3_PATH"; then
      record_gate "pluginval-vst3" "PASS" "strictness=$STRICTNESS_LEVEL $VST3_PATH"
    else
      record_gate "pluginval-vst3" "FAIL" "pluginval failed"
      mark_fail
    fi
  fi
else
  record_gate "pluginval-vst3" "FAIL" "disabled by RUN_PLUGINVAL=$RUN_PLUGINVAL; release verification is fail-closed"
  mark_fail
fi
echo

# --- Status table -------------------------------------------------------------
echo "========================================"
echo "STATUS TABLE (automated)"
echo "========================================"
printf '%-22s %-10s %s\n' "GATE" "STATUS" "DETAIL"
printf '%-22s %-10s %s\n' "----" "------" "------"
overall="GREEN"
for i in "${!GATE_NAMES[@]}"; do
  st="${GATE_STATUSES[$i]}"
  printf '%-22s %-10s %s\n' "${GATE_NAMES[$i]}" "$st" "${GATE_DETAILS[$i]}"
  if [[ "$st" != "PASS" ]]; then
    overall="RED"
  fi
done
echo
if [[ "$overall" == "GREEN" ]]; then
  echo "Overall automated status: GREEN / PASS"
else
  echo "Overall automated status: RED / FAIL"
fi
if [[ -n "$DISCOVERED_TOTAL" ]]; then
  echo "Runtime-discovered ctest total: $DISCOVERED_TOTAL (not a hard-coded expectation)"
fi
echo

# --- HUMAN_NEEDED (BASE-08) — never PASS --------------------------------------
echo "========================================"
echo "HUMAN_NEEDED (explicit; never silent green)"
echo "========================================"
echo "- AU pluginval / auval: human_needed (not automated in this runner)"
echo "- Windows CI matrix (when not executed in this run): human_needed"
echo "- Linux CI matrix (when not executed in this run): human_needed"
echo "- DAW smoke Logic: human_needed"
echo "- DAW smoke Cubase: human_needed"
echo "- DAW smoke REAPER: human_needed"
echo "- Developer ID signing: human_needed"
echo "- Notarization / stapling: human_needed"
echo "- JUCE commercial entitlement evidence: human_needed"
echo "- Hardware reference grids: human_needed"
echo "- Blind or level-matched listening review: human_needed"
echo
# Note: do not place the token PASS/passed on any human_needed line (BASE-08).


if [[ "$automated_failed" -ne 0 ]]; then
  echo "verify-v1: AUTOMATED GATES FAILED (exit 1)"
  exit 1
fi

echo "verify-v1: all automated gates passed (exit 0)"
exit 0
