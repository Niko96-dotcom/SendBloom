# Phase 9: UI + Presets — Research

**Researched:** 2026-07-06  
**Requirements:** UI-01–UI-05, PRST-01, PRST-02  
**Status:** Ready for planning

---

## Summary

Phase 9 replaces the minimal Phase 1 `PluginEditor` shell with a guitarist-facing stompbox UI and wires 8 factory presets through JUCE's `AudioProcessor` program API + embedded XML resources. All controls bind via `AudioProcessorValueTreeState::SliderAttachment` / `ButtonAttachment` / `ComboBoxAttachment`. Clip LED requires a thread-safe flag from `InputStage` exposed on `PluginProcessor`. Pressure pad is a custom `Component` writing `send_connected` and `send_amount` on the message thread.

**Primary recommendation:** Custom `SendBloomLookAndFeel` + composable UI components in `source/ui/`; `FactoryPresets` module with `juce_add_binary_data` for 8 XML preset blobs; extend existing APVTS state round-trip test pattern for PRST-02.

---

## Standard Stack

| Layer | Choice | Rationale |
|-------|--------|-----------|
| UI framework | JUCE 7 `juce_gui_basics` | Already linked; AU/VST3 editor |
| Parameter binding | APVTS attachments | Phase 2 pattern; host automation |
| Preset storage | Embedded XML + `getNumPrograms` | PRST-01 bundle resources; host-compatible |
| Testing | Catch2 + `PluginEditor` headless | `JUCE_MODAL_LOOPS_PERMITTED=1` in Tests.cmake |
| Validation | pluginval 7 (CI) | Phase 8 gate pattern |

---

## Architecture

### Component Tree

```
PluginEditor (340×520)
├── titleLabel ("SendBloom")
├── presetComboBox → FactoryPresets
├── PedalKnob ×5 (In, Size, Lvl, Distn, Out)
├── darkToggle, gateToggle
├── ClipLed
├── PressureSendPad
├── advancedToggleButton
└── AdvancedDrawer (Gate Sens, Send Feel, 32k Color, disabled toggles)
```

### Clip LED Thread Safety

`InputStage::isClipHoldActive()` is updated on audio thread. Mirror to `std::atomic<bool> clipHoldFlag` in `PluginProcessor::processBlock` after `inputStage.processSample`. Editor timer reads atomic — no audio-thread UI calls.

### PressureSendPad Behavior

Matches Phase 7 MIDI CC1 semantics: pad down = connected, vertical drag = amount. On release, disconnect (`send_connected=false`). Visual bloom: `paint()` radial gradient centered on last touch, alpha from `send_amount`.

### Preset XML Format

Uses existing `apvts.copyState().createXml()` output (`<SendBloomParams ...>`). Factory presets generated at build time or hand-authored with normalized parameter attributes matching `ParameterIDs`.

### Factory Preset Names (8)

| # | Name | Character |
|---|------|-----------|
| 0 | Sparkle Verb | Bright, short gate, low distn |
| 1 | Cut Sample Gate | Tight post gate, high threshold |
| 2 | Spacerock Burn | Large size, med distn, dark |
| 3 | Dry Dub Sends | Low level, firm send feel |
| 4 | Dark Bloom | dark_mode on, med size |
| 5 | Firm Pressure | send connected default, firm feel |
| 6 | Gated Room | post gate, long size |
| 7 | Hot Clip | high input gain, hot send |

---

## APVTS Parameter Map

| UI Label | ID | Type | Range |
|----------|-----|------|-------|
| In | `input_gain` | float | 0–1 |
| Gate Sens | `input_threshold` | float | 0–1 skewed |
| Size | `size` | float | 0–1 skewed |
| Lvl | `level` | float | 0–1 |
| Distn | `distn` | float | 0–1 |
| Out | `output_gain` | float | -12..+12 dB |
| Dark | `dark_mode` | bool | |
| Gate Pre/Post | `gate_pre_post` | choice | PreSoft/PostHard |
| Send (pad) | `send_connected` | bool | |
| Send (pad) | `send_amount` | float | 0–1 |
| Send Feel | `send_feel` | choice | Firm/Soft |
| 32k Color | `authentic_color` | bool | |
| Extended Stereo | `extended_stereo` | bool | disabled in UI |
| Dirt OS | `dirt_os` | bool | disabled in UI |

---

## Existing Code to Reuse

| Asset | Path | Use |
|-------|------|-----|
| APVTS layout | `source/ParameterLayout.cpp` | All attachments |
| Clip hold | `source/InputStage.h` | LED source |
| Send curves | `source/ParameterCurves.h` | Pad visual scaling |
| State round-trip | `tests/PluginBasics.cpp` | PRST-02 pattern |
| Test helpers | `tests/helpers/test_helpers.h` | Editor tests |

---

## Implementation Notes

### CMake Binary Data

```cmake
juce_add_binary_data(SendBloomPresets SOURCES resources/presets/*.xml)
target_link_libraries(SharedCode INTERFACE SendBloomPresets)
```

### pluginval 7

Already configured in CI from Phase 8. Phase 9 adds UI components — no new pluginval config needed unless editor size triggers validation; run locally if `pluginval.app` present.

### Human DAW Smoke

Executor defers interactive UI + preset audition to `human_needed` in VERIFICATION.md per Phase 8 pattern.

---

## Risks

| Risk | Mitigation |
|------|------------|
| Pad drag on trackpad vs mouse | Use `MouseEvent::getDistanceFromDragStartY()` inverted |
| Preset XML version drift | Round-trip test all 8 presets |
| Editor instantiation in CI | Headless test with `ScopedJuceInitialiser_GUI` |

---

## Plan Suggestions

| Plan | Scope | Requirements |
|------|-------|--------------|
| 09-01 | Pedal shell, LookAndFeel, 5 knobs, toggles, preset combo | UI-01, UI-05 |
| 09-02 | PressureSendPad, ClipLed, AdvancedDrawer | UI-02, UI-03, UI-04 |
| 09-03 | Factory presets XML, program API, round-trip tests, VERIFICATION | PRST-01, PRST-02 |

---

## Sources

- `.planning/phases/09-ui-presets/09-CONTEXT.md`
- `.planning/phases/09-ui-presets/09-UI-SPEC.md`
- `source/PluginEditor.cpp` (minimal shell)
- `source/ParameterLayout.cpp`
- JUCE APVTS attachment docs (in-tree `juce_audio_processors`)
