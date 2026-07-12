---
phase: 24-reverb-state-wet-dirt-integrity
plan: 01
subsystem: dsp
tags: [predelay, schroeder-tank, adr-v1-12, catch2, legacy-accumulator]

requires: []
provides:
  - ADR-V1-12 continuous fixed 55 ms predelay tap in SchroederTankCore
  - LegacyAccumulatorPath SRC-05 predelay parity
  - V1ContractPredelayTest DSP-01…04 contract proofs
affects: [24-02-mod-src, 24-03-wet-dirt]

tech-stack:
  added: []
  patterns:
    - "Fixed predelay delay at prepare; darkMix lerp-only blend every sample"
    - "Unconditional predelayLine push/pop in bright and dark modes"

key-files:
  created:
    - tests/V1ContractPredelayTest.cpp
  modified:
    - source/SchroederTankCore.h
    - source/LegacyAccumulatorPath.h

key-decisions:
  - "Predelay delay set once in prepare from kDarkPredelaySeconds × processing rate"
  - "darkMix_ member drives lerp only; updateCoeffs no longer mutates delay length"
  - "DSP-01 contract uses differential early-window impulse after bright silence to isolate predelay from tank ringing"

patterns-established:
  - "ADR-V1-12 predelay: push/pop every sample; x = input + darkMix * (delayed - input)"

requirements-completed: [DSP-01, DSP-02, DSP-03, DSP-04]

coverage:
  - id: D1
    description: "V1ContractPredelayTest DSP-01…04 [v1][contract][predelay] green"
    requirement: DSP-01
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][predelay]\""
        status: pass
    human_judgment: false
  - id: D2
    description: "SchroederTankCore ADR-V1-12 fixed 55 ms tap with continuous clock"
    requirement: DSP-02
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][predelay][DSP-02]\""
        status: pass
    human_judgment: false
  - id: D3
    description: "LegacyAccumulatorPath predelay parity for SRC-05"
    requirement: DSP-03
    verification:
      - kind: integration
        ref: "Builds/Tests \"[verb][FixedRateAdapter][LegacyAccumulator]\""
        status: pass
    human_judgment: false

duration: 45min
completed: 2026-07-12
status: complete
---

# Phase 24 Plan 01: Predelay Fixed Tap Summary

**ADR-V1-12 continuous fixed 55 ms dark predelay with mix-only lerp in SchroederTankCore and LegacyAccumulatorPath; DSP-01…04 contracts green**

## Performance

- **Duration:** ~45 min
- **Started:** 2026-07-12T16:58:00Z
- **Completed:** 2026-07-12T17:43:00Z
- **Tasks:** 3
- **Files modified:** 3

## Accomplishments

- Added `V1ContractPredelayTest.cpp` with DSP-01…04 and host/ProperSRC predelay-delta parity proofs
- Replaced variable `darkMix × 55 ms` delay and conditional clock gate with fixed tap + unconditional push/pop in `SchroederTankCore`
- Mirrored identical predelay semantics in `LegacyAccumulatorPath` for SRC-05 legacy diagnostics parity
- Preserved SchroederTank32/Core RT60, latency tail, Phase 20–23 contract greens

## Task Commits

1. **Task 1: Wave 0 predelay contracts** - `b8cce0b` (test)
2. **Task 2: SchroederTankCore fixed tap** - `6e40f94` (feat)
3. **Task 3: LegacyAccumulator parity** - `8b77f1c` (feat)

## Files Created/Modified

- `tests/V1ContractPredelayTest.cpp` - DSP-01…04 `[v1][contract][predelay]` contract suite
- `source/SchroederTankCore.h` - Fixed delay in `prepare`, `darkMix_` lerp, unconditional clock
- `source/LegacyAccumulatorPath.h` - SRC-05 parity mirror of ADR-V1-12 predelay topology

## Decisions Made

- Predelay delay length set only in `prepare` / rate change; `updateCoeffs` stores `darkMix_` for damping maps only
- DSP-01 uses differential early-window (10 ms) impulse measurement after extended bright silence + tank decay to separate predelay flush from tank ringing
- Parity case compares dark−bright onset delta between host core and ProperSRC adapter (not absolute onset) to tolerate SRC group delay

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Test design] Refined DSP-01 and parity contract assertions during GREEN**
- **Found during:** Task 2 (SchroederTankCore fixed tap)
- **Issue:** Initial DSP-01 measured full-tank output including reverb ringing; parity compared absolute onsets skewed by SRC latency
- **Fix:** Differential early-window DSP-01 with min RT60 + decay silence; parity uses dark−bright predelay delta
- **Files modified:** `tests/V1ContractPredelayTest.cpp`
- **Verification:** `Builds/Tests "[v1][contract][predelay]"` exits 0
- **Committed in:** `6e40f94`

---

**Total deviations:** 1 auto-fixed (Rule 1 test correctness)
**Impact on plan:** Test refinements only; production semantics match ADR-V1-12 as specified.

## Issues Encountered

None blocking. SchroederTank32 dark predelay regression test remained green without modification.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 24-02 (LFO time-invariant depth + ProperSRC underfill) can proceed independently
- Predelay contracts green; no blockers for mod/SRC or wet-dirt plans

## Self-Check: PASSED

- FOUND: tests/V1ContractPredelayTest.cpp
- FOUND: source/SchroederTankCore.h
- FOUND: source/LegacyAccumulatorPath.h
- FOUND: b8cce0b
- FOUND: 6e40f94
- FOUND: 8b77f1c

---
*Phase: 24-reverb-state-wet-dirt-integrity*
*Completed: 2026-07-12*
