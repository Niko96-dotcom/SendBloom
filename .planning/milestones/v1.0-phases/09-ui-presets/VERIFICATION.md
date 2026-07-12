# Phase 9 Verification

**Phase:** UI + Presets  
**Date:** 2026-07-06  
**Status:** `human_needed`

## Automated Gates

| Gate | Result | Details |
|------|--------|---------|
| Catch2 unit tests | PASS | **96/96** test cases (+8 from Phase 8) |
| Release build | PASS | AU + VST3 |
| pluginval strictness 7 | PASS | Local run on Release VST3 |
| Pedal UI editor smoke | PASS | 340×520 headless instantiation |
| Factory presets (PRST-01) | PASS | 8 programs + embedded XML resources |
| Preset round-trip (PRST-02) | PASS | 6 preset tests including all-8 state round-trip |
| Clip LED atomic flag | PASS | `isClipHoldActive()` smoke test |

## Requirements Covered

| ID | Status | Evidence |
|----|--------|----------|
| UI-01 | PASS | 5 knobs + Dark + Gate toggles in `PluginEditor` |
| UI-02 | PASS | `PressureSendPad` down-drag-up with bloom visual |
| UI-03 | PASS | `ClipLed` polls `clipHoldFlag` at 30 Hz |
| UI-04 | PASS | `AdvancedDrawer` with Gate Sens, Send Feel, 32k Color; Extended Stereo + Dirt OS disabled |
| UI-05 | PASS | Guitarist copy only; no graphs or algorithm language |
| PRST-01 | PASS | 8 factory presets in `FactoryPresets` + `resources/presets/*.xml` |
| PRST-02 | PASS | `tests/PresetTest.cpp` round-trip proofs |

## Test Counts

| Suite | Count |
|-------|-------|
| Total ctest | **96/96** |
| Preset (new) | 6 |
| PluginEditor (new) | 2 |
| Delta from Phase 8 | +8 |

## Human DAW Smoke

**Status:** Deferred — executor cannot audition interactive UI in host.

Follow README **Phase 9 — UI + Presets DAW Smoke** and reply `approved` when complete.

**Checklist:**

1. Load SendBloom; confirm pedal layout (In, Size, Lvl, Distn, Out + Dark + Gate toggles).
2. Press and drag on pressure pad — bloom visual and send response.
3. Drive input hot — CLIP LED lights during clip hold.
4. Open Advanced drawer — Gate Sens, Send Feel, 32k Color active; Extended Stereo / Dirt OS greyed out.
5. Load each of 8 factory presets from menu; audition briefly.
6. Save user preset in host; reload and confirm parameters match.

## Known Limitations

- Extended Stereo and Dirt OS shown disabled (post-v1 per CONTEXT.md)
- pluginval 7 also configured in CI from Phase 8
