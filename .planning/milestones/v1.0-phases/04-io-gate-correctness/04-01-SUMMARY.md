---
phase: 04-io-gate-correctness
plan: 01
subsystem: audio
tags: [juce, input-stage, output-stage, io, soft-clip]
requires:
  - phase: 03-ugly-signature-chain
    provides: ParameterCurves gain mappings and processor integration pattern
provides:
  - InputStage with soft clip and 50 ms clip-hold flag
  - OutputStage output trim helper
affects: [04-io-gate-correctness, phase-9-ui]
tech-stack:
  added: []
  patterns: ["Header-only IO stages; clip-hold counter in samples"]
key-files:
  created: [source/InputStage.h, source/OutputStage.h, tests/InputStageTest.cpp, tests/OutputStageTest.cpp, tests/MonoBusTest.cpp]
  modified: []
key-decisions:
  - "Tanh soft clip knee at -3 dBFS per BUILD_MICROSTEPS MB-028"
  - "Clip-hold clears when counter reaches zero after 50 ms"
patterns-established:
  - "InputStage owns gain+clip; dry tap stays pre-gain in processor"
requirements-completed: [IO-01, IO-02, IO-03]
coverage:
  - id: D1
    description: InputStage gain, soft clip, and 50 ms clip-hold flag
    requirement: IO-01
    verification:
      - kind: unit
        ref: tests/InputStageTest.cpp
        status: pass
    human_judgment: false
  - id: D2
    description: OutputStage applies output gain trim
    requirement: IO-02
    verification:
      - kind: unit
        ref: tests/OutputStageTest.cpp
        status: pass
    human_judgment: false
  - id: D3
    description: Mono bus stereo-to-mono sum contract
    requirement: IO-03
    verification:
      - kind: unit
        ref: tests/MonoBusTest.cpp
        status: pass
    human_judgment: false
duration: 12min
completed: 2026-07-06
status: complete
---

# Phase 4 Plan 01: IO Stages Summary

**InputStage soft clip with 50 ms clip-hold and OutputStage gain trim with mono bus tests**

## Task Commits

1. **Task 1-2: IO stages + tests** - `793e77d` (feat)

## Deviations from Plan

None - plan executed exactly as written.

## Self-Check: PASSED

- FOUND: source/InputStage.h
- FOUND: source/OutputStage.h
- FOUND: 793e77d
