---
phase: 20-pressure-send-state-truth
plan: 03
subsystem: presets
tags: [factory-presets, resting-matrix, SEND-11, SEND-14, UX-01, UX-02, UX-03, UX-04, UX-05, Catch2]

requires:
  - phase: 20-pressure-send-state-truth
    provides: PressureController + pad release-to-zero + pressure-release green
provides:
  - Eight factory presets aligned to §9.7 resting matrix (pressure trio connected+amount0; always-on five disconnected+amount0)
  - PresetTest resting matrix + UX-03 program↔XML send parity
  - SEND-14 in-range block-size coverage with Phase 21 oversized caveat locked
affects:
  - Phase 21 oversized / span engine (caveat remains until then)
  - Phase 25 branding (no Soft Pressure factory preset added)

tech-stack:
  added: []
  patterns:
    - "Factory resting state only via resources/presets XML → BinaryData (no FactoryPresets hardcode)"
    - "SEND-14 settle-by-wall-time when sweeping prepared block sizes"

key-files:
  created: []
  modified:
    - resources/presets/Sparkle_Verb.xml
    - resources/presets/Cut_Sample_Gate.xml
    - resources/presets/Spacerock_Burn.xml
    - resources/presets/Dry_Dub_Sends.xml
    - resources/presets/Dark_Bloom.xml
    - resources/presets/Firm_Pressure.xml
    - resources/presets/Gated_Room.xml
    - resources/presets/Hot_Clip.xml
    - tests/PresetTest.cpp
    - tests/PressureControllerTest.cpp
    - tests/V1ContractPressureReleaseTest.cpp
    - .planning/phases/SENDBLOOM-20-pressure-send-state-truth/20-RESEARCH.md

key-decisions:
  - "Dry Dub Sends is a pressure preset (connected=1 amount=0) per §9.7 / research Q3"
  - "No Soft Pressure factory preset — Soft remains send_feel via Advanced"
  - "SEND-14 in-range greens only; oversized dry fallback documented as Phase 21 caveat"

patterns-established:
  - "Pattern: resting matrix asserted in PresetTest [preset][send] with per-preset XML parity"
  - "Pattern: block-size sweeps settle by ~0.5 s wall time, not fixed block counts"

requirements-completed: [SEND-11, SEND-14, UX-01, UX-02, UX-03, UX-04, UX-05]

coverage:
  - id: D1
    description: Pressure factory presets load connected with send_amount 0 (SEND-11 / UX-04)
    requirement: SEND-11
    verification:
      - kind: unit
        ref: tests/PresetTest.cpp#Factory presets match §9.7 send resting matrix
        status: pass
    human_judgment: false
  - id: D2
    description: Always-on factory presets load disconnected with amount 0 (UX-05)
    requirement: UX-05
    verification:
      - kind: unit
        ref: tests/PresetTest.cpp#Factory presets match §9.7 send resting matrix
        status: pass
    human_judgment: false
  - id: D3
    description: Program load and XML parse recall identical send resting state (UX-03)
    requirement: UX-03
    verification:
      - kind: unit
        ref: tests/PresetTest.cpp#UX-03 program↔XML send parity per preset
        status: pass
    human_judgment: false
  - id: D4
    description: Parameter IDs unchanged; default send_amount 0 (UX-01/02)
    requirement: UX-02
    verification:
      - kind: unit
        ref: Builds/Tests "[parm][layout]"
        status: pass
    human_judgment: false
  - id: D5
    description: SEND-14 in-range pressure semantics across prepared blocks 64/128/256/512
    requirement: SEND-14
    verification:
      - kind: unit
        ref: Builds/Tests "[send][SEND-14]"
        status: pass
    human_judgment: false
  - id: D6
    description: Oversized / true-bypass / midi-apvts contracts remain intentionally red
    verification:
      - kind: unit
        ref: Builds/Tests "[v1][contract][oversized-block]" (expect fail)
        status: pass
      - kind: unit
        ref: Builds/Tests "[v1][contract][true-bypass]" (expect fail)
        status: pass
      - kind: unit
        ref: Builds/Tests "[v1][contract][midi-apvts]" (expect fail)
        status: pass
    human_judgment: false

duration: 5min
completed: 2026-07-12
status: complete
---

# Phase 20 Plan 03: Presets matrix + UX + SEND-14 caveat Summary

**Factory presets now rest on the §9.7 matrix (pressure trio connected+amount 0; always-on five disconnected+amount 0); SEND-14 in-range block sweeps are green with an explicit Phase 21 oversized caveat.**

## Performance

- **Duration:** 5 min
- **Started:** 2026-07-12T15:55:43Z
- **Completed:** 2026-07-12T16:00:21Z
- **Tasks:** 2
- **Files modified:** 12

## Accomplishments

- Aligned all eight `resources/presets/*.xml` send resting values; rebuilt `SendBloomPresets` BinaryData
- Added `[preset][send]` resting-matrix coverage including UX-03 program↔XML parity (no Soft Pressure factory preset)
- Added `[send][SEND-14]` controller + processor block-size coverage; locked oversized caveat in `20-RESEARCH.md` Q1

## Task Commits

1. **Task 1 RED:** `8e363f1` — test(20-03): add failing preset resting matrix (SEND-11)
2. **Task 1 GREEN:** `cc8e531` — feat(20-03): align factory presets to §9.7 resting matrix
3. **Task 2 tests:** `c877738` — test(20-03): add SEND-14 in-range block-size coverage
4. **Task 2 docs:** `9fae206` — docs(20-03): lock SEND-14 oversized Phase 21 caveat

**Plan metadata:** `1e0f2e4` (docs: complete plan)

## Files Created/Modified

- `resources/presets/*.xml` (8) — §9.7 `send_connected` / `send_amount` resting matrix
- `tests/PresetTest.cpp` — resting matrix + UX-03 send parity
- `tests/PressureControllerTest.cpp` — SEND-14 chunk sweep
- `tests/V1ContractPressureReleaseTest.cpp` — SEND-14 processor rest/press/release
- `20-RESEARCH.md` — Q1 execution lock for oversized Phase 21 caveat

## Decisions Made

- Dry Dub Sends is pressure (connected=1, amount=0), not always-on
- Soft Pressure is not a factory preset; Soft stays `send_feel`
- SEND-14 claims in-range prepared blocks only; oversized remains Phase 21

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] SEND-14 processor settle used fixed block counts**
- **Found during:** Task 2
- **Issue:** At prepared block 64, 20 blocks ≈ 26.7 ms — insufficient for 25 ms release + margin; `releaseEnergy` assertion failed
- **Fix:** Settle/probe by wall time (~0.5 s / ~0.05 s) derived from block size
- **Files modified:** `tests/V1ContractPressureReleaseTest.cpp`
- **Commit:** `c877738`

## TDD Gate Compliance

- Task 1: RED (`8e363f1`) then GREEN (`cc8e531`) — compliant
- Task 2: SEND-14 production path already shipped in 20-01; this plan adds coverage + docs (no new DSP). Tests committed first (`c877738`), caveat docs second (`9fae206`). No unexpected RED fail-fast because Task 2 is coverage of existing behavior, not a new feature RED

## Known Stubs

None — presets and tests assert real resting values; no placeholder Soft Pressure preset.

## Threat Flags

None — no new network/auth/file trust boundaries beyond existing embedded preset XML (T-20-07/08 mitigated by matrix asserts + BinaryData rebuild).

## Verification

| Filter | Result |
|--------|--------|
| `[preset]` | PASS |
| `[parm][layout]` | PASS |
| `[send]` / `[send][SEND-14]` | PASS |
| `[pressure-release]` | PASS |
| `[v1][contract][oversized-block]` | FAIL (intentional) |
| `[v1][contract][true-bypass]` | FAIL (intentional) |
| `[v1][contract][midi-apvts]` | FAIL (intentional) |
| BaselinePresetMetrics | PASS (finite/documented; no expectation rewrite needed) |

## Self-Check: PASSED

- All key files found on disk
- Commits `8e363f1`, `cc8e531`, `c877738`, `9fae206` present in git log
