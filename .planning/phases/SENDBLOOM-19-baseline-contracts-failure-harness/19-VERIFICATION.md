---
phase: 19-baseline-contracts-failure-harness
verified: 2026-07-12T15:30:00Z
status: passed
score: 6/6 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification: false
deferred:
  - truth: "Full ~[v1] suite is all-green (Xml entity + GZIP bounds cases)"
    addressed_in: "Phase 27 (RC) / security-JUCE follow-up"
    evidence: "deferred-items.md — XmlDocumentEntityExpansionTest + ZipDecompressionBoundsTest GZIP fail on reachable JUCE 8.0.12; out of Phase 19 harness scope. BASE-04 primary proof is [release]/[DryPath]/ProperSRC/HF/ENAB."
  - truth: "MIDI §8.4 DSP-effect half (pressure changes without APVTS write)"
    addressed_in: "Phase 22"
    evidence: "Phase 22 goal: MIDI CC1 sample-position-aware realtime pressure modulation; 19-02 shipped purity + source scan only"
---

# Phase 19: Baseline, Contracts & Failure Harness Verification Report

**Phase Goal:** Current truth is frozen, every confirmed defect has a failing test for the right reason, and milestone traceability plus a durable verifier exist before production fixes.

**Verified:** 2026-07-12T15:30:00Z  
**Status:** passed  
**Re-verification:** No — initial verification

**Success model (phase-specific):** All-green full `ctest` is **not** required. Intentional `[v1][contract]` reds plus truthful `verify-v1.sh` reporting constitute success.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | ------- | ---------- | -------------- |
| 1 | Baseline report records commit, branch, build/CI state, discovered test count, and known manual gaps before production DSP/UI changes | ✓ VERIFIED | `19-BASELINE.md` has SHA `9024ccf…`, branch `main`, VERSION `0.0.1`, Release/`Builds`, `Total Tests: 204` at capture, CI workflow `SendBloom`, manual gaps table |
| 2 | Every confirmed defect has a contract test that fails for the intended reason (pressure, oversized, bypass, PostHard, Input anchors, MIDI APVTS, shipping policy) | ✓ VERIFIED | All seven filters exit 42 with intended REQUIRE failures (see spot-checks); 10 `[v1][contract]` cases / 0 passed |
| 3 | Preexisting ProperSRC, HF, dry-integrity, and release-truth tests still pass (contracts excluded) | ✓ VERIFIED | `[release]` 12/12 PASS; `[DryPath]` 4/4 PASS; `[ProperSRC]` PASS; `[HF]` PASS; ENAB 10/10 PASS |
| 4 | `scripts/verify-v1.sh` runs the automated gate set and truthfully reports red/green without hard-coded totals | ✓ VERIFIED | Executable; discovers `216` at runtime; STATUS TABLE shows ctest FAIL (202 passed / 14 failed), ENAB PASS, pluginval SKIPPED; exit 1 |
| 5 | Human-only gates are marked `human_needed` and never silently treated as pass | ✓ VERIFIED | HUMAN_NEEDED section prints 9× `human_needed`; `grep human_needed.*(PASS\|passed)` clean; checklist aligned |
| 6 | No production DSP/UI fixes under `source/` were required for this phase to pass | ✓ VERIFIED | `git diff --name-only 8da6ea7^..HEAD -- source/` → 0 files; phase commits are tests/docs/scripts/submodules only |

**Score:** 6/6 truths verified (0 present, behavior-unverified)

### Deferred Items

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | Xml entity expansion + GZIP bounds reds on JUCE 8.0.12 (`~[v1]` 4 failed) | Phase 27 / security follow-up | `deferred-items.md`; not BASE-04 primary proof |
| 2 | MIDI DSP-effect half beyond APVTS purity | Phase 22 | Phase 22 goal + 19-02 A1 deferral |

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | ----------- | ------ | ------- |
| `cmake/Tests.cmake` | Catch2 include restored | ✓ VERIFIED | 64 lines; `include(Tests)` in `CMakeLists.txt:97`; `ctest -N` → 216 |
| `19-BASELINE.md` | Frozen BASE-01 report | ✓ VERIFIED | Identity, build, discovery, CI, manual gaps, expected contract reds |
| `19-BASELINE-METRICS.md` | Factory peak/RMS | ✓ VERIFIED | 8 presets; method committed |
| `tests/BaselinePresetMetricsTest.cpp` | `[baseline][metrics]` | ✓ VERIFIED | PASS (78 assertions); emits BASELINE_METRICS lines |
| `tests/RequirementsTraceabilityTest.cpp` | BASE-03 anchors | ✓ VERIFIED | `[traceability]` PASS (552 assertions) |
| `tests/V1ContractPressureReleaseTest.cpp` | Pressure contract | ✓ VERIFIED | Fails `REQUIRE( connectedAfter > 0.5f )` |
| `tests/V1ContractOversizedBlockTest.cpp` | Oversized wet | ✓ VERIFIED | Fails `REQUIRE( wetEnergy > 1.0e-4f )` |
| `tests/V1ContractTrueBypassTest.cpp` | Bypass unity | ✓ VERIFIED | Fails `REQUIRE( maxErrL < 1.0e-6f )` |
| `tests/V1ContractPostHardRampTest.cpp` | PostHard ramp | ✓ VERIFIED | Fails `REQUIRE( g1 > 0.0f )` (snap) |
| `tests/V1ContractInputAnchorsTest.cpp` | Input −9/0/+9 | ✓ VERIFIED | Fails `inputGainDb(0) == -9` |
| `tests/V1ContractMidiApvtsPurityTest.cpp` | MIDI purity | ✓ VERIFIED | Fails APVTS mutation + source-scan cases |
| `tests/V1ContractShippingPolicyTest.cpp` | Shipping policy | ✓ VERIFIED | Fails REVERB X / reverbx / dryBuffer.setSize |
| `scripts/verify-v1.sh` | Durable gate runner | ✓ VERIFIED | 207 lines; executable; composes legal/cmake/ctest/ENAB |
| `docs/RELEASE_CHECKLIST.md` | No hard-coded totals | ✓ VERIFIED | Discovery language; `human_needed`; references verify-v1 |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | --- | --- | ------ | ------- |
| `CMakeLists.txt` `include(Tests)` | `cmake/Tests.cmake` GLOB | submodule restore | ✓ WIRED | Manual: file present + discovery 216 (gsd-tools path-from heuristic N/A) |
| `19-BASELINE.md` discovered count | `ctest -N` | runtime snapshot | ✓ WIRED | Capture notes runtime-only; BASE-06 rule stated |
| REQUIREMENTS Traceability column | `RequirementsTraceabilityTest` | `[traceability]` | ✓ WIRED | Test PASS; artifact column present for BASE-01…08 |
| `V1Contract*` REQUIREs | broken loci in `source/` | observe-only | ✓ WIRED | Failures match expected loci; **zero** `source/` edits in phase range |
| BASE-04 green proof | `[release]`/`[DryPath]`/ENAB | tag filters | ✓ WIRED | All PASS in this verification run |
| `scripts/verify-v1.sh` | legal + ENAB + ctest | subprocess | ✓ WIRED | Invokes all three; records per-gate status |
| HUMAN_NEEDED section | AU/matrix/DAW/signing/license | explicit labels | ✓ WIRED | 9 lines with literal `human_needed`; never PASS |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| -------- | ------------- | ------ | ------------------ | ------ |
| `BaselinePresetMetricsTest` | peak/RMS per preset | Offline `PluginProcessor` render of factory XML | Yes (logged + finite asserts) | ✓ FLOWING |
| `V1Contract*` | process/APVTS/file scans | Live plugin / source text | Yes (asserts against real broken behavior) | ✓ FLOWING |
| `verify-v1.sh` | `DISCOVERED_TOTAL`, gate statuses | `ctest -N` + gate exit codes | Yes (runtime parse) | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Contract filter fails (×7) | `Builds/Tests "[v1][contract][<id>]"` | All EC=42; intended REQUIRE lines | ✓ PASS |
| Combined contracts red | `Builds/Tests "[v1][contract]"` | 10 failed / 0 passed | ✓ PASS |
| Release truth green | `Builds/Tests "[release]"` | 12 cases PASS | ✓ PASS |
| Dry integrity green | `Builds/Tests "[DryPath]"` | 4 cases PASS | ✓ PASS |
| ProperSRC / HF green | `Builds/Tests "[ProperSRC]"`, `"[HF]"` | PASS | ✓ PASS |
| ENAB green | `BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` | 10/10 PASS | ✓ PASS |
| Metrics + traceability | `"[baseline][metrics]"`, `"[traceability]"` | PASS | ✓ PASS |
| verify-v1 truthful red | `bash scripts/verify-v1.sh` | exit 1; STATUS RED; human_needed; no human+PASS | ✓ PASS |
| No hard-coded totals | grep checklist/script | No `N/N` suite claims; no `TOTAL_TESTS=N` | ✓ PASS |
| No `source/` fixes | `git diff --name-only … -- source/` | 0 files | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| ----- | ------- | ------ | ------ |
| — | — | No phase-declared `scripts/*/tests/probe-*.sh` | SKIPPED |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ---------- | ----------- | ------ | -------- |
| BASE-01 | 19-01 | Baseline commit/branch/build/count/CI/gaps | ✓ SATISFIED | `19-BASELINE.md` |
| BASE-02 | 19-01 | REQUIREMENTS catalog complete | ✓ SATISFIED | `.planning/REQUIREMENTS.md` + Traceability |
| BASE-03 | 19-01 | Req → phase + ≥1 artifact | ✓ SATISFIED | Artifact column + `[traceability]` PASS |
| BASE-04 | 19-02 | Preserve ProperSRC/HF/DryPath/release greens | ✓ SATISFIED | Targeted greens + ENAB PASS; contracts in dedicated files only |
| BASE-05 | 19-03 | Durable verify-v1 runner | ✓ SATISFIED | `scripts/verify-v1.sh` |
| BASE-06 | 19-03 | No hard-coded test totals | ✓ SATISFIED | Runtime discovery; checklist rewritten |
| BASE-07 | 19-01 | Factory preset metrics before DSP change | ✓ SATISFIED | `19-BASELINE-METRICS.md` + metrics test PASS |
| BASE-08 | 19-03 | Human gates `human_needed` | ✓ SATISFIED | Verifier + checklist |

**Orphaned requirements:** none for Phase 19 (BASE-01…08 all claimed).

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| `scripts/verify-v1.sh` | 95 | `XXXXXX` in `mktemp` template | ℹ️ Info | False-positive on TBD/FIXME/XXX debt scan — not a debt marker |
| Preexisting greens | — | Untouched | ✓ | MidiSendAmount / ParameterCurves / ReleaseTruth / PostGateTiming / DryPathIntegrity unmodified |
| `source/` | — | No phase edits | ✓ | Harness-only phase honored |

### Human Verification Required

None for Phase 19 goal closure. Human-only release gates (AU pluginval, OS matrix, DAW smoke, signing, notarization, license) are **correctly labeled** `human_needed` in the verifier/checklist; executing them is Phase 27 / release work, not a Phase 19 blocker.

### Gaps Summary

No blocking gaps. Full-suite ctest remains red by design (`[v1][contract]` plus documented JUCE 8.0.12 Xml/Zip noise). Phase 19 harness goals are achieved; production fixes belong to Phases 20–25.

---

_Verified: 2026-07-12T15:30:00Z_  
_Verifier: Claude (gsd-verifier)_
