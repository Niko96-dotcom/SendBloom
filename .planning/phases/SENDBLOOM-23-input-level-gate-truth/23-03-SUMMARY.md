---
phase: 23-input-level-gate-truth
plan: 03
subsystem: gate-dsp
tags: [juce, NoiseGate, PostHard, ADR-V1-11, CORE-10, CORE-11, CORE-12, CORE-13]

requires:
  - phase: 23-01
    provides: Input truth green
  - phase: 23-02
    provides: Level/Gate Sens truth green
provides:
  - PostHard 0.75 ms linear close ramp (no one-sample snap)
  - [posthard] green; 15 ms chop + PreSoft retained
  - Phase 20–22 greens preserved; shipping-policy still red
affects:
  - Phase 24 reverb/dirt work (consumes gate timing)

tech-stack:
  added: []
  patterns:
    - "ADR-V1-11: deterministic 0.75 ms PostHard close ramp"

key-files:
  created: []
  modified:
    - source/NoiseGate.h
    - tests/PostGateTimingTest.cpp

key-decisions:
  - "Linear close step = 1 / round(0.75ms * sr); attack stays 0.5 ms exponential"
  - "Update 15 ms unit budget to wait for floor, not first isOpen=false sample"

requirements-completed: [CORE-10, CORE-11, CORE-12, CORE-13]

---

# Plan 23-03 Summary

PostHard de-clicked with 0.75 ms ramp; `[posthard]` green without breaking 15 ms chop or PreSoft.
