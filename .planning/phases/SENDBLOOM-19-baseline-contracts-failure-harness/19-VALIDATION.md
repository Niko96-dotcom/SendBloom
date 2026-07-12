---
phase: 19
slug: baseline-contracts-failure-harness
status: complete
nyquist_compliant: true
wave_0_complete: true
created: 2026-07-12
updated: 2026-07-12
verification: passed
---

# Phase 19 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
>
> **Phase 19 success model (not all-green ctest):**
> - Contracts tagged `[v1][contract]` must **FAIL** for intended reasons
> - Green proof: `Builds/Tests "~[v1]"` (or targeted `[release]` + `[DryPath]` + ENAB with `BUILD_DIR=Builds`) still **PASS**
> - `scripts/verify-v1.sh` reports red/green truthfully and prints `human_needed`
> - Baseline file exists with required fields (commit, branch, VERSION, build config, discovered count, CI workflows, manual gaps)
> - No hard-coded test totals in scripts/checklist
>
> **Post-verify:** Aligned to `19-01/02/03-SUMMARY.md` + `19-VERIFICATION.md` (status: passed, 6/6 must-haves). Intentional `[v1][contract]` reds are **expected** — map Status ✅ means the automated expect-fail / green-proof command succeeded, not that contracts themselves passed.

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.8.1 + ctest (`Tests` executable) |
| **Config file** | `cmake/Tests.cmake` (via `include(Tests)` in root `CMakeLists.txt`) |
| **Quick run command** | `Builds/Tests "[release]"` && `Builds/Tests "[DryPath]"` && `BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` |
| **Green-without-contracts** | `Builds/Tests "~[v1]"` |
| **Contract-only (expect fail)** | `Builds/Tests "[v1][contract]"` (exit non-zero required) |
| **Full suite command** | `ctest --test-dir Builds -C Release --output-on-failure` (do **not** treat all-green as Phase 19 success while contracts exist) |
| **Phase gate** | `bash scripts/verify-v1.sh` (truthful status + `human_needed`; exit non-zero while contract reds remain) |
| **Estimated runtime** | Quick ~30–90s; `~[v1]` ~2–5 min; verify-v1 depends on configure/build |

---

## Sampling Rate

- **After every task commit:** Run the task's `<automated>` command (see map below)
- **After Wave 1 (19-01):** `ctest --test-dir Builds -N` discovers >0 tests; baseline artifacts present; `[traceability]` green
- **After Wave 2 (19-02):** each `[v1][contract]` filter fails; quick green (`[release]` + `[DryPath]` + ENAB) passes; wave-end proof `Builds/Tests "~[v1]"` passes
- **After Wave 3 (19-03) / before `/gsd-verify-work`:** `bash scripts/verify-v1.sh` truthful red/green + `human_needed`; no hard-coded totals
- **Max feedback latency (task verify):** prefer < 90s via targeted filters; full `~[v1]` reserved for wave/plan-end proof

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| BASE-01 | Baseline fields captured | `test -f .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md && grep -Eiq 'commit|branch|VERSION|manual|ctest|workflow' .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md` | pass | `19-BASELINE.md` |
| BASE-02 | REQUIREMENTS completeness | `grep -c 'BASE-0[1-8]' .planning/REQUIREMENTS.md \| awk '{exit !($1>=8)}'` (+ full ID audit in Task 19-01-03) | pass | `.planning/REQUIREMENTS.md` |
| BASE-03 | phase + ≥1 artifact map | `cmake --build Builds --config Release --target Tests && Builds/Tests "[traceability]"` | pass | `RequirementsTraceabilityTest.cpp` |
| BASE-04 | ProperSRC/HF/DryPath/release greens preserved | Task: `Builds/Tests "[release]" && Builds/Tests "[DryPath]" && BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh`; Wave/plan-end: `Builds/Tests "~[v1]"` | pass (contracts excluded) | existing green tests |
| BASE-05 | Durable verify-v1 gate runner | `test -x scripts/verify-v1.sh && bash scripts/verify-v1.sh >/tmp/verify-v1.out 2>&1; EC=$?; grep -q 'human_needed' /tmp/verify-v1.out && grep -Eiq 'FAIL\|PASS\|RED\|GREEN\|status' /tmp/verify-v1.out && ! grep -Eiq 'human_needed.*(PASS\|passed)' /tmp/verify-v1.out && test "$EC" -ne 0` | exit ≠0 while contracts red; status table present | `scripts/verify-v1.sh` |
| BASE-06 | No hard-coded test totals | `! grep -E '[0-9]+/[0-9]+' docs/RELEASE_CHECKLIST.md \| grep -iE 'ctest\|Catch2\|suite\|tests pass' && ! grep -E 'expected_total\|TOTAL_TESTS=[0-9]+' scripts/verify-v1.sh` | pass | checklist + verifier |
| BASE-07 | Factory-preset metrics recorded | `test -f .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE-METRICS.md && Builds/Tests "[baseline][metrics]"` | pass | `19-BASELINE-METRICS.md` + `BaselinePresetMetricsTest.cpp` |
| BASE-08 | Human gates `human_needed` | same verify-v1 capture as BASE-05; also `grep -q 'human_needed' scripts/verify-v1.sh` | pass | verifier + checklist |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|------------|-----------------|-----------|-------------------|-------------|--------|
| 19-01-01 | 01 | 1 | BASE-01 | T-19-01 | Restore only known cmake SHA with Tests.cmake; no rogue Catch2 install | infra / discovery | `test -f cmake/Tests.cmake && ctest --test-dir Builds -N 2>/dev/null \| tee /tmp/ctest-n.txt \| grep -E 'Total Tests: [1-9][0-9]*' && test -x Builds/Tests -o -x Builds/Release/Tests -o -f Builds/Tests` | ✅ | ✅ green |
| 19-01-02 | 01 | 1 | BASE-01, BASE-07 | T-19-02 / T-19-03 | Manual gaps listed not as pass; metrics from in-repo FactoryPresets only | doc + Catch2 | `test -f .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md && test -f .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE-METRICS.md && test -f tests/BaselinePresetMetricsTest.cpp && grep -E 'commit\|branch\|VERSION\|manual\|ctest\|workflow' -i .planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md \| head -5 && cmake --build Builds --config Release --target Tests && Builds/Tests "[baseline][metrics]"` | ✅ | ✅ green |
| 19-01-03 | 01 | 1 | BASE-02, BASE-03 | T-19-02 | Traceability map is evidence, not silent pass | Catch2 + doc | `grep -c 'BASE-0[1-8]' .planning/REQUIREMENTS.md \| awk '{exit !($1>=8)}' && cmake --build Builds --config Release --target Tests && Builds/Tests "[traceability]"` | ✅ | ✅ green |
| 19-02-01 | 02 | 2 | BASE-04 | T-19-04 / T-19-05 | New contract files only; expect fail | contract (expect fail) | `cmake --build Builds --config Release --target Tests && (Builds/Tests "[v1][contract][pressure-release]" ; test $? -ne 0) && (Builds/Tests "[v1][contract][oversized-block]" ; test $? -ne 0) && (Builds/Tests "[v1][contract][true-bypass]" ; test $? -ne 0)` | ✅ | ✅ green |
| 19-02-02 | 02 | 2 | BASE-04 | T-19-04 / T-19-05 | New contract files only; expect fail | contract (expect fail) | `cmake --build Builds --config Release --target Tests && (Builds/Tests "[v1][contract][posthard]" ; test $? -ne 0) && (Builds/Tests "[v1][contract][input-anchors]" ; test $? -ne 0) && (Builds/Tests "[v1][contract][midi-apvts]" ; test $? -ne 0)` | ✅ | ✅ green |
| 19-02-03 | 02 | 2 | BASE-04 | T-19-04 / T-19-05 / T-19-06 | Contract fails; green suites stay green | contract + green proof | **Primary (task):** `cmake --build Builds --config Release --target Tests && (Builds/Tests "[v1][contract][shipping-policy]" ; test $? -ne 0) && Builds/Tests "[release]" && Builds/Tests "[DryPath]" && BUILD_DIR=Builds bash scripts/enab-acceptance-gates.sh` — **Wave/plan-end:** `Builds/Tests "~[v1]"` (see Sampling Results) | ✅ | ✅ green |
| 19-03-01 | 03 | 3 | BASE-05, BASE-08 | T-19-07 | Status truthful; human_needed never PASS | smoke script | `test -x scripts/verify-v1.sh && bash scripts/verify-v1.sh >/tmp/verify-v1.out 2>&1; EC=$?; grep -q 'human_needed' /tmp/verify-v1.out && grep -Eiq 'FAIL\|PASS\|RED\|GREEN\|status' /tmp/verify-v1.out && ! grep -Eiq 'human_needed.*(PASS\|passed)' /tmp/verify-v1.out && test "$EC" -ne 0` | ✅ | ✅ green |
| 19-03-02 | 03 | 3 | BASE-06, BASE-08 | T-19-08 | No hard-coded suite totals; human honesty preserved | lint / doc | `! grep -E '[0-9]+/[0-9]+' docs/RELEASE_CHECKLIST.md \| grep -iE 'ctest\|Catch2\|suite\|tests pass' && ! grep -E 'expected_total\|TOTAL_TESTS=[0-9]+' scripts/verify-v1.sh && grep -q 'human_needed' scripts/verify-v1.sh && grep -q 'verify-v1' docs/RELEASE_CHECKLIST.md` | ✅ | ✅ green |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*
*Contract rows: ✅ green = expect-fail command observed non-zero exit for intended REQUIREs (intentional reds, not suite failures).*

---

## Sampling Results (post-verify 2026-07-12)

Evidence from `19-01/02/03-SUMMARY.md` and `19-VERIFICATION.md` (passed, 6/6). Counts are runtime snapshots — never hard-code as expected suite size (BASE-06).

| Wave / Gate | Command | Result | Notes |
|-------------|---------|--------|-------|
| 19-01 discovery | `ctest --test-dir Builds -N` | pass | Total Tests > 0 (206 at 19-01 end; 216 at verify) |
| 19-01 metrics | `Builds/Tests "[baseline][metrics]"` | pass | 78 assertions |
| 19-01 traceability | `Builds/Tests "[traceability]"` | pass | 552 assertions; BASE-01…08 artifacts mapped |
| 19-02 contracts (×7 filters) | `Builds/Tests "[v1][contract][<id>]"` | expect-fail ✓ | All EC=42; intended REQUIRE lines |
| 19-02 combined contracts | `Builds/Tests "[v1][contract]"` | expect-fail ✓ | 10 failed / 0 passed — intentional |
| 19-02 primary green | `[release]` + `[DryPath]` + ENAB | pass | 12/12, 4/4, 10/10; ProperSRC/HF also pass |
| 19-02 wave-end `~[v1]` | `Builds/Tests "~[v1]"` | ⚠️ caveat | 201 passed / 4 failed / 1 skipped — XmlDocument + GZIP Zip bounds on JUCE 8.0.12; deferred (not V1 contracts; not BASE-04 primary) |
| 19-03 verify-v1 | `bash scripts/verify-v1.sh` | pass (truthful RED) | exit 1; discovered_total=216; ctest FAIL 202/14; ENAB PASS; human_needed ×9; no human+PASS |
| BASE-06 lint | checklist + script greps | pass | No hard-coded `N/N` suite claims; no `TOTAL_TESTS=N` |
| Harness scope | `git diff … -- source/` | pass | 0 production DSP/UI edits in phase range |

**Intentional reds (do not treat as validation gaps):** all `[v1][contract]*` cases until Phases 20–25 fix production loci.

**Deferred (non-blocking for Phase 19):** Xml entity + Zip GZIP bounds under `~[v1]` → Phase 27 / security-JUCE; MIDI DSP-effect half → Phase 22.

---

## Wave / Plan-End Green Proof

| When | Command | Expect | Post-verify |
|------|---------|--------|-------------|
| After 19-02 Task 3 (primary) | `[release]` + `[DryPath]` + ENAB | pass | ✅ observed |
| After 19-02 Task 3 (wave/plan-end) | `Builds/Tests "~[v1]"` | pass excluding known deferred JUCE noise | ⚠️ 4 Xml/Zip fails deferred |
| After 19-03 / phase gate | `bash scripts/verify-v1.sh` | exit ≠0 while `[v1][contract]` reds remain; prints status + `human_needed` | ✅ observed |
| Optional full discovery | `ctest --test-dir Builds -N` | Total Tests > 0 (runtime; never hard-code) | ✅ 216 at verify |

---

## Wave 0 Requirements

- [x] Restore `cmake/` submodule to reachable SHA with `Tests.cmake` (e.g. `d5cb9b3`); realign parent gitlink — **19-01 Task 1**
- [x] Reconfigure `Builds` so `ctest -N` discovers tests
- [x] `tests/BaselinePresetMetricsTest.cpp` — stubs/`[baseline][metrics]` for BASE-07 — **19-01 Task 2**
- [x] Extend `tests/RequirementsTraceabilityTest.cpp` for BASE-03 — **19-01 Task 3**
- [x] `tests/V1Contract*.cpp` — dedicated per-defect files — **19-02**
- [x] `scripts/verify-v1.sh` — **19-03 Task 1**
- [x] Reword hard-coded suite totals in `docs/RELEASE_CHECKLIST.md` — **19-03 Task 2**

Existing Catch2 + ENAB + legal scripts cover green-suite preservation once cmake is restored; Wave 0 complete.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| AU pluginval / auval | BASE-08 / REL | Host/tool not always available locally | Record as `human_needed` in verify-v1; do not auto-PASS |
| Windows/Linux matrix | BASE-08 | Not executed in single local macOS run | Label `human_needed` when not run |
| DAW smoke (Logic/Cubase/REAPER) | BASE-08 | Requires human DAW session | Checklist + verify-v1 `human_needed` |
| Developer ID signing / notarization | BASE-08 | Apple account / CI secrets | Never map skip → PASS |
| JUCE license decision | BASE-08 | Legal/business | Explicit `human_needed` |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Task feedback prefers targeted green filters (< ~90s); full `~[v1]` at wave/plan-end
- [x] Phase success ≠ all-green ctest while contracts intentionally red
- [x] `nyquist_compliant: true` set in frontmatter
- [x] Sampling results updated from executed SUMMARYs + VERIFICATION (passed)
- [x] No invented failures for intentional `[v1][contract]` reds

**Approval:** approved 2026-07-12 (post `19-VERIFICATION.md` passed)
