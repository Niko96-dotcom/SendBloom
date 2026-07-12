---
phase: 19-baseline-contracts-failure-harness
reviewed: 2026-07-12T15:35:00Z
depth: deep
files_reviewed: 14
files_reviewed_list:
  - scripts/verify-v1.sh
  - docs/RELEASE_CHECKLIST.md
  - tests/BaselinePresetMetricsTest.cpp
  - tests/RequirementsTraceabilityTest.cpp
  - tests/ZipDecompressionBoundsTest.cpp
  - tests/V1ContractPressureReleaseTest.cpp
  - tests/V1ContractOversizedBlockTest.cpp
  - tests/V1ContractTrueBypassTest.cpp
  - tests/V1ContractPostHardRampTest.cpp
  - tests/V1ContractInputAnchorsTest.cpp
  - tests/V1ContractMidiApvtsPurityTest.cpp
  - tests/V1ContractShippingPolicyTest.cpp
  - cmake/Tests.cmake
  - CMakeLists.txt
findings:
  critical: 0
  warning: 0
  info: 2
  total: 2
status: clean
---

# Phase 19: Code Review Report

**Reviewed:** 2026-07-12T15:35:00Z
**Depth:** deep
**Files Reviewed:** 14
**Status:** clean

## Summary

Phase 19 (plans 19-01..03) is a harness-only change set: Catch2/ctest restore, baseline freeze, failing `[v1][contract]` suites asserting v1-correct behavior, and an honest `verify-v1.sh` / checklist. `git diff` over the plan commit range shows **zero** `source/` production DSP/UI edits. Contract filters exit 42 for the intended predicates; `[release]` / `[DryPath]` remain green. No blockers or warnings — advisory notes only.

## Narrative Findings (AI reviewer)

### Focus checks

| Focus | Result |
| --- | --- |
| No production DSP/UI fixes leaked | **Pass** — phase range touches tests, scripts, docs, cmake/JUCE gitlinks only |
| Contract tests assert v1-correct behavior | **Pass** — REQUIREs encode connected-at-rest, wet continuity, per-channel bypass unity, PostHard ramp, −9/0/+9 Input, APVTS MIDI purity, shipping bans; legacy greens left documenting buggy truth |
| `verify-v1.sh` honesty | **Pass** — runtime `ctest -N` discovery; no hard-coded suite totals; pluginval SKIPPED≠PASS; human gates labeled `human_needed`; automated FAIL → exit 1 |
| cmake/test registration | **Pass** — `include(Tests)` → `cmake/Tests.cmake` GLOB + `catch_discover_tests`; 10 `[v1][contract]` cases discovered in 216-test suite |
| BASE-04 greens not weakened | **Pass** — no edits to MidiSendAmount / ParameterCurves / ReleaseTruth MIDI / PostGateTiming suites; Zip max-uncompressed uses Catch2 `SKIP` (not silent pass); `[release]` 12/12 and `[DryPath]` 4/4 exit 0 |

### Spot-checked contract failure loci (runtime)

| Filter | Observed failure (correct v1 assert) |
| --- | --- |
| `[pressure-release]` | `connectedAfter > 0.5f` with actual `0.0f` (exit 42) |
| `[input-anchors]` | `inputGainDb(0) ≈ -9` vs production `+9…−3` curve (exit 42) |
| `[release]` / `[DryPath]` | All passed (exit 0) |

## Info

### IN-01: BASE-08 anti-pattern grep is fragile against docs wording

**File:** `docs/RELEASE_CHECKLIST.md:23`, `scripts/verify-v1.sh:184-198`
**Issue:** Naive `human_needed.*(PASS|passed)` also matches explanatory phrases (`not auto-PASS`) and comments that *forbid* PASS on human lines. Intent is correct; the grep is a poor honesty oracle.
**Fix (advisory):** Prefer wording like `never silent green` (already used in the HUMAN_NEEDED header) and scope the check to status-table / checklist checkbox lines only.

### IN-02: Baseline metrics Catch2 does not pin numeric peak/RMS

**File:** `tests/BaselinePresetMetricsTest.cpp:124-164`
**Issue:** Suite asserts finiteness + preset names present in `19-BASELINE-METRICS.md`, not equality to committed floats. Acceptable for a capture harness; drift of documented numbers would not fail CI.
**Fix (advisory):** Optional later `[baseline][metrics]` assertion against table values if Phase 27 wants regression locking.

---

_Reviewed: 2026-07-12T15:35:00Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
