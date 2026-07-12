---
phase: 21-realtime-span-engine-true-bypass
plan: 01
subsystem: realtime-dsp
tags: [juce, processBlock, span, no-alloc, oversized-block, RT-01, RT-02, RT-03, RT-05, RT-15]

requires:
  - phase: 20-pressure-mode-truth
    provides: PressureController wiring / pressure-release green baseline
provides:
  - No-alloc span processBlock with kControlQuantum=128
  - processSpan private path; dry oversized fallback removed
  - IntegrationAllocScan bans .setSize in process bodies
  - RT-05 / RT-15 realtime span contracts
  - [oversized-block] green (RT-02/03 wet parity)
affects:
  - 21-02 true-bypass mix order
  - 21-03 authentic snapshot edge
  - Phase 22 MIDI span distance / purity

tech-stack:
  added: []
  patterns:
    - "ADR-V1-05 span loop: min(remaining, preparedMaxBlock_, kControlQuantum)"
    - "Prepare-sized scratch only; never AudioBuffer::setSize on audio thread"

key-files:
  created:
    - tests/V1ContractRealtimeSpanTest.cpp
  modified:
    - source/PluginProcessor.h
    - source/PluginProcessor.cpp
    - tests/IntegrationAllocScanTest.cpp

key-decisions:
  - "Scratch strategy: prepare-sized member scratch/dryBuffer; oversized hosts iterate spans ≤ preparedMaxBlock_ (Q1)"
  - "MIDI event distance excluded from span min until Phase 22 (Q2)"
  - "Authentic smoother-edge left in processSpan for Plan 03 ADR-V1-07 cleanup"
  - "Bypass Output/mix order unchanged — CORE-14 stays red for Plan 02"

patterns-established:
  - "PluginProcessor::kControlQuantum = 128 public constant for contracts"
  - "processSpan(buffer, offset, span, snap) owns dry copy + control latch + chain + mix-back"

requirements-completed: [RT-01, RT-02, RT-03, RT-05, RT-15]

coverage:
  - id: D1
    description: "processBlock has no dryBuffer.setSize / heap grow; alloc scan bans .setSize"
    requirement: RT-01
    verification:
      - kind: unit
        ref: "Builds/Tests \"[realtime][static][integration]\""
        status: pass
    human_judgment: false
  - id: D2
    description: "Oversized 2048@prepare512 keeps wet and matches 4×512 within 2e-5"
    requirement: RT-02
    verification:
      - kind: unit
        ref: "Builds/Tests \"[oversized-block]\" (V1ContractOversizedBlockTest)"
        status: pass
    human_judgment: false
  - id: D3
    description: "2048 vs chunked absolute max diff < 2e-5 (same contract as RT-02)"
    requirement: RT-03
    verification:
      - kind: unit
        ref: "Builds/Tests \"[oversized-block]\""
        status: pass
    human_judgment: false
  - id: D4
    description: "Control-rate latch quantum ≤128 with wet continuity across late span boundary"
    requirement: RT-05
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][realtime][RT-05]\""
        status: pass
    human_judgment: false
  - id: D5
    description: "Unprepared / releaseResources then processBlock is safe and finite"
    requirement: RT-15
    verification:
      - kind: unit
        ref: "Builds/Tests \"[v1][contract][realtime][RT-15]\""
        status: pass
    human_judgment: false

duration: 7min
completed: 2026-07-12
status: complete
---

# Phase 21 Plan 01: Span Engine + Oversized Wet Parity Summary

**No-alloc `kControlQuantum=128` span/`processSpan` path removed dry oversized fallback and flipped `[oversized-block]` green (wetEnergy≈0.011, maxAbsDiff=0).**

## Performance

- **Duration:** ~7 min
- **Started:** 2026-07-12T16:08:33Z
- **Completed:** 2026-07-12T16:16:00Z
- **Tasks:** 2
- **Files modified:** 4

## Accomplishments

- Deleted `dryBuffer.setSize` and dry-only `numSamples > preparedMaxBlock_` fallback from `processBlock`
- Added prepare-safe early return when `preparedMaxBlock_ <= 0` (RT-15)
- Implemented continuous offset span loop calling `processSpan` with `span = min(remaining, preparedMaxBlock_, 128)`
- Extended `IntegrationAllocScanTest` to ban `.setSize` (matches JUCE spacing `.setSize (`)
- Flipped `[oversized-block]` green; pressure-release and DryPath remain green
- Note (Q7): SEND-14 Phase 20 oversized dry caveat is resolved as a side effect for wet oversized path

## Task Commits

1. **Task 1 RED: Alloc-scan setSize ban + RT-15 scaffold** - `4677ae1` (test)
2. **Task 1 GREEN: Unprepared guard + drop process setSize** - `c996357` (feat)
3. **Task 2: Span loop + processSpan + RT-05** - `7cd0b8e` (feat)

## Files Created/Modified

- `source/PluginProcessor.h` — `kControlQuantum`, `processSpan` declaration
- `source/PluginProcessor.cpp` — span engine; no process-time setSize; RT-15 early return
- `tests/IntegrationAllocScanTest.cpp` — ban `.setSize` in scanned process bodies
- `tests/V1ContractRealtimeSpanTest.cpp` — RT-15 unprepared + RT-05 quantum/wet continuity

## Decisions Made

- Scratch members sized only in `prepareToPlay`; oversized hosts never grow buffers (Q1)
- Span `min` excludes MIDI distance (Phase 22 / RT-04)
- Authentic smoother 0.5-edge retained inside `processSpan` until Plan 03
- True-bypass Output/mix order intentionally untouched (Plan 02)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Alloc-scan token missed spaced `.setSize (`**
- **Found during:** Task 1 RED
- **Issue:** Production uses `dryBuffer.setSize (` (space before `(`); ban on `.setSize(` alone stayed green incorrectly
- **Fix:** Ban substring `.setSize` so both call spellings fail
- **Files modified:** `tests/IntegrationAllocScanTest.cpp`
- **Verification:** Alloc scan failed on PluginProcessor.cpp until setSize removed
- **Committed in:** `4677ae1` / follow-up in same test file

**2. [Rule 1 - Bug] RT-05 early-span wet energy is tank-silent**
- **Found during:** Task 2
- **Issue:** First ~12×128 samples have wetApprox≈0 due to tank/predelay build-up; asserting the first quantum boundary falsely failed
- **Fix:** Assert total wet energy plus continuity across the final quantum boundary (samples 1792|1920)
- **Files modified:** `tests/V1ContractRealtimeSpanTest.cpp`
- **Verification:** `[v1][contract][realtime][RT-05]` green
- **Committed in:** `7cd0b8e`

## Proof: `[oversized-block]` green

```
Filters: [oversized-block]
REQUIRE( wetEnergy > 1.0e-4f )  PASSED  with 0.011429918f > 0.0001f
REQUIRE( diff < 2.0e-5f )       PASSED  with 0.0f < 0.00002f
All tests passed (2 assertions in 1 test case)
```

## Known Stubs

None — span path is production wet processing (bypass order still pre-ADR-V1-10 for Plan 02).

## Threat Flags

None beyond plan register (T-21-01/T-21-02 mitigated by no-alloc span + unprepared early return).

## Self-Check: PASSED

- FOUND: `source/PluginProcessor.cpp` / `.h` with `kControlQuantum`, `processSpan`, `preparedMaxBlock_ <= 0`
- FOUND: `tests/V1ContractRealtimeSpanTest.cpp`, `tests/IntegrationAllocScanTest.cpp`
- FOUND commits: `4677ae1`, `c996357`, `7cd0b8e`
- FOUND: only `setSize` remaining is in `prepareToPlay` (line 125)
- VERIFIED: `Builds/Tests "[oversized-block]"` wetEnergy=0.011429918 maxAbsDiff=0
