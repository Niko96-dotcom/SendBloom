---
phase: 19-baseline-contracts-failure-harness
plan: 03
subsystem: testing
tags: [verify-v1, ctest, human_needed, BASE-05, BASE-06, BASE-08, release-checklist]

requires:
  - phase: 19-01
    provides: Restored Catch2/ctest Builds harness + frozen baseline
  - phase: 19-02
    provides: Failing [v1][contract] suites that keep full ctest red
provides:
  - Durable scripts/verify-v1.sh automated gate entry with truthful RED/GREEN table
  - Runtime-discovered ctest totals (no hard-coded suite size)
  - Explicit human_needed section for human-only gates
  - RELEASE_CHECKLIST.md aligned to BASE-06/08 honesty model
affects: [phase-27, release-gates, CI-operators]

tech-stack:
  added: []
  patterns:
    - "Per-gate status collection without blanket set -e; final table + nonzero exit on automated FAIL"
    - "ctest -N runtime discovery for suite size; never encode expected N/N"
    - "human_needed lines never paired with PASS/passed tokens"

key-files:
  created:
    - scripts/verify-v1.sh
  modified:
    - docs/RELEASE_CHECKLIST.md

key-decisions:
  - "Default BUILD_DIR to $ROOT/Builds (CI-aligned); override ENAB's build default via BUILD_DIR env"
  - "Optional VST3 pluginval only when RUN_PLUGINVAL=1 and PLUGINVAL_BIN set — otherwise SKIPPED, never PASS"
  - "Unchecked Catch2 full-suite checklist box while contract reds remain; discovery language replaces 113/113"

patterns-established:
  - "verify-v1 is the Phase 19/27 durable automated entry composing legal + cmake + ctest + ENAB"
  - "Human gates listed with literal human_needed token; verification greps reject human_needed+PASS pairing"

requirements-completed: [BASE-05, BASE-06, BASE-08]

coverage:
  - id: D1
    description: Durable verify-v1.sh runs legal, configure/build, ctest, ENAB, optional pluginval and prints truthful status
    requirement: BASE-05
    verification:
      - kind: other
        ref: "bash scripts/verify-v1.sh (exit≠0 while contracts red; STATUS TABLE present)"
        status: pass
    human_judgment: false
  - id: D2
    description: Suite counts discovered at runtime; no hard-coded expected totals in verifier or checklist
    requirement: BASE-06
    verification:
      - kind: other
        ref: "ctest -N discovery in verify-v1; no 113/113 in RELEASE_CHECKLIST; no expected_total/TOTAL_TESTS=N"
        status: pass
    human_judgment: false
  - id: D3
    description: Human-only gates labeled human_needed and never silent PASS
    requirement: BASE-08
    verification:
      - kind: other
        ref: "HUMAN_NEEDED section in verify-v1 output; checklist human_needed labels; grep rejects human_needed.*(PASS|passed)"
        status: pass
    human_judgment: false

duration: 2min
completed: 2026-07-12
status: complete
---

# Phase 19 Plan 03: Verifier & Human Gates Summary

**Durable `scripts/verify-v1.sh` reports automated RED/GREEN truthfully (contracts keep ctest red), discovers suite size at runtime, and labels human-only gates `human_needed` — RELEASE_CHECKLIST no longer claims 113/113.**

## Performance

- **Duration:** 2 min
- **Started:** 2026-07-12T15:25:37Z
- **Completed:** 2026-07-12T15:27:58Z
- **Tasks:** 2
- **Files modified:** 2

## Accomplishments

- Added executable `scripts/verify-v1.sh` composing legal metadata, cmake configure/build (Tests), full ctest, ENAB gates, and optional pluginval
- Runtime discovery via `ctest -N` (sample run: discovered_total=216; passed=202 failed=14) with overall RED / exit 1 while contracts remain
- Rewrote Catch2 checklist bullet to discovery language; marked DAW/signing/AU/matrix gaps `human_needed`

## Task Commits

Each task was committed atomically:

1. **Task 1: Implement scripts/verify-v1.sh durable gate runner** - `d0c13ef` (feat)
2. **Task 2: Remove hard-coded suite totals and align checklist honesty** - `4af6b94` (docs)

**Plan metadata:** (pending docs commit)

## Files Created/Modified

- `scripts/verify-v1.sh` — durable automated gate runner (BASE-05/06/08)
- `docs/RELEASE_CHECKLIST.md` — no hard-coded suite totals; verify-v1 referenced; human_needed labels

## Decisions Made

- Default `BUILD_DIR` to `$ROOT/Builds` to match CI; pass `BUILD_DIR` into ENAB subprocess (ENAB alone defaults to `build`)
- pluginval recorded as SKIPPED unless `RUN_PLUGINVAL=1` + `PLUGINVAL_BIN` — never mapped to PASS when not run
- Left Catch2 full-suite checklist unchecked while `[v1][contract]` reds remain (honesty over stale green claim)

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] HUMAN_NEEDED header contained auto-PASS token**
- **Found during:** Task 1 verification (`grep -Eiq 'human_needed.*(PASS|passed)'`)
- **Issue:** Header `HUMAN_NEEDED (never auto-PASS)` matched the BASE-08 anti-pattern grep
- **Fix:** Renamed header to `HUMAN_NEEDED (explicit; never silent green)` and kept PASS off all human_needed lines
- **Files modified:** `scripts/verify-v1.sh`
- **Committed in:** `d0c13ef` (Task 1)

## Sample verify-v1.sh output (local run)

```
GATE                   STATUS     DETAIL
legal-metadata         PASS
cmake-configure        PASS       already configured (.../Builds)
build-Tests            PASS
ctest-full-suite       FAIL       exit=8; discovered_total=216; passed=202 failed=14 (runtime)
enab-acceptance        PASS
pluginval-vst3         SKIPPED    not-run (...); never treated as PASS
Overall automated status: RED / FAIL
HUMAN_NEEDED: AU pluginval/auval, Win/Linux matrix, DAW smoke, signing, notarization, JUCE license
Exit: 1
```

## Threat Flags

None — no new network endpoints, auth paths, or schema changes beyond the planned local gate runner.

## Known Stubs

None — verifier invokes real subprocesses; SKIPPED pluginval is intentional env-gated behavior, not a stub.

## Self-Check: PASSED

- FOUND: `scripts/verify-v1.sh`
- FOUND: `docs/RELEASE_CHECKLIST.md`
- FOUND: commit `d0c13ef`
- FOUND: commit `4af6b94`
