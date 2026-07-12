---
phase: 20-pressure-send-state-truth
plan: 01
subsystem: dsp
tags: [PressureController, send-gain, asymmetric-smooth, APVTS, Catch2]

requires:
  - phase: 19-baseline-contracts-failure-harness
    provides: PressureSend computeGain, ParameterCurves::sendGain, Catch2 [send] harness
provides:
  - PressureController with asymmetric 3/25 ms pressure smooth then Firm/Soft curve
  - send_amount layout default 0 (UX-02)
  - PluginProcessor per-sample send-gain path without bank dual-smooth
affects:
  - 20-02 pad/overlay/Advanced (consumes DSP truth)
  - 20-03 preset matrix / SEND-14 caveat
  - 22 MIDI pressure target wiring

tech-stack:
  added: []
  patterns:
    - "Smooth raw pressure asymmetrically before ParameterCurves::sendGain"
    - "GatedBloomChain processBlock overload takes const float* sendGains"

key-files:
  created:
    - source/PressureController.h
    - tests/PressureControllerTest.cpp
  modified:
    - source/ParameterLayout.cpp
    - source/SmoothedParameterBank.h
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp
    - source/GatedBloomChain.h
    - tests/ParameterLayoutTest.cpp

key-decisions:
  - "PressureController owns live send gain; bank sendGain smoother removed"
  - "prepareToPlay uses snapToTarget so DryPath/reference tanks stay aligned"
  - "MIDI pressure target forced 0 in Phase 20 (Phase 22 wires realtime)"
  - "ParameterSnapshot.sendGain kept as diagnostic/static curve; not audio-thread truth"

patterns-established:
  - "Pattern: disconnected → unity gain; connected → max(host,midi) → asym smooth → curve"
  - "Pattern: per-sample sendGainScratch_ fed into GatedBloomChain"

requirements-completed: [SEND-01, SEND-02, SEND-03, SEND-04, SEND-09, SEND-10, SEND-12, SEND-13, UX-01, UX-02]

coverage:
  - id: D1
    description: Disconnected mode yields unity send gain
    requirement: SEND-01
    verification:
      - kind: unit
        ref: tests/PressureControllerTest.cpp#disconnected returns unity
        status: pass
    human_judgment: false
  - id: D2
    description: Connected-at-rest host 0 settles to zero gain
    requirement: SEND-02
    verification:
      - kind: unit
        ref: tests/PressureControllerTest.cpp#connected at host 0 settles to zero
        status: pass
    human_judgment: false
  - id: D3
    description: Firm vs Soft curves remain distinct after settle
    requirement: SEND-09
    verification:
      - kind: unit
        ref: tests/PressureControllerTest.cpp#Firm/Soft settled gain
        status: pass
    human_judgment: false
  - id: D4
    description: Attack ~3 ms / release ~25 ms on raw pressure before curve
    requirement: SEND-10
    verification:
      - kind: unit
        ref: tests/PressureControllerTest.cpp#attack/release timing
        status: pass
    human_judgment: false
  - id: D5
    description: Default send_amount is 0; ParameterIDs unchanged
    requirement: UX-02
    verification:
      - kind: unit
        ref: tests/ParameterLayoutTest.cpp#default send_amount is 0
        status: pass
    human_judgment: false
  - id: D6
    description: Processor drives wet send from PressureController without bank dual-smooth
    requirement: SEND-13
    verification:
      - kind: unit
        ref: Builds/Tests "[send]" + "[DryPath]" + "[release]"
        status: pass
    human_judgment: false

duration: 10min
completed: 2026-07-12
status: complete
---

# Phase 20 Plan 01: PressureController + defaults + processor wiring Summary

**Asymmetric PressureController (3 ms attack / 25 ms release on raw pressure → Firm/Soft curve) owns audio-thread send gain; default send_amount is 0; bank dual-smooth removed.**

## Performance

- **Duration:** 10 min
- **Started:** 2026-07-12T15:39:20Z
- **Completed:** 2026-07-12T15:49:00Z
- **Tasks:** 2
- **Files modified:** 8

## Accomplishments

- Header-only `PressureController` with clamp, disconnect unity, connected smooth-then-curve
- Catch2 `[send][PressureController]` covers SEND-01/02/09/10/12 timing and gain semantics
- `send_amount` default 0 (UX-02); IDs `send_amount` / `send_connected` unchanged (UX-01)
- `PluginProcessor` advances per-sample gains into `GatedBloomChain`; oversized dry-fallback left alone
- Removed `SmoothedParameterBank` sendGain smoother to prevent dual smoothing

## Task Commits

Each task was committed atomically:

1. **Task 1: PressureController + unit tests** - `2939e01` (feat)
2. **Task 2: Layout default 0 + processor/bank handoff** - `79bbd0e` (feat)

**Plan metadata:** (docs commit after state update)

## Files Created/Modified

- `source/PressureController.h` - prepare/reset/snapToTarget, setConnected/Host/Midi/FirmFeel, processSample
- `tests/PressureControllerTest.cpp` - disconnect/rest/Firm≠Soft/attack/release/clamp
- `source/ParameterLayout.cpp` - send_amount default 0.0f
- `source/SmoothedParameterBank.h` - remove sendGain smoother
- `source/PluginProcessor.h/.cpp` - PressureController member + sendGainScratch_ wiring
- `source/GatedBloomChain.h` - per-sample `const float* sendGains` overload
- `tests/ParameterLayoutTest.cpp` - UX-02 default assert + ID stability

## Decisions Made

- MIDI target stubbed to 0 for Phase 20 (Phase 22 owns realtime MIDI)
- `snapToTarget()` on prepare mirrors former bank `snapToTargets` so DryPath reference tanks stay aligned
- Snapshot still computes static `sendGain` for diagnostics/tests; live path ignores it
- ParameterSnapshot.h left unchanged (plan allowed keeping raw fields + static sendGain)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] prepare reset zeroed pressure and broke DryPath tank parity**
- **Found during:** Task 2 verification (`[DryPath]`)
- **Issue:** `pressureController.reset()` after setting targets forced a 3 ms attack from 0 on every `prepareToPlay`, so plugin reverb energy diverged from the DryPathIntegrityTest local chain (maxDelta 0.045 > 0.02)
- **Fix:** Added `snapToTarget()` and call it from `prepareToPlay` instead of `reset()`
- **Files modified:** `source/PressureController.h`, `source/PluginProcessor.cpp`
- **Verification:** `Builds/Tests "[DryPath]"` green
- **Committed in:** `79bbd0e` (Task 2)

---

**Total deviations:** 1 auto-fixed (1 bug)
**Impact on plan:** Required for BASE-04 DryPath parity; no scope creep.

## Issues Encountered

None beyond the DryPath prepare-snap fix above.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- DSP truth for connected-at-rest / disconnect / SEND-10 ready for 20-02 pad release-to-zero + overlay
- Do not touch Phase 21 span or Phase 22 MIDI APVTS store in remaining Phase 20 plans

## Self-Check: PASSED

- FOUND: `source/PressureController.h`
- FOUND: `tests/PressureControllerTest.cpp`
- FOUND: `2939e01`
- FOUND: `79bbd0e`

---
*Phase: 20-pressure-send-state-truth*
*Completed: 2026-07-12*
