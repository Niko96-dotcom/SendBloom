---
phase: 21-realtime-span-engine-true-bypass
plan: 02
subsystem: realtime-dsp
tags: [juce, true-bypass, ADR-V1-10, BypassCrossfade, CORE-14, CORE-15, CORE-16, CORE-17, CORE-18]

requires:
  - phase: 21-realtime-span-engine-true-bypass
    provides: processSpan engine / [oversized-block] green from 21-01
provides:
  - ADR-V1-10 mix order (OutputStage before bypass crossfade)
  - BypassCrossfade::mixSample per-channel dry×(1−mix)+engaged×mix
  - [true-bypass] green (CORE-14…16,18)
  - CORE-17 click-bounded bypass transitions ([parm][bypass])
affects:
  - 21-03 authentic snapshot edge
  - Phase 22 MIDI / per-sample delivery
  - Phase 23 PostHard / Input (untouched here)

tech-stack:
  added: []
  patterns:
    - "ADR-V1-10: engaged = Output(ParallelWetMixer(...)); final = mixSample(originalDry, engaged, engagedMix)"
    - "Settled bypass dry side is always per-channel original; mono-first only on engaged path"

key-files:
  created: []
  modified:
    - source/PluginProcessor.cpp
    - source/BypassCrossfade.h
    - tests/V1ContractTrueBypassTest.cpp
    - tests/BypassCrossfadeTest.cpp

key-decisions:
  - "OutputStage applies only on engaged path before BypassCrossfade::mixSample (never after)"
  - "extended_stereo off: mono-first dual-mono engaged preserved; bypass dry remains per-channel (CORE-18)"
  - "Added cheap CORE-17 plugin mid-stream bypass toggle click bound (<1.0 adjacent delta) in addition to unit ramp"

patterns-established:
  - "BypassCrossfade::mixSample is the single ADR-V1-10 blend primitive; processBlock helper calls it"
  - "bypassWetScratch_ / engagedMix semantics unchanged: 0=dry/bypass, 1=fully engaged"

requirements-completed: [CORE-14, CORE-15, CORE-16, CORE-17, CORE-18]

coverage:
  - id: D1
    description: "Settled bypass preserves distinct L/R inputs within 1e-6 despite Output +6 dB"
    requirement: CORE-14
    verification:
      - kind: unit
        ref: "Builds/Tests \"[true-bypass]\" (V1ContractTrueBypassTest)"
        status: pass
    human_judgment: false
  - id: D2
    description: "Settled bypass is unity per channel (maxErrL/R == 0)"
    requirement: CORE-15
    verification:
      - kind: unit
        ref: "Builds/Tests \"[true-bypass]\""
        status: pass
    human_judgment: false
  - id: D3
    description: "Output/Input/Distn/Gate/Level ignored when settled bypassed (Output only on engaged)"
    requirement: CORE-16
    verification:
      - kind: unit
        ref: "Builds/Tests \"[true-bypass]\" with outputGain=+6 dB"
        status: pass
    human_judgment: false
  - id: D4
    description: "Bypass transitions click-bounded (5 ms smoother + mid-stream toggle)"
    requirement: CORE-17
    verification:
      - kind: unit
        ref: "Builds/Tests \"[parm][bypass]\""
        status: pass
    human_judgment: false
  - id: D5
    description: "Engaged mono-first dual-mono unchanged when extended_stereo off; L/R remain unequal under bypass"
    requirement: CORE-18
    verification:
      - kind: unit
        ref: "Builds/Tests \"[true-bypass]\" L/R distinctness assert"
        status: pass
    human_judgment: false

duration: 3min
completed: 2026-07-12
status: complete
---

# Phase 21 Plan 02: True Bypass (ADR-V1-10) Summary

**Per-channel true bypass via engaged-then-crossfade mix order — Output gain only on the engaged path; `[true-bypass]` green with maxErrL/R = 0**

## Performance

- **Duration:** 3 min
- **Started:** 2026-07-12T16:18:32Z
- **Completed:** 2026-07-12T16:21:15Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Flipped `[true-bypass]` green: settled bypass reproduces distinct L/R within 1e-6 with Output +6 dB (maxErrL/R = 0.0)
- ADR-V1-10 mix order in `processSpan`: ParallelWetMixer → OutputStage → `BypassCrossfade::mixSample(originalDry, engaged, engagedMix)`
- CORE-17 click coverage: mixSample unit cases, 5 ms ramp adjacent-delta, mid-stream plugin bypass toggle `< 1.0`
- No regressions: `[oversized-block]` and `[pressure-release]` remain green; MIDI/PostHard/Input/shipping untouched

## Task Commits

Each task was committed atomically:

1. **Task 1 (RED): strengthen true-bypass L/R assert** - `5a53868` (test)
2. **Task 1 (GREEN): ADR-V1-10 engaged-then-bypass mix** - `d5de0da` (feat)
3. **Task 2: mixSample + mid-stream click bound** - `062f48c` (test)

**Plan metadata:** (pending docs commit)

## Files Created/Modified

- `source/BypassCrossfade.h` - Added `mixSample`; `processBlock` delegates per sample/channel
- `source/PluginProcessor.cpp` - Fixed `processSpan` writeback mix order (Output before bypass)
- `tests/V1ContractTrueBypassTest.cpp` - L/R distinctness assert under settled bypass
- `tests/BypassCrossfadeTest.cpp` - mixSample units + CORE-17 mid-stream toggle

## Decisions Made

- OutputStage only on engaged path before bypass crossfade (never after) — required for CORE-16
- When `extended_stereo` is off, engaged path stays mono-first dual-mono; bypass dry taps each channel's original sample (CORE-18)
- Chose cheap plugin integration click case for CORE-17 in addition to existing unit ramp (documented)

## Deviations from Plan

None - plan executed exactly as written.

## Issues Encountered

None

## Known Stubs

None — no placeholder/TODO stubs in modified files that block plan goal.

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Plan 03 can switch authentic request to snapshot-edge (ADR-V1-07) without reopening bypass mix order
- MIDI purity / per-sample delivery remain Phase 22

## Proof: `[true-bypass]` green

```
REQUIRE maxErrL < 1e-6  →  0.0f < 0.000001f  PASSED
REQUIRE maxErrR < 1e-6  →  0.0f < 0.000001f  PASSED
REQUIRE |L-R| > 1e-3    →  0.58 > 0.001      PASSED
All tests passed (4 assertions in 1 test case)
```

Also green: `Builds/Tests "[parm][bypass]"` (8 assertions / 5 cases), `"[oversized-block]"`, `"[pressure-release]"`.

## Self-Check: PASSED

- FOUND: source/BypassCrossfade.h, source/PluginProcessor.cpp, tests/V1ContractTrueBypassTest.cpp, tests/BypassCrossfadeTest.cpp
- FOUND commits: 5a53868, d5de0da, 062f48c
- FOUND: `[true-bypass]` all assertions pass with maxErr = 0

---
*Phase: 21-realtime-span-engine-true-bypass*
*Completed: 2026-07-12*
