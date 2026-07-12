---
phase: 15-three-path-diagnostics
plan: 02
subsystem: testing
tags: [cpp, catch2, schroeder-tank32, hf-diagnostics, three-path]

requires:
  - phase: 15-three-path-diagnostics
    plan: 01
    provides: HfDiagnosticsHelpers.h with renderTankPath, fixtures, imaging14825Rms
provides:
  - AuthenticPathDiagnosticsTest.cpp with DIAG-01/02/03 tagged Catch2 cases
  - Three-path CSV matrix at 48 kHz block 512
  - Legacy vs ProperSRC imaging differential gate on guitar pluck
affects: [15-03, 15-04, DIAG-04, TEST-08]

tech-stack:
  added: []
  patterns:
    - "Fresh SchroederTank32 per path render prevents state bleed"
    - "Impulse reserved for finiteness spot-checks only (not imaging gates)"

key-files:
  created:
    - tests/AuthenticPathDiagnosticsTest.cpp
  modified: []

key-decisions:
  - "Used renderFreshTankPath wrapper to guarantee isolated tank per path/fixture cell"
  - "DIAG-02 impulse spot-check limited to HostRate and LegacyAccumulator finiteness per RESEARCH Pitfall 3"

patterns-established:
  - "Three-path test tags: [diagnostics][DIAG-01|02|03] for selective CTest filtering"
  - "Imaging ceiling 0.0022f and narrowband dominance 10.0f aligned with HighFrequencyRingingDiagnosticsTest"

requirements-completed: [DIAG-01, DIAG-02, DIAG-03]

coverage:
  - id: D1
    description: Three-path side-by-side render matrix with CSV output at 48 kHz
    requirement: DIAG-01
    verification:
      - kind: unit
        ref: "build/Tests \"[DIAG-01]\" -r compact"
        status: pass
    human_judgment: false
  - id: D2
    description: Five fixtures verified across three-path harness with impulse spot-check
    requirement: DIAG-02
    verification:
      - kind: unit
        ref: "build/Tests \"[DIAG-02]\" -r compact"
        status: pass
    human_judgment: false
  - id: D3
    description: ProperSRC HF metric suite with legacy imaging differential on guitar pluck
    requirement: DIAG-03
    verification:
      - kind: unit
        ref: "build/Tests \"[DIAG-03]\" -r compact"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-07-08
status: complete
---

# Phase 15 Plan 02: Three-Path Diagnostics Matrix Summary

**Catch2 three-path SchroederTank32 harness proving LegacyAccumulator imaging exceeds ProperSRC on guitar pluck with full HF metric gates**

## Performance

- **Duration:** 3 min
- **Started:** 2026-07-08T21:44:47Z
- **Completed:** 2026-07-08T21:47:30Z
- **Tasks:** 3
- **Files modified:** 1

## Accomplishments

- Created `tests/AuthenticPathDiagnosticsTest.cpp` with `[DIAG-01]` three-path CSV matrix (15 fixture×path cells)
- Added `[DIAG-02]` five-fixture coverage gate with impulse-only HostRate/Legacy finiteness spot-check
- Added `[DIAG-03]` ProperSRC HF metric suite with legacy imaging differential aligned to SRC-06 spirit

## Task Commits

1. **Task 1: Three-path render matrix with CSV output** - `3ea5c9f` (test)
2. **Task 2: Fixture coverage gate** - `92e2e5a` (test)
3. **Task 3: HF metric suite assertions on ProperSRC path** - `8d9189e` (test)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified

- `tests/AuthenticPathDiagnosticsTest.cpp` - DIAG-01/02/03 tagged three-path diagnostics harness

## Decisions Made

- Fresh `SchroederTank32` per render via `renderFreshTankPath` to avoid cross-path state bleed
- Impulse excluded from legacy-vs-proper imaging gates per Phase 15 research Pitfall 3

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## Next Phase Readiness

- DIAG-01/02/03 green at 48 kHz; ready for 15-03 multi-rate invariance (DIAG-04) and 15-04 finiteness (TEST-08)

## Self-Check: PASSED

- FOUND: tests/AuthenticPathDiagnosticsTest.cpp
- FOUND: 3ea5c9f
- FOUND: 92e2e5a
- FOUND: 8d9189e

---
*Phase: 15-three-path-diagnostics*
*Completed: 2026-07-08*
