---
phase: 23-input-level-gate-truth
plan: 01
subsystem: input-dsp-ui
tags: [juce, input, ParameterCurves, ADR-V1-08, CORE-01, CORE-02, CORE-03, CORE-04, CORE-05]

requires:
  - phase: 22-midi-per-sample-control-delivery
    provides: per-sample input/level path stable
provides:
  - Canonical inputGainDb −9/0/+9 via smoothstep
  - Input display formatter calls ParameterCurves::inputGainDb
  - [input-anchors] green
affects:
  - 23-02 Level/Gate Sens display
  - 23-03 PostHard ramp

tech-stack:
  added: []
  patterns:
    - "ADR-V1-08: single canonical Input curve for DSP + UI"

key-files:
  created: []
  modified:
    - source/ParameterCurves.h
    - source/PluginEditor.cpp
    - tests/ParameterCurvesTest.cpp

key-decisions:
  - "inputGainDb = -9 + 18*smoothstep(norm)"
  - "Delete duplicated formatSignedDbFromNorm arithmetic"

requirements-completed: [CORE-01, CORE-02, CORE-03, CORE-04, CORE-05]

---

# Plan 23-01 Summary

Canonical Input −9/0/+9 shared by DSP and Input knob display; `[input-anchors]` green.
