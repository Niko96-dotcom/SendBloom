---
phase: 21-realtime-span-engine-true-bypass
plan: 03
subsystem: realtime-dsp
tags: [juce, authentic-color, ADR-V1-07, EngineCrossfade, RT-08, RT-09, RT-10, RT-11, RT-12, RT-13, RT-14]

requires:
  - phase: 21-realtime-span-engine-true-bypass
    provides: processSpan engine / oversized wet parity (21-01) + true bypass mix order (21-02)
provides:
  - ADR-V1-07 authentic snapshot-edge requestEngineCrossfade
  - requestedAuthenticColor_ once-per-change semantics (RT-08…11)
  - Latency stays 0 across authentic transitions (RT-09 / Path B)
  - RT-14 10k stress with authentic + bypass + oversized finite/peak
affects:
  - Phase 22 MIDI / per-sample delivery
  - Phase 21 verification / ship gate

tech-stack:
  added: []
  patterns:
    - "Authentic crossfade: processBlock snapshot inequality vs requestedAuthenticColor_ → one requestEngineCrossfade"
    - "Span authentic latch from ParameterSnapshot, not authenticColorTarget 0.5 threshold"

key-files:
  created:
    - tests/V1ContractAuthenticTransitionTest.cpp
  modified:
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp
    - source/GatedBloomChain.h
    - tests/RealtimeStressTest.cpp

key-decisions:
  - "Block-start snapshot edge replaces 15 ms authenticColorTarget 0.5 threshold for crossfade requests"
  - "Left authenticColorTarget smoother in SmoothedParameterBank; drained but unused as request trigger"
  - "Exposed GatedBloomChain::isCrossfading() for production-observable RT-08/10/11 asserts"

patterns-established:
  - "requestedAuthenticColor_ initialized in prepareToPlay from APVTS; updated only after request"
  - "Idle-engine reset on crossfade completion remains in SchroederTank32 (unchanged)"

requirements-completed: [RT-08, RT-09, RT-10, RT-11, RT-12, RT-13, RT-14]

coverage:
  - id: D1
    description: "Single authentic_color change requests one engine crossfade in the first subsequent processBlock"
    requirement: RT-08
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][authentic]\" (RT-08/RT-10)"
        status: pass
    human_judgment: false
  - id: D2
    description: "getLatencySamples() stays 0 before/during/after authentic toggles"
    requirement: RT-09
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][authentic][RT-09]\" && Builds/Tests \"[chain][latency]\""
        status: pass
    human_judgment: false
  - id: D3
    description: "Crossfade begins within the first processBlock after authentic snapshot change"
    requirement: RT-10
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][authentic][RT-08][RT-10]\""
        status: pass
    human_judgment: false
  - id: D4
    description: "Rapid authentic toggles converge to final snapshot target after settle"
    requirement: RT-11
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][authentic][RT-11]\""
        status: pass
    human_judgment: false
  - id: D5
    description: "Crossfade completion resets only the idle engine; no new heap in process path"
    requirement: RT-12
    verification:
      - kind: other
        ref: "SchroederTank32 idle reset unchanged; authentic contracts + existing EngineCrossfade green"
        status: pass
    human_judgment: false
  - id: D6
    description: "No new heap tokens on authentic request / completion paths"
    requirement: RT-13
    verification:
      - kind: other
        ref: "No new allocations in PluginProcessor authentic edge; EngineCrossfade retargets in place"
        status: pass
    human_judgment: false
  - id: D7
    description: "10k-block stress with authentic, bypass, and oversized host blocks stays finite with peak < 4"
    requirement: RT-14
    verification:
      - kind: integration
        ref: "Builds/Tests \"[realtime][stress][RT-14]\""
        status: pass
    human_judgment: false

duration: 8min
completed: 2026-07-12
status: complete
---

# Phase 21 Plan 03: Authentic Snapshot Edge & 10k Stress Summary

**ADR-V1-07 authentic snapshot-edge crossfade requests with RT-08…14 contracts and strengthened authentic/bypass/oversized 10k stress**

## Performance

- **Duration:** 8 min
- **Started:** 2026-07-12T16:22:38Z
- **Completed:** 2026-07-12T16:30:30Z
- **Tasks:** 2
- **Files modified:** 5

## Accomplishments

- Replaced authenticColorTarget 0.5 smoother-edge detection with `requestedAuthenticColor_` snapshot inequality at processBlock start
- Proved first-block crossfade start, latency 0, and rapid-toggle convergence via `[v1][contract][authentic]`
- Extended RealtimeStressTest with RT-14 case: prepare 512, ≥10k blocks including 1024/2048, periodic authentic + bypass toggles, finite + peak < 4
- Confirmed midi-apvts and shipping-policy contracts remain intentionally red

## Task Commits

Each task was committed atomically:

1. **Task 1 RED: Authentic snapshot-edge contracts** - `c702c00` (test)
2. **Task 1 GREEN: Snapshot-edge authentic requests** - `1d85584` (feat)
3. **Task 2: Strengthen 10k stress (RT-14)** - `9b2fba8` (test)

**Plan metadata:** (pending docs commit)

_Note: TDD Task 1 used test → feat; Task 2 was test-only coverage._

## Files Created/Modified

- `tests/V1ContractAuthenticTransitionTest.cpp` - RT-08/09/10/11 authentic transition contracts
- `source/GatedBloomChain.h` - `isCrossfading()` forwarder for observable asserts
- `source/PluginProcessor.h` - `requestedAuthenticColor_` replaces `lastAuthenticColorSmoothed_`
- `source/PluginProcessor.cpp` - block-start snapshot edge + snap-latched span authentic
- `tests/RealtimeStressTest.cpp` - RT-14 authentic/bypass/oversized 10k stress

## Decisions Made

- Authentic requests fire once per differing APVTS snapshot at block start (ADR-V1-07), not when the 15 ms smoother crosses 0.5
- Kept `authenticColorTarget` member/prepare wiring but stopped using it as the request trigger; still drained in the span loop to avoid smoother backlog
- Exposed `GatedBloomChain::isCrossfading()` rather than adding a call-counter test seam

## Deviations from Plan

None - plan executed exactly as written.

SmoothedParameterBank.h was listed in plan `files_modified` but left unchanged (discretion: unused smoother members may remain; they must not drive requests — satisfied).

## Issues Encountered

None

## User Setup Required

None - no external service configuration required.

## Next Phase Readiness

- Phase 21 RT-08…14 + CORE-14…18 + oversized contracts green
- Phase 19 midi-apvts / shipping-policy (and other deferred contracts) still red by policy
- Ready for phase verification (`/gsd-verify-work`) then Phase 22 MIDI / per-sample delivery

## Self-Check: PASSED

- FOUND: tests/V1ContractAuthenticTransitionTest.cpp
- FOUND: source/PluginProcessor.cpp requestedAuthenticColor_ edge
- FOUND: tests/RealtimeStressTest.cpp RT-14 case
- FOUND: c702c00, 1d85584, 9b2fba8

---
*Phase: 21-realtime-span-engine-true-bypass*
*Completed: 2026-07-12*
