---
phase: 20-pressure-send-state-truth
plan: 02
subsystem: ui
tags: [PressureSendPad, release-to-zero, overlay, AdvancedDrawer, pressure-release, Catch2]

requires:
  - phase: 20-pressure-send-state-truth
    provides: PressureController DSP truth (connected-at-rest / asymmetric smooth)
provides:
  - Pad mouseUp release-to-zero with send_connected kept true
  - Footswitch pressed overlay keyed on press/amount (not connection alone)
  - Advanced PRESSURE MODE toggle bound to send_connected
  - [v1][contract][pressure-release] green
affects:
  - 20-03 preset matrix / Soft Pressure factory resting state
  - Phase 25 branding (PRESSURE MODE copy stays generic)

tech-stack:
  added: []
  patterns:
    - "Pad amount edits bracketed with beginChangeGesture/endChangeGesture"
    - "Overlay predicate shouldDrawFootswitchPressedOverlay(pressed, display, amount)"

key-files:
  created:
    - tests/FootswitchOverlayPredicateTest.cpp
  modified:
    - source/ui/PressureSendPad.h
    - source/ui/PressureSendPad.cpp
    - source/ui/PedalFaceplatePaint.h
    - source/ui/PedalFaceplatePaint.cpp
    - source/ui/AdvancedDrawer.h
    - source/ui/AdvancedDrawer.cpp
    - source/PluginEditor.cpp
    - tests/V1ContractPressureReleaseTest.cpp

key-decisions:
  - "mouseUp never writes send_connected false; zeros amount with gesture bracketing"
  - "Overlay uses pad isPressed/displayAmount plus APVTS amount epsilon — not connection"
  - "PRESSURE MODE label avoids third-party controller product names"
  - "Settle ~20 blocks after release before energy assert (ADR-V1-04 / SEND-10)"

patterns-established:
  - "Pattern: release-to-zero keeps connection; bloom fade is display-only"
  - "Pattern: Advanced ButtonAttachment → ParameterIDs::sendConnected"

requirements-completed: [SEND-05, SEND-06, SEND-07, SEND-08]

coverage:
  - id: D1
    description: Pad release zeros amount and keeps send_connected true (SEND-05/08)
    requirement: SEND-05
    verification:
      - kind: unit
        ref: Builds/Tests "[pressure-release]"
        status: pass
    human_judgment: false
  - id: D2
    description: Footswitch pressed overlay follows press/amount not connection (SEND-06)
    requirement: SEND-06
    verification:
      - kind: unit
        ref: tests/FootswitchOverlayPredicateTest.cpp
        status: pass
    human_judgment: false
  - id: D3
    description: Advanced PRESSURE MODE toggle bound to send_connected (SEND-07)
    requirement: SEND-07
    verification:
      - kind: unit
        ref: Builds/Tests "[send]"
        status: pass
    human_judgment: true
    rationale: "ButtonAttachment wiring verified by build + [send]; visual placement needs UI-SPEC glance later"
  - id: D4
    description: Unrelated Phase 19 contracts remain intentionally red
    verification:
      - kind: unit
        ref: Builds/Tests "[v1][contract][oversized-block]" (expect fail)
        status: pass
      - kind: unit
        ref: Builds/Tests "[v1][contract][midi-apvts]" (expect fail)
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-07-12
status: complete
---

# Phase 20 Plan 02: Pad release / overlay / Advanced toggle Summary

**Pad release-to-zero keeps `send_connected` true; pressed overlay follows press/amount; Advanced exposes PRESSURE MODE; `[pressure-release]` is green.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-07-12T15:49:30Z
- **Completed:** 2026-07-12T15:55:30Z
- **Tasks:** 2
- **Files modified:** 9

## Accomplishments

- `PressureSendPad::mouseUp` zeros `send_amount` with gesture bracketing and never disconnects (SEND-05/08)
- Faceplate footswitch overlay uses `shouldDrawFootswitchPressedOverlay` (press/display/amount) — not `send_connected` alone (SEND-06)
- Advanced drawer `PRESSURE MODE` ToggleButton → `ParameterIDs::sendConnected` via ButtonAttachment (SEND-07)
- `Builds/Tests "[pressure-release]"` green; `[oversized-block]` and `[midi-apvts]` still fail

## Task Commits

Each task was committed atomically:

1. **Task 1: Pad release-to-zero + auto-connect** - `72dcfbd` (feat)
2. **Task 2: Overlay + Advanced PRESSURE MODE** - `108a774` (feat)

**Plan metadata:** (docs commit after state update)

## Files Created/Modified

- `source/ui/PressureSendPad.h/.cpp` - release-to-zero, gesture bracketing, public isPressed/getDisplayAmount
- `source/ui/PedalFaceplatePaint.h/.cpp` - overlay predicate + pad state args on paintPedalFaceplate
- `source/ui/AdvancedDrawer.h/.cpp` - PRESSURE MODE toggle + sendConnected attachment; height 204
- `source/PluginEditor.cpp` - pass pad state into paint; AdvancedDrawer ctor arg
- `tests/V1ContractPressureReleaseTest.cpp` - settle SEND-10 release before energy probe
- `tests/FootswitchOverlayPredicateTest.cpp` - SEND-06 predicate unit coverage

## Decisions Made

- Gesture-bracket amount on down→drag→up; bloom fade remains display-only after APVTS zero
- Prefer pad pressed/displayAmount for overlay, with APVTS amount as additional epsilon input
- Label copy is `PRESSURE MODE` (no third-party product names; full branding Phase 25)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Energy assert raced SEND-10 25 ms release**
- **Found during:** Task 1 (after mouseUp production fix)
- **Issue:** With correct connected-at-rest, one 512-sample block (~10.7 ms) still carried wet energy because PressureController release τ is 25 ms — assertions on connected/amount already passed
- **Fix:** `processToneBlocks(plugin, 20)` after release before the dry energy probe; thresholds unchanged (ADR-V1-04 alignment, not relaxed semantics)
- **Files modified:** `tests/V1ContractPressureReleaseTest.cpp`
- **Verification:** `Builds/Tests "[pressure-release]"` → All tests passed (8 assertions)
- **Committed in:** `72dcfbd` (Task 1)

---

**Total deviations:** 1 auto-fixed (1 bug / measurement window)
**Impact on plan:** Required to flip pressure-release green without breaking SEND-10; no scope creep into other Phase 19 contracts.

## Issues Encountered

None beyond the SEND-10 settle-window alignment above.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- UI half of Pressure Mode honesty ready for 20-03 preset matrix / Soft Pressure resting state
- Do not touch MIDI APVTS store (Phase 22) or span/no-alloc (Phase 21)

## Self-Check: PASSED

- FOUND: `source/ui/PressureSendPad.cpp`
- FOUND: `source/ui/PedalFaceplatePaint.cpp`
- FOUND: `source/ui/AdvancedDrawer.cpp`
- FOUND: `tests/FootswitchOverlayPredicateTest.cpp`
- FOUND: `72dcfbd`
- FOUND: `108a774`
- FOUND: `Builds/Tests "[pressure-release]"` green

---
*Phase: 20-pressure-send-state-truth*
*Completed: 2026-07-12*
