---
phase: 23-input-level-gate-truth
plan: 02
subsystem: level-gate-ui
tags: [juce, level, gate-sens, ADR-V1-09, CORE-06, CORE-07, CORE-08, CORE-09]

requires:
  - phase: 23-01
    provides: ParameterCurves Input anchors stable
provides:
  - Wet-only Level (dry=1, wet=sin(halfPi*norm))
  - Removed dryGain / levelDryGain / processSpan dry drain
  - Gate Sens advanced display via inputThresholdDb
affects:
  - 23-03 PostHard verification regressions

tech-stack:
  added: []
  patterns:
    - "ADR-V1-09: Level scales wet return only"

key-files:
  created: []
  modified:
    - source/ParameterCurves.h
    - source/ParameterSnapshot.h
    - source/SmoothedParameterBank.h
    - source/PluginProcessor.cpp
    - source/ui/AdvancedDrawer.cpp
    - tests/ParameterSnapshotTest.cpp
    - tests/ParallelWetMixerTest.cpp

key-decisions:
  - "levelEqualPower dry fixed at 1.0f"
  - "Gate Sens keeps input_threshold ID; shows threshold dB"

requirements-completed: [CORE-06, CORE-07, CORE-08, CORE-09]

---

# Plan 23-02 Summary

Wet-only Level with dead dry-gain state removed; Gate Sens shows canonical threshold dB.
