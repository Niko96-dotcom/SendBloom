# Cross-Agent Synthesis — Pre-Code Research Complete

**Date:** 2026-07-06  
**Investigators:** **8/8 lanes complete.**

---

## Unified stack decision

| Layer | Source | Action |
|-------|--------|--------|
| **Scaffold** | [pamplejuce](8b58ec7c-bdae-454d-9e85-a67e69c92932) | GitHub **Use this template** (not fork); add APVTS from [lupex-analog-delay](8b58ec7c-bdae-454d-9e85-a67e69c92932) |
| **CI / tests** | [pamplejuce](9fe2e368-911b-4083-b561-4e6c10071a6c) + [melatonin_test_helpers](9fe2e368-911b-4083-b561-4e6c10071a6c) | Copy `build_and_test.yml` + `Tests.cmake`; add custom RT60 test (no public repo has this) |
| **Params** | [melatonin_parameters](f28d6588-f01e-4387-992e-586c8c931227) + JUCE `DryWetMixer` | `logarithmicRange` for size; `decibelRange` for distn; `squareRoot3dB` for level; **split** `INPUT_GAIN` + `INPUT_THRESHOLD` APVTS (not one dual param) |
| **Smoothing** | [aesuf/JUCE-Plugin-Template](f28d6588-f01e-4387-992e-586c8c931227) | Per-sample `getNextValue()` discipline |
| **Reverb core** | [CloudSeedCore](bf380fad-e04c-4018-9a1f-1b5d03b59cda) MIT + [Jatin FDN RT60 math](bf380fad-e04c-4018-9a1f-1b5d03b59cda) | MIT commercial path; steal `calcGainForT60` / `UpdateLines` |
| **Gate** | [Noise gate repos](9fb7b9be-e9b2-4601-b2e3-d6f79206051c) + BYOD `Gate.cpp` | **Two instances** in chain: soft pre (LSP Expander / GateXpander concepts) + hard post (LSP Gate / airwindows DigitalBlack); pre-detect on input, post-apply after wet OD (PointyGuitar precedent) |
| **Wet OD** | [reverb_trickery](49d86a4b-9fd6-422a-a8dd-27d78cbe981f) reorder + greenfield clipper | `wetPath = verb → distort → gate` (BSD Faust graph reorder) |
| **Send / Igor UI** | [orbit-looper](eb83ae66-59c5-4af0-9dc0-bcde506dc6e6) + [RoomReverb send bus](eb83ae66-59c5-4af0-9dc0-bcde506dc6e6) + [IEM SpherePanner](eb83ae66-59c5-4af0-9dc0-bcde506dc6e6) | Greenfield `PressureSendPad`; [Gin MidiLearn](eb83ae66-59c5-4af0-9dc0-bcde506dc6e6) for CC |
| **Product gap** | [Gated dirty reverb landscape](54171b78-9bde-4cac-b5ef-ab775048ec05) | No OSS ≥7/10; routing is the moat |

---

## Revised ADR notes

- **ADR-001:** Prefer **pamplejuce template** over raw fork; pin JUCE 8.0.12+; C++20 not 23.
- **ADR-002:** Primary MIT engine candidate is **GhostNoteAudio/CloudSeedCore**, not only chowdsp SimpleReverb (GPL module risk). Fallback: greenfield 8-line FDN + Jatin `calcGainForT60`.

---

## MB unblocked — ready for MB-002

All research gates in `BUILD_MICROSTEPS.md` are satisfied (R3 confirms dual-instance pattern; no single OSS repo has the full chain).

**Do not write plugin `Source/` until:** product name chosen (MB-000) + ADR-001/002 approved.

**User dossier ingested:** see `RESEARCH_ADDENDUM_USER_DOSSIER.md` — confirms topology-first approach; adds FV-1 Schroeder detail, Authentic/Extended modes, legal no-EEPROM rule, DV-1/claytonyen competitive refs.
