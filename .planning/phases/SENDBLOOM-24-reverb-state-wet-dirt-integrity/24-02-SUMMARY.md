---
phase: 24-reverb-state-wet-dirt-integrity
plan: 02
subsystem: dsp
tags: [lfo-modulation, proper-src, adr-v1-13, adr-v1-14, catch2, fixed-rate-adapter]

requires:
  - phase: 24-01
    provides: ADR-V1-12 predelay in SchroederTankCore (unchanged by this plan)
provides:
  - ADR-V1-13 time-invariant tank LFO depth via tankLfoDepthSamplesForRate
  - ADR-V1-14 ProperSRC downsample output pre-clear in FixedRateAdapter
  - V1ContractModInvariantTest DSP-05 contract
  - V1ContractSrcUnderfillTest DSP-06/07 contracts
affects: [24-03-wet-dirt]

tech-stack:
  added: []
  patterns:
    - "LFO depth in seconds: kTankLfoDepthSeconds * processingRate"
    - "std::fill(out, out+n, 0) before RateConverterPair::downsample in ProperSRC branch"

key-files:
  created:
    - tests/V1ContractModInvariantTest.cpp
    - tests/V1ContractSrcUnderfillTest.cpp
  modified:
    - source/SchroederTank32DelayTable.h
    - source/SchroederTankCore.h
    - source/LegacyAccumulatorPath.h
    - source/FixedRateAdapter.h

key-decisions:
  - "kTankLfoDepthSeconds = 16.0/32768.0 preserves 16 samples at internal 32.768 kHz"
  - "kTankLfoDepthSamples retained as compile-time alias for documentation only"
  - "ProperSRC pre-clear uses std::fill only; kProperSrcQuality unchanged"

patterns-established:
  - "ADR-V1-13: tank AP mod uses tankLfoDepthSamplesForRate(processingRate)"
  - "ADR-V1-14: zero host output before downsample; jassert 0 <= written <= n"

requirements-completed: [DSP-05, DSP-06, DSP-07, DSP-08]

coverage:
  - id: D1
    description: "LFO depth seconds invariant at five rates (DSP-05)"
    requirement: DSP-05
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][mod-invariant]\""
        status: pass
    human_judgment: false
  - id: D2
    description: "ProperSRC output pre-cleared before downsample (DSP-06)"
    requirement: DSP-06
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][src-underfill][DSP-06]\""
        status: pass
    human_judgment: false
  - id: D3
    description: "Unwritten downsample tail slots remain zero (DSP-07)"
    requirement: DSP-07
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][src-underfill][DSP-07]\""
        status: pass
    human_judgment: false
  - id: D4
    description: "ProperSRC/HF imaging regression gates preserved (DSP-08)"
    requirement: DSP-08
    verification:
      - kind: integration
        ref: "Builds/Tests \"[verb][FixedRateAdapter][SRC-06]\""
        status: pass
      - kind: integration
        ref: "Builds/Tests \"[hf]\""
        status: pass
      - kind: integration
        ref: "Builds/Tests \"[release][safe]\""
        status: pass
    human_judgment: false

duration: 8min
completed: 2026-07-12
status: complete
---

# Phase 24 Plan 02: Mod Time-Invariant + SRC Underfill Summary

**ADR-V1-13 rate-scaled LFO depth and ADR-V1-14 ProperSRC output pre-clear; DSP-05…08 contracts green with SRC-06/HF preserved**

## Performance

- **Duration:** ~8 min
- **Started:** 2026-07-12T17:09:00Z
- **Completed:** 2026-07-12T17:13:00Z
- **Tasks:** 3
- **Files modified:** 6

## Accomplishments

- Added `kTankLfoDepthSeconds` and `tankLfoDepthSamplesForRate()` so modulation depth stays 16/32768 s at all processing rates (16 samples @ 32.768 kHz preserved).
- Wired `SchroederTankCore` and `LegacyAccumulatorPath` tank AP modulation to the helper instead of fixed `kTankLfoDepthSamples`.
- Pre-clear ProperSRC host output with `std::fill` before `downsample`; capture `written` and `jassert` bounds.
- Added `V1ContractModInvariantTest` and `V1ContractSrcUnderfillTest`; all regression gates (SRC-06, HF, predelay) stay green.

## Task Commits

Each task was committed atomically:

1. **Task 1: Mod depth time-invariant (DSP-05)** - `827008f` (feat)
2. **Task 2: ProperSRC underfill pre-clear (DSP-06/07)** - `28bd863` (feat)
3. **Task 3: ProperSRC/HF regression gate (DSP-08)** - verification only (no code changes)

**Plan metadata:** pending docs commit

## Files Created/Modified

- `source/SchroederTank32DelayTable.h` - ADR-V1-13 depth seconds helper
- `source/SchroederTankCore.h` - rate-scaled LFO mod depth
- `source/LegacyAccumulatorPath.h` - internal-rate LFO mod depth helper
- `source/FixedRateAdapter.h` - ADR-V1-14 pre-clear + written bounds assert
- `tests/V1ContractModInvariantTest.cpp` - DSP-05 five-rate contract
- `tests/V1ContractSrcUnderfillTest.cpp` - DSP-06/07 sentinel and tail-zero proofs

## Decisions Made

- Kept `kTankLfoHz` and `kProperSrcQuality` unchanged per locked CONTEXT decisions.
- `kTankLfoDepthSamples` aliased to `tankLfoDepthSamplesForRate(kInternalRate)` for backward-compatible documentation.

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 24-03 (wet dirt HP/DC + safety defaults) can proceed; mod and SRC integrity contracts are green.
- ADR-V1-12 predelay from 24-01 verified still green via `[predelay]` filter.

## Self-Check: PASSED

- FOUND: tests/V1ContractModInvariantTest.cpp
- FOUND: tests/V1ContractSrcUnderfillTest.cpp
- FOUND: source/SchroederTank32DelayTable.h (kTankLfoDepthSeconds)
- FOUND: source/FixedRateAdapter.h (std::fill before downsample)
- FOUND: commit 827008f
- FOUND: commit 28bd863

---
*Phase: 24-reverb-state-wet-dirt-integrity*
*Completed: 2026-07-12*
