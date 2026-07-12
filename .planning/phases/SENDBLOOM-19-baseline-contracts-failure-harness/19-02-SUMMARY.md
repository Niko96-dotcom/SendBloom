---
phase: 19-baseline-contracts-failure-harness
plan: 02
subsystem: testing
tags: [catch2, contract, v1, baseline, failure-harness, BASE-04]

requires:
  - phase: 19-01
    provides: Restored Catch2/ctest Builds harness + frozen baseline + traceability
provides:
  - Seven dedicated [v1][contract] Catch2 suites that fail for intended defect reasons
  - BASE-04 green proof ([release] + [DryPath] + ENAB + ProperSRC/HF)
affects: [19-03, verify-v1, phases-20-25-fixes]

tech-stack:
  added: []
  patterns:
    - "Dedicated V1Contract*.cpp files assert v1-CORRECT behavior; legacy greens document current buggy truth"
    - "Catch2 tags [v1][contract][<defect-id>] enable filtered red/green proof"
    - "Energy-tracking IReverbEngine mock via GatedBloomChain::setReverbEngineForTests"

key-files:
  created:
    - tests/V1ContractPressureReleaseTest.cpp
    - tests/V1ContractOversizedBlockTest.cpp
    - tests/V1ContractTrueBypassTest.cpp
    - tests/V1ContractPostHardRampTest.cpp
    - tests/V1ContractInputAnchorsTest.cpp
    - tests/V1ContractMidiApvtsPurityTest.cpp
    - tests/V1ContractShippingPolicyTest.cpp
    - .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/deferred-items.md
  modified:
    - .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md

key-decisions:
  - "MIDI purity + processBlock sendParam->store source scan only in Phase 19; DSP-effect half deferred to Phase 22 (A1)"
  - "Primary BASE-04 proof is [release]+[DryPath]+ENAB; full ~[v1] documents pre-existing Xml/Zip JUCE noise separately"
  - "No edits to MidiSendAmountTest, ParameterCurvesTest, ReleaseTruthTest MIDI case, PostGateTimingTest, or source/"

patterns-established:
  - "Contract tests REQUIRE correct v1 behavior so they fail red on current production"
  - "Shipping-policy covers REVERB X / reverbx gaps missed by check-legal-metadata.sh"

requirements-completed: [BASE-04]

coverage:
  - id: D1
    description: Pressure release contract fails on connected-at-rest / amount / wet-feed
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractPressureReleaseTest.cpp#[v1][contract][pressure-release]"
        status: pass
    human_judgment: false
  - id: D2
    description: Oversized-block wet continuity contract fails on dry fallback
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractOversizedBlockTest.cpp#[v1][contract][oversized-block]"
        status: pass
    human_judgment: false
  - id: D3
    description: True bypass per-channel unity contract fails on mono-sum/output gain
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractTrueBypassTest.cpp#[v1][contract][true-bypass]"
        status: pass
    human_judgment: false
  - id: D4
    description: PostHard ramp contract fails on one-sample snap
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractPostHardRampTest.cpp#[v1][contract][posthard]"
        status: pass
    human_judgment: false
  - id: D5
    description: Input −9/0/+9 anchors contract fails on +9…−3 mapping
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractInputAnchorsTest.cpp#[v1][contract][input-anchors]"
        status: pass
    human_judgment: false
  - id: D6
    description: MIDI APVTS purity contract fails on CC1 mutation + sendParam->store scan
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractMidiApvtsPurityTest.cpp#[v1][contract][midi-apvts]"
        status: pass
    human_judgment: false
  - id: D7
    description: Shipping-policy contract fails on REVERB X / reverbx / dryBuffer.setSize
    requirement: BASE-04
    verification:
      - kind: unit
        ref: "tests/V1ContractShippingPolicyTest.cpp#[v1][contract][shipping-policy]"
        status: pass
    human_judgment: false
  - id: D8
    description: Preexisting green suites remain green excluding [v1] contracts
    requirement: BASE-04
    verification:
      - kind: other
        ref: "Builds/Tests \"[release]\" && Builds/Tests \"[DryPath]\" && BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh"
        status: pass
    human_judgment: false

duration: 6min
completed: 2026-07-12
status: complete
---

# Phase 19 Plan 02: Failing Defect Contracts Summary

**Seven dedicated `[v1][contract]` Catch2 suites assert v1-correct behavior and fail for the intended defect reasons, while `[release]`, `[DryPath]`, ProperSRC/HF, and ENAB stay green.**

## Performance

- **Duration:** 6 min
- **Started:** 2026-07-12T15:18:53Z
- **Completed:** 2026-07-12T15:24:35Z
- **Tasks:** 3/3
- **Files modified:** 9 (7 contract TUs + baseline note + deferred-items)

## Accomplishments

- Added seven `V1Contract*.cpp` files tagged `[v1][contract][<defect-id>]` (GLOB-picked by `cmake/Tests.cmake`)
- Each contract filter exits non-zero for the intended assertion (connection flip, wet-zero fallback, channel collapse, PostHard snap, Input anchors, MIDI APVTS store, shipping strings)
- BASE-04 primary green proof passed: `[release]`, `[DryPath]`, ENAB (`BUILD_DIR=Builds`)
- Confirmed `[ProperSRC]` and `[HF]` still pass; no production `source/` edits; legacy green tests untouched

## Task Commits

| Task | Commit | Description |
|------|--------|-------------|
| 1 | `c0571c2` | Pressure / oversized / true-bypass contracts |
| 2 | `7571c49` | PostHard / Input / MIDI purity contracts |
| 3 | `228eee7` | Shipping-policy contract + baseline expected-reds note |

## Contract Failure Map

| Filter | Failure observed |
|--------|------------------|
| `[pressure-release]` | `connectedAfter > 0.5f` → `0.0f` (`mouseUp` disconnects) |
| `[oversized-block]` | `wetEnergy > 1e-4` → `0.0` (dry fallback) |
| `[true-bypass]` | `maxErrL < 1e-6` → `~0.21` (mono-sum/output) |
| `[posthard]` | `g1 > 0` → `0.0` (one-sample snap) |
| `[input-anchors]` | `inputGainDb(0) ≈ -9` → `9.0` |
| `[midi-apvts]` | APVTS unchanged → mutated to ~0.787; `sendParam->store` present |
| `[shipping-policy]` | `"REVERB X"` / `reverbx` / `dryBuffer.setSize` still present |

## Green Proof

| Gate | Result |
|------|--------|
| `Builds/Tests "[release]"` | PASS (12 cases) |
| `Builds/Tests "[DryPath]"` | PASS (4 cases) |
| `BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` | PASS (10/10) |
| `Builds/Tests "[ProperSRC]"` | PASS |
| `Builds/Tests "[HF]"` | PASS |
| `Builds/Tests "~[v1]"` | 201 passed / 4 failed / 1 skipped — failures are pre-existing XmlDocument + GZIP Zip bounds on JUCE 8.0.12 (see deferred-items.md), not V1 contracts |

## Deviations from Plan

### Auto-fixed Issues

None - plan executed as written for contract delivery.

### Out-of-scope discoveries (deferred)

**1. [Deferred] XmlDocument entity-expansion / Zip GZIP bounds fail under `~[v1]`**
- **Found during:** Task 3 wave/plan-end green proof
- **Issue:** Pre-existing vs reachable JUCE 8.0.12 pin (19-01); not caused by contracts
- **Action:** Logged in `deferred-items.md`; primary BASE-04 gates remain the task verify
- **Files modified:** deferred-items.md only

## Auth Gates

None.

## Known Stubs

None — contracts are intentional reds asserting real predicates (not placeholder/TODO stubs).

## Threat Flags

None beyond plan register (T-19-04..06). No production network/auth/schema surface added.

## Decisions Made

- MIDI-03 purity + source scan only; Phase 22 owns DSP-effect companion (A1)
- Document expected contract reds in `19-BASELINE.md`
- Treat `~[v1]` Xml/Zip failures as deferred JUCE security noise, not a BASE-04 regression from this plan

## Self-Check: PASSED

- All seven `tests/V1Contract*.cpp` files present
- Commits `c0571c2`, `7571c49`, `228eee7` present on `main`
- `git diff source/` empty of intentional behavior fixes
