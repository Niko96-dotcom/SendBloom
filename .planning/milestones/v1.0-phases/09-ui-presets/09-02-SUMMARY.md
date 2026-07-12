---
phase: 09-ui-presets
plan: 02
subsystem: ui
tags: [pressure-pad, clip-led, advanced-drawer]
requires:
  - phase: 09-01
    provides: Pedal shell
provides:
  - PressureSendPad with bloom visual
  - Clip LED from InputStage
  - Advanced drawer with disabled post-v1 toggles
affects: [09-03]
tech-stack:
  added: []
  patterns: [atomic clip flag, pad down-drag-up]
key-files:
  created: [source/ui/PressureSendPad.h, source/ui/PressureSendPad.cpp, source/ui/ClipLed.h, source/ui/AdvancedDrawer.h, source/ui/AdvancedDrawer.cpp]
  modified: [source/PluginProcessor.h, source/PluginProcessor.cpp, source/PluginEditor.h, source/PluginEditor.cpp]
key-decisions:
  - "clipHoldFlag updated once per block from InputStage"
  - "Extended Stereo and Dirt OS disabled with Coming soon tooltip"
requirements-completed: [UI-02, UI-03, UI-04]
duration: 20min
completed: 2026-07-06
status: complete
---

# Phase 9 Plan 02: Pressure Pad + Clip LED + Drawer Summary

**Interactive pad, real-time clip LED, and collapsible advanced controls**

## Performance

- **Duration:** ~20 min
- **Tasks:** 1/1
- **Files modified:** 9

## Accomplishments

- `PressureSendPad`: pointer down connects send, vertical drag sets amount, release disconnects, radial bloom paint
- `ClipLed`: 30 Hz timer polls `PluginProcessor::isClipHoldActive()`
- `AdvancedDrawer`: Gate Sens, Send Feel, 32k Color; Extended Stereo / Dirt OS disabled
- `std::atomic<bool> clipHoldFlag` on processor

## Test Counts

| Suite | Count |
|-------|-------|
| PluginEditor smoke | 2 |
| Clip flag smoke | 1 |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] clipHoldFlag per-sample store caused SIGTRAP in stress test**
- **Found during:** Task 1 verification
- **Issue:** `clipHoldFlag.store` inside per-sample loop correlated with authentic-color stress SIGTRAP
- **Fix:** Move store to once per block after sample loop
- **Files modified:** `source/PluginProcessor.cpp`

## Self-Check: PASSED
