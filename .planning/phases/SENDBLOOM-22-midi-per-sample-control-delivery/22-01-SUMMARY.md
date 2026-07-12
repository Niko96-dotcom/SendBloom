---
phase: 22-midi-per-sample-control-delivery
plan: 01
subsystem: midi-dsp
tags: [juce, midi, CC1, ADR-V1-03, MIDI-01, MIDI-02, MIDI-03, MIDI-07, MIDI-08]

requires:
  - phase: 21-realtime-span-engine-true-bypass
    provides: span engine + PressureController host wiring
provides:
  - MIDI CC1 → PressureController::setMidiPressureTarget (no APVTS store)
  - Removed Phase 20 forced midiTarget=0 each block
  - [midi-apvts] green; MidiSendAmountTest + ReleaseTruth MIDI updated to ADR-V1-03
affects:
  - 22-02 sample-accurate span cuts
  - 22-03 per-sample distn/threshold arrays

tech-stack:
  added: []
  patterns:
    - "ADR-V1-03: MIDI is realtime modulation only"
    - "max(host, midi) via PressureController"

key-files:
  created: []
  modified:
    - source/PluginProcessor.cpp
    - tests/MidiSendAmountTest.cpp
    - tests/V1ContractMidiApvtsPurityTest.cpp
    - tests/ReleaseTruthTest.cpp

key-decisions:
  - "Delete sendParam->store MIDI path"
  - "Persist midiTarget across blocks"
  - "Update MidiSendAmountTest / ReleaseTruth MIDI to purity contract"

requirements-completed: [MIDI-01, MIDI-02, MIDI-03, MIDI-07, MIDI-08]

---

# Plan 01 Summary — MIDI Purity

Removed APVTS mutation for CC1. Connected CC1 updates `PressureController` MIDI target only; host/UI combine via `max`. `[midi-apvts]` green; pressure-release regression green.
