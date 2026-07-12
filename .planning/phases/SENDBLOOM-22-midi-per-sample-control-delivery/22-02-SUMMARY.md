---
phase: 22-midi-per-sample-control-delivery
plan: 02
subsystem: midi-dsp
tags: [juce, midi, sample-accurate, RT-04, MIDI-04, MIDI-05, MIDI-06, MIDI-09, MIDI-10]

requires:
  - phase: 22-01
    provides: MIDI purity / PressureController midi target
provides:
  - Span cuts at next CC1 (RT-04)
  - Sample-position CC1 apply at span start
  - V1ContractMidiSampleAccurateTest suite
affects:
  - 22-03 per-sample control arrays

tech-stack:
  added: []
  patterns:
    - "span = min(remaining, preparedMaxBlock_, kControlQuantum, nextCc1 - offset)"
    - "applyCc1AtSample before each span"

key-files:
  created:
    - tests/V1ContractMidiSampleAccurateTest.cpp
  modified:
    - source/PluginProcessor.cpp
    - source/PluginProcessor.h

key-decisions:
  - "CC1 ignored when disconnected"
  - "Last CC1 at same sample wins"
  - "EnergyTrackingReverb proves sample windows"

requirements-completed: [MIDI-04, MIDI-05, MIDI-06, MIDI-09, MIDI-10, RT-04]

---

# Plan 02 Summary — Sample-Accurate MIDI

MIDI cursor applies CC1 at span offsets and cuts spans before the next CC1. Sample-accurate, ordered, release-to-zero, non-CC1, and block-size finite contracts green. Phase 20/21 regressions preserved.
