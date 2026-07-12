---
phase: 22-midi-per-sample-control-delivery
plan: 03
subsystem: realtime-dsp
tags: [juce, ADR-V1-06, RT-06, RT-07, per-sample]

requires:
  - phase: 22-02
    provides: sample-accurate MIDI spans
provides:
  - distnScratch_ / thresholdDbScratch_ filled every sample
  - GatedBloomChain array overload for distn/send/threshold
  - V1ContractPerSampleControlsTest
affects:
  - Phase 23 Input/Level/Gate (consumes same per-sample path)

tech-stack:
  added: []
  patterns:
    - "ADR-V1-06 dynamic control arrays"
    - "Nullable array pointers with scalar fallback overloads"

key-files:
  created:
    - tests/V1ContractPerSampleControlsTest.cpp
  modified:
    - source/GatedBloomChain.h
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp
    - tests/GatedBloomChainTest.cpp

key-decisions:
  - "Keep scalar chain overloads for existing unit tests"
  - "Production processSpan always passes array pointers"

requirements-completed: [RT-06, RT-07]

---

# Plan 03 Summary — Per-Sample Controls

`GatedBloomChain` consumes per-sample distn/send/threshold arrays; processor fills scratches every sample. Input/level/output/bypass remain sample-resolved. RT-06/07 contracts green; MIDI + Phase 20/21 gates still green.
