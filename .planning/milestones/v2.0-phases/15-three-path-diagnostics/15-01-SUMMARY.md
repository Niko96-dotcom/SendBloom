---
phase: 15-three-path-diagnostics
plan: 01
subsystem: testing
tags: [cpp, catch2, goertzel, schroeder-tank32, hf-diagnostics]

requires:
  - phase: 14-integrability
    provides: SchroederTank32 diagnostics API and processBlock authentic path
provides:
  - Shared HF fixture generators (DIAG-02)
  - HfMetrics with rmsAbove14k and tail spectral fields (DIAG-03)
  - Three-path renderTankPath harness for DIAG-01 downstream tests
affects: [15-02, 15-03, 15-04, AuthenticPathDiagnosticsTest]

tech-stack:
  added: []
  patterns:
    - "Header-only sendbloom::test helpers extracted from HighFrequencyRingingDiagnosticsTest"
    - "ReverbPath enum routes setAuthentic32ModeForDiagnostics before processBlock"

key-files:
  created:
    - tests/HfDiagnosticsHelpers.h
  modified: []

key-decisions:
  - "Kept Goertzel step sizes and tail window identical to HighFrequencyRingingDiagnosticsTest to prevent metric drift"
  - "renderTankPath calls prepare per invocation so callers can reuse tank with fresh state"

patterns-established:
  - "Three-path render: HostRate (auth off), LegacyAccumulator (diag mode), ProperSRC (diag cleared, auth on)"
  - "CSV helpers separate legacy config/fixture columns from three-path path/fixture matrix"

requirements-completed: [DIAG-02, DIAG-03]

coverage:
  - id: D1
    description: Five standard HF fixtures available from shared header
    requirement: DIAG-02
    verification:
      - kind: unit
        ref: "compile probe /tmp/hf_compile_check.cpp via Tests target flags"
        status: pass
    human_judgment: false
  - id: D2
    description: HfMetrics includes rmsAbove14k plus tail spectral fields
    requirement: DIAG-03
    verification:
      - kind: unit
        ref: "compile probe exercises measureTail and imaging14825Rms"
        status: pass
    human_judgment: false
  - id: D3
    description: Three-path SchroederTank32 render via processBlock with diagnostics mode per path
    requirement: DIAG-01
    verification:
      - kind: unit
        ref: "compile probe exercises renderTankPath ProperSRC"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-07-08
status: complete
---

# Phase 15 Plan 01: HfDiagnosticsHelpers Summary

**Shared HF diagnostics header with five fixtures, rmsAbove14k metrics, and three-path SchroederTank32 render harness**

## Performance

- **Duration:** 3 min
- **Started:** 2026-07-08T21:40:03Z
- **Completed:** 2026-07-08T21:42:02Z
- **Tasks:** 3
- **Files modified:** 1

## Accomplishments

- Created `tests/HfDiagnosticsHelpers.h` with five named fixtures (guitar_pluck, sine220_decay, sine880_decay, impulse, swept_sine)
- Added `ReverbPath` enum, `applyPathDiagnostics`, and `renderTankPath` block-loop render for host / legacy / proper paths
- Ported full `HfMetrics` suite with new `rmsAbove14k`, `imaging14825Rms`, and three-path CSV output helpers

## Task Commits

Each task was committed atomically:

1. **Task 1: Create HfDiagnosticsHelpers.h with fixtures and render constants** - `5c33e40` (feat)
2. **Task 2: Add ReverbPath enum and processBlock render harness** - `ca88fb3` (feat)
3. **Task 3: HfMetrics suite with rmsAbove14k and tail analysis** - `e45cb19` (feat)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified

- `tests/HfDiagnosticsHelpers.h` - Header-only shared fixtures, three-path render, HF tail metrics, CSV helpers

## Decisions Made

- Preserved exact numeric formulas from `HighFrequencyRingingDiagnosticsTest.cpp` for metric parity
- `renderTankPath` calls `prepare` each invocation — downstream tests should use a fresh tank or call per path

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 15-02 can add `AuthenticPathDiagnosticsTest.cpp` consuming `HfDiagnosticsHelpers.h`
- `HighFrequencyRingingDiagnosticsTest.cpp` still has inline duplicates until 15-02 refactors it

---
*Phase: 15-three-path-diagnostics*
*Completed: 2026-07-08*

## Self-Check: PASSED

- FOUND: tests/HfDiagnosticsHelpers.h
- FOUND: 5c33e40
- FOUND: ca88fb3
- FOUND: e45cb19
