---
phase: 04-io-gate-correctness
plan: 02
subsystem: audio
tags: [gate, envelope, hysteresis, noise-gate]
requires:
  - phase: 04-io-gate-correctness
    provides: IO stage foundation from plan 01
provides:
  - EnvelopeDetector peak follower
  - NoiseGate PreSoft and PostHard profiles with 3 dB hysteresis
affects: [04-io-gate-correctness, phase-5-reverb]
tech-stack:
  added: []
  patterns: ["constexpr GateProfile drives release/floor", "Hysteresis open/close thresholds"]
key-files:
  created: [source/EnvelopeDetector.h, source/NoiseGate.h, tests/EnvelopeDetectorTest.cpp, tests/NoiseGateTest.cpp]
  modified: []
key-decisions:
  - "PreSoft floor at -80 dB; PostHard hard floor 0"
  - "3 dB hysteresis on threshold per GATE-05"
requirements-completed: [GATE-01, GATE-02, GATE-03, GATE-05]
coverage:
  - id: D1
    description: EnvelopeDetector peak follower with attack/release
    requirement: GATE-01
    verification:
      - kind: unit
        ref: tests/EnvelopeDetectorTest.cpp
        status: pass
    human_judgment: false
  - id: D2
    description: PreSoft hum silencer vs PostHard hard chop profiles
    requirement: GATE-02
    verification:
      - kind: unit
        ref: tests/NoiseGateTest.cpp#PreSoft
        status: pass
    human_judgment: false
  - id: D3
    description: PostHard brutal wet chop profile
    requirement: GATE-03
    verification:
      - kind: unit
        ref: tests/NoiseGateTest.cpp#PostHard
        status: pass
    human_judgment: false
  - id: D4
    description: 3 dB threshold hysteresis
    requirement: GATE-05
    verification:
      - kind: unit
        ref: tests/NoiseGateTest.cpp#hysteresis
        status: pass
    human_judgment: false
duration: 15min
completed: 2026-07-06
status: complete
---

# Phase 4 Plan 02: Envelope + Gate Summary

**EnvelopeDetector peak follower and dual-profile NoiseGate with 3 dB hysteresis**

## Task Commits

1. **Tasks 1-2: Envelope + gate + tests** - `b785b88` (feat)

## Deviations from Plan

None - plan executed exactly as written.

## Self-Check: PASSED

- FOUND: source/EnvelopeDetector.h
- FOUND: source/NoiseGate.h
- FOUND: b785b88
