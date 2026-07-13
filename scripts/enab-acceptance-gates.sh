#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build}"

echo "==> ENAB-01 acceptance gates (BUILD=$BUILD)"

# TEST-11 HF gates use [regression] Catch tags but not "regression" in ctest names.
ctest --test-dir "$BUILD" -C Release --output-on-failure -R \
  "HF ringing (ProperSRC|no narrowband)|ProperSRC HF metrics invariant|fixed-rate reverb|Plugin reported latency stays zero|14825 Hz imaging vs LegacyAccumulator"

test -f "$ROOT/docs/architecture/ADR-003-proper-32k-src.md"

echo "ENAB-01 acceptance gates: PASS"
