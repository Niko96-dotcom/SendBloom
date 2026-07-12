---
phase: 16-engine-crossfade
plan: 01
subsystem: dsp
tags: [crossfade, schroeder, equal-power, juce, catch2]

requires:
  - phase: 14-block-integration
    provides: GatedBloomChain block path and SchroederTank32 processBlock
provides:
  - EngineCrossfade equal-power wet blend helper (35 ms default)
  - SchroederTank32 dual-engine parallel crossfade state machine
  - GatedBloomChain crossfade-aware block routing
  - PluginProcessor per-block authentic_color 0.5 edge detection
affects: [17-pdc, 18-enablement]

tech-stack:
  added: []
  patterns:
    - "Equal-power sin/cos engine crossfade separate from bypass crossfade"
    - "Idle reverb engine reset once after fade completes"
    - "Smoothed APVTS edge detection triggers requestEngineCrossfade once per block"

key-files:
  created:
    - source/EngineCrossfade.h
    - tests/EngineCrossfadeTest.cpp
  modified:
    - source/IReverbEngine.h
    - source/HostRateReverbEngine.h
    - source/SchroederTank32.h
    - source/GatedBloomChain.h
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp

key-decisions:
  - "EngineCrossfade uses dedicated SmoothedValue ramp, not authenticColorTarget smoother"
  - "Host path during crossfade calls hostEngine.processBlock directly to avoid recursion"
  - "blockStartAuthentic retained for rt60/dark/send snapshot only; routing uses isCrossfading()"

patterns-established:
  - "IReverbEngine crossfade hooks default no-op for Fdn8Reverb compatibility"
  - "GatedBloomChain block path when authenticColor OR reverb->isCrossfading()"

requirements-completed: [XFADE-01]

coverage:
  - id: D1
    description: "35 ms equal-power dual-engine crossfade helper with unit fade duration and click gates"
    requirement: XFADE-01
    verification:
      - kind: unit
        ref: "tests/EngineCrossfadeTest.cpp#[EngineCrossfade]"
        status: pass
    human_judgment: false
  - id: D2
    description: "SchroederTank32 parallel host/ProperSRC crossfade with post-fade idle engine reset"
    requirement: XFADE-01
    verification:
      - kind: unit
        ref: "ctest -R SchroederTank32"
        status: pass
    human_judgment: false
  - id: D3
    description: "Full-plugin authentic_color toggle stays click-bounded on wet path"
    requirement: XFADE-01
    verification:
      - kind: integration
        ref: "tests/EngineCrossfadeTest.cpp#[integration][XFADE-01]"
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-07-09
status: complete
---

# Phase 16 Plan 01: Engine Crossfade Summary

**Click-free 35 ms equal-power dual-engine crossfade when toggling authentic_color, with unit and integration click gates green**

## Performance

- **Duration:** 5 min
- **Started:** 2026-07-08T22:03:00Z
- **Completed:** 2026-07-08T22:07:34Z
- **Tasks:** 3
- **Files modified:** 8

## Accomplishments
- `EngineCrossfade` header-only helper with equal-power sin/cos wet blend and 20–50 ms ramp window
- `SchroederTank32` runs host and ProperSRC engines in parallel during fade; idle engine resets after completion
- `PluginProcessor` detects smoothed `authenticColorTarget` 0.5 crossings and triggers crossfade once per block edge
- Integration test confirms single authentic_color toggle stays below 1.0f max adjacent delta

## Task Commits

1. **Task 1: EngineCrossfade helper and unit tests** - `b0a67ec` (feat)
2. **Task 2: SchroederTank32 dual-engine crossfade state machine** - `a4d74bc` (feat)
3. **Task 3: GatedBloomChain routing and PluginProcessor edge detection** - `20439f9` (feat)

**Plan metadata:** skipped (commit_docs disabled)

## Files Created/Modified
- `source/EngineCrossfade.h` - Equal-power wet mix helper with SmoothedValue ramp
- `tests/EngineCrossfadeTest.cpp` - Fade duration, click metrics, integration toggle gate
- `source/SchroederTank32.h` - Dual-engine crossfade state machine and scratch buffers
- `source/IReverbEngine.h` - Virtual `isCrossfading()` / `requestEngineCrossfade()` hooks
- `source/HostRateReverbEngine.h` - Public `reset()` for idle-engine cleanup
- `source/GatedBloomChain.h` - Crossfade-aware block routing and forwarder
- `source/PluginProcessor.cpp` / `.h` - Per-block smoothed edge detection

## Decisions Made
- Dedicated `EngineCrossfade` smoother kept separate from `authenticColorTarget` per plan
- `hostEngine.processBlock` used directly during crossfade to avoid `processSample` recursion

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered
None

## User Setup Required
None - no external service configuration required.

## Next Phase Readiness
- XFADE-01 satisfied; ready for Phase 16 plan 02 (finiteness tests) and Phase 17 PDC
- Manual DAW toggle smoke still recommended per plan verification note

## Self-Check: PASSED
- FOUND: source/EngineCrossfade.h
- FOUND: tests/EngineCrossfadeTest.cpp
- FOUND: source/SchroederTank32.h
- FOUND: b0a67ec
- FOUND: a4d74bc
- FOUND: 20439f9

---
*Phase: 16-engine-crossfade*
*Completed: 2026-07-09*
