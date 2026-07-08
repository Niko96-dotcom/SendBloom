#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/build}"

echo "==> ENAB-01 acceptance gates (BUILD=$BUILD)"

ctest --test-dir "$BUILD" -C Release --output-on-failure -R \
  "HF ringing.*regression|ProperSRC HF metrics invariant|EngineCrossfade|processBlock survives 1000 authentic_color|Plugin reports SRC latency|14825 Hz imaging vs LegacyAccumulator"

test -f "$ROOT/docs/architecture/ADR-003-proper-32k-src.md"

echo "ENAB-01 acceptance gates: PASS"
