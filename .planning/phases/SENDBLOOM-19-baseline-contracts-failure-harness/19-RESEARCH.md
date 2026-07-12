# Phase 19: Baseline, Contracts & Failure Harness - Research

**Researched:** 2026-07-12
**Domain:** Catch2 contract harness / CMake+ctest verifier / audio-plugin defect baselines (C++20 / JUCE 8)
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Baseline report lives at `.planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md` (phase-scoped, committed with the harness)
- BASE-01 captures commit SHA, branch, VERSION, build config, discovered Catch2/ctest count, CI workflow names, and known manual gaps
- Requirement→phase→artifact mapping verified via planning tables plus extendable `RequirementsTraceabilityTest` / test tags so each ID maps to a phase and at least one test tag or doc path
- Baseline is frozen before any production DSP/UI fix lands — Phase 19 commits only harness, docs, and intentionally failing contracts
- New failing contracts live in dedicated Catch2 files under `tests/` (e.g. `V1Contract*.cpp` or defect-named files), tagged `[v1][contract]` plus a stable defect id — do not weaken existing green tests to encode failures
- Defect coverage for Phase 19: pressure release flip, oversized-block dry fallback, true bypass unity, PostHard one-sample snap, Input display/DSP anchors, MIDI APVTS mutation from `processBlock`, banned shipping strings/filenames
- Each contract must fail for the intended reason (assert the broken contract), not flaky timing or unrelated setup
- Preexisting ProperSRC, HF, dry-integrity, and release-truth tests must remain green unless a requirement explicitly updates their contract (BASE-04)
- Add `scripts/verify-v1.sh` as the durable automated gate entry point (alongside existing `enab-acceptance-gates.sh` / `check-legal-metadata.sh`)
- Script discovers and runs the full automated gate set (configure/build/ctest + legal/release gates as applicable) and reports current red/green truthfully
- Do not hard-code expected total test counts in docs or scripts (BASE-06); discover counts at runtime
- Human-only gates are listed separately and never auto-pass
- BASE-07 records baseline audio metrics for representative factory presets before DSP changes (document method + captured numbers/paths in the phase baseline artifact or adjacent metrics note)
- Human-only gates (AU pluginval, Windows/Linux matrix when not run, DAW smoke, signing/notarization, etc.) are marked `human_needed` in verifier/checklist output — never silently treated as pass (BASE-08)
- Reuse patterns from `docs/RELEASE_CHECKLIST.md` honesty model (verified vs not verified)
- Metrics capture may be offline/scripted analysis of factory preset renders; exact tooling at implementer discretion within “representative presets before DSP change”

### Claude's Discretion
- Exact Catch2 file naming, helper fixtures, and how baseline metrics are computed/stored (as long as committed and reproducible)
- Whether to wrap existing ENAB gates inside `verify-v1.sh` or invoke them as subprocesses
- Incremental vs single-PR layout of contract files within Phase 19 plans

### Deferred Ideas (OUT OF SCOPE)
None — discussion stayed within phase scope. Production DSP/UI fixes deferred to Phases 20–25; RC packaging to Phase 27.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| BASE-01 | Record commit, branch, build config, test count, CI state, known manual gaps | `19-BASELINE.md` template; discover counts via `ctest -N` / `./Tests --list-tests`; CI from `.github/workflows/build_and_test.yml` |
| BASE-02 | REQUIREMENTS.md entries for every milestone requirement | Already present (128 milestone IDs + FUT-01…07 deferred); Phase 19 verifies completeness vs MILESTONE-SPEC, not greenfield authoring |
| BASE-03 | Map each requirement → one phase + ≥1 verification artifact | Extend `RequirementsTraceabilityTest` + planning tables with tags/doc paths; `[v1][contract]` tags for BASE defect harness |
| BASE-04 | Preserve ProperSRC/HF/dry-integrity/release-truth greens | Filter/exclude `[v1][contract]`; never edit `ParameterCurvesTest`, `MidiSendAmountTest`, `ReleaseTruthTest` MIDI case, `PostGateTimingTest` to encode failures |
| BASE-05 | Durable `scripts/verify-v1.sh` | Compose legal + cmake/build + ctest + ENAB gates + optional pluginval; report red/green; exit non-zero when automated gates fail |
| BASE-06 | No hard-coded test totals | Remove/avoid `113/113` pattern; discover at runtime in baseline + verifier |
| BASE-07 | Baseline audio metrics for factory presets | Offline `PluginProcessor` preset renders → RMS/peak (and optional spectrum) committed beside baseline |
| BASE-08 | Human gates `human_needed` | Verifier section + checklist honesty; never treat DAW/AU/signing as auto-pass |
</phase_requirements>

## Summary

Phase 19 is a **harness-and-truth freeze**, not a fix phase. Confirmed defects already live in production code (`PluginProcessor::processBlock`, `NoiseGate`, `ParameterCurves::inputGainDb`, `PressureSendPad::mouseUp`, faceplate/binary asset names). Existing Catch2 coverage largely documents **current** (pre-v1) behavior and must stay green (BASE-04). New dedicated `[v1][contract]` tests must assert the **v1-correct** contracts from the milestone / ADRs so they fail now for the right reason.

`REQUIREMENTS.md` already enumerates all 128 milestone requirement IDs (plus FUT deferred). The remaining BASE-02/03 work is verification artifact mapping and an extendable traceability test — not rewriting the catalog. The durable verifier must compose existing scripts (`check-legal-metadata.sh`, `enab-acceptance-gates.sh`) with configure/build/ctest, discover counts at runtime, and print an explicit `human_needed` section.

**Primary recommendation:** Restore the broken `cmake` submodule first (Wave 0), then ship three plans: (1) baseline + metrics + traceability, (2) failing `[v1][contract]` suite, (3) `verify-v1.sh` + honest human gates / checklist de-hardcoding.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Baseline report / metrics docs | Planning artifacts (repo docs) | Offline render via API/test harness | Freeze truth before DSP edits; metrics from deterministic offline renders |
| Requirement→phase→artifact map | Planning tables + Catch2 traceability | Verifier docs | BASE-02/03 are planning + automated anchors, not runtime product logic |
| Failing defect contracts | Catch2 unit/integration tests | — | Assert v1-correct DSP/UI/policy without changing production |
| Green suite preservation | Catch2 filtered suites / ctest -R/-E | `verify-v1.sh` | Prove BASE-04 by running ProperSRC/HF/DryPath/release without `[v1][contract]` |
| Durable automated gate runner | Shell scripts (`scripts/`) | GitHub Actions (later / Phase 27) | Local truthful gate entry; CI may call later |
| Human-only gates | Checklist / verifier status labels | — | Cannot be auto-green; `human_needed` only |
| Production DSP/UI fixes | — (out of scope) | Phases 20–25 | Locked: no production fixes in Phase 19 |

## Project Constraints (from .cursor/rules/)

No `.cursor/rules/` directory present in this workspace. [VERIFIED: filesystem]

Applicable repo constraints from `PROJECT.md` / milestone (treat as planning constraints):

- C++20 / JUCE 8 / CMake / Catch2 stack only
- No heap alloc / host notify / locks / logging / file I/O in `processBlock` (realtime) — Phase 19 contracts may *detect* violations, not fix them
- Parameter IDs / `NkMo` / `SbLm` / bundle ID immutable
- Human gates remain `human_needed`
- Clean-room: no third-party firmware/dumps/schematics

## Standard Stack

### Core

| Library / Tool | Version | Purpose | Why Standard |
|----------------|---------|---------|--------------|
| Catch2 | 3.8.1 (via CPM in `cmake/Tests.cmake`) | Unit/integration contracts | Already wired; `catch_discover_tests` → ctest [VERIFIED: cmake/Tests.cmake @ d5cb9b3; Context7 /catchorg/catch2] |
| CMake | ≥3.25 (local 3.30.3) | Build + test target | Project floor [VERIFIED: CMakeLists.txt; `cmake --version`] |
| ctest | same as CMake | Gate runner | CI + ENAB scripts already use it [VERIFIED: `.github/workflows/build_and_test.yml`, `scripts/enab-acceptance-gates.sh`] |
| JUCE | 8.0.12 (submodule) | PluginProcessor / APVTS test host | Existing plugin stack [VERIFIED: `.gitmodules`] |
| Apple clang++ | 21.0.0 (local) | Compile Tests | Available on research host [VERIFIED: `clang++ --version`] |

### Supporting

| Tool | Version | Purpose | When to Use |
|------|---------|---------|-------------|
| `scripts/check-legal-metadata.sh` | repo script | Legal/metadata scan | Always in verify-v1 automated set |
| `scripts/enab-acceptance-gates.sh` | repo script | ENAB-01 ProperSRC/HF ctest filter + ADR-003 | Invoke as subprocess from verify-v1 |
| pluginval | v1.0.3 (CI downloads) | VST3 strictness 10 | Optional via `RUN_PLUGINVAL=1` / `PLUGINVAL_BIN`; AU remains human/CI gap |
| bash | system | `verify-v1.sh` | Durable gate entry |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Dedicated `[v1][contract]` files | Flip existing green tests to EXPECT_FAIL | Forbidden — weakens BASE-04 and hides current truth |
| Hard-coded `113/113` | Runtime discovery | Forbidden by BASE-06; checklist already has stale hard-code |
| New npm/C++ packages | Existing Catch2 + shell | No new packages needed |

**Installation:** None — no new packages. Catch2 arrives via existing CPM when `cmake` submodule + `include(Tests)` work.

**Version verification:** Catch2 pin `gh:catchorg/Catch2@3.8.1` in `Tests.cmake` [VERIFIED: `git show d5cb9b3:Tests.cmake`]. npm `catch2` package-legitimacy check returned SLOP (wrong ecosystem) — do **not** install Catch2 from npm.

## Package Legitimacy Audit

> Phase 19 installs **no new external packages**.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| Catch2@3.8.1 | CPM/GitHub (existing) | mature | n/a (C++) | github.com/catchorg/Catch2 | OK (existing pin) | Keep — already in Tests.cmake |
| catch2 (npm) | npm | — | — | none | SLOP | REMOVED — wrong ecosystem; do not install |

**Packages removed due to [SLOP] verdict:** npm `catch2` (not applicable)
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```text
┌─────────────────────────────────────────────────────────────────┐
│ Phase 19 Harness (docs + tests + scripts only)                  │
│                                                                 │
│  git metadata / VERSION / CI names ──► 19-BASELINE.md           │
│  factory preset offline renders ─────► metrics section/file     │
│  REQUIREMENTS.md tables ─────────────► BASE-02/03 verification  │
│                                                                 │
│  tests/V1Contract*.cpp  [v1][contract][defect-id]               │
│       │                                                         │
│       ▼                                                         │
│  assert v1-CORRECT behavior ──► FAIL on current production code │
│                                                                 │
│  preexisting [release]/[hf][regression]|[DryPath]|ProperSRC     │
│       │                                                         │
│       ▼                                                         │
│  must still PASS (BASE-04)                                      │
│                                                                 │
│  scripts/verify-v1.sh                                           │
│    ├─ check-legal-metadata.sh                                   │
│    ├─ cmake configure/build (BUILD_DIR)                         │
│    ├─ ctest full suite (expect red from contracts)              │
│    ├─ enab-acceptance-gates.sh                                  │
│    ├─ optional pluginval (env-gated)                            │
│    └─ print human_needed gates (never auto-pass)                │
└─────────────────────────────────────────────────────────────────┘

Production DSP/UI (PluginProcessor, NoiseGate, pad, faceplate)
        ▲
        │  READ-ONLY in Phase 19 — contracts observe defects
        │  Fixes deferred to Phases 20–25
```

### Recommended Project Structure

```text
.planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/
├── 19-CONTEXT.md          # locked (exists)
├── 19-RESEARCH.md         # this file
├── 19-BASELINE.md         # BASE-01 + metrics summary (Plan 01)
└── 19-BASELINE-METRICS.md # optional adjacent metrics tables (discretion)

scripts/
└── verify-v1.sh           # BASE-05/06/08

tests/
├── V1ContractPressureReleaseTest.cpp      # or V1Contract*.cpp split
├── V1ContractOversizedBlockTest.cpp
├── V1ContractTrueBypassTest.cpp
├── V1ContractPostHardRampTest.cpp
├── V1ContractInputAnchorsTest.cpp
├── V1ContractMidiApvtsPurityTest.cpp
├── V1ContractShippingPolicyTest.cpp
└── RequirementsTraceabilityTest.cpp       # extend for BASE-03

# Auto-picked by cmake/Tests.cmake GLOB — no CMakeLists edit if GLOB intact
```

### Pattern 1: Assert the correct contract (fail now)

**What:** New tests REQUIRE the v1-correct behavior from ADRs / §8.4.
**When to use:** Every Phase 19 defect harness.
**Example:**

```cpp
// Source: Catch2 tags — https://github.com/catchorg/Catch2/blob/devel/docs/test-cases-and-sections.md
// Source: ADR-V1-08 / MILESTONE-SPEC §8.4 canonical Input
TEST_CASE("v1 Input anchors are -9/0/+9 dB", "[v1][contract][input-anchors][CORE-01]")
{
    using namespace sendbloom::ParameterCurves;
    REQUIRE(inputGainDb(0.0f) == Catch::Approx(-9.0f).margin(1e-4f));
    REQUIRE(inputGainDb(0.5f) == Catch::Approx(0.0f).margin(1e-4f));
    REQUIRE(inputGainDb(1.0f) == Catch::Approx(9.0f).margin(1e-4f));
}
// Current code returns +9 / ~0 / -3 — this FAILS for the intended reason.
```

### Pattern 2: Keep legacy green tests untouched

**What:** Preexisting tests that encode *current* buggy contracts stay as-is until the fix phase updates them.
**When to use:** MIDI APVTS mutation, Input curve `+9…-3`, PostHard timing-only checks.
**Evidence:** `tests/MidiSendAmountTest.cpp` and `tests/ReleaseTruthTest.cpp` currently REQUIRE `send_amount` updates from CC1 [VERIFIED: codebase]. `tests/ParameterCurvesTest.cpp` REQUIRES `inputGainDb(0)==9`, `(1)==-3` [VERIFIED: codebase].

### Pattern 3: Tag filtering for BASE-04 proof

**What:** Run green subsets excluding `[v1][contract]`.
**Example:**

```bash
# Source: Catch2 CLI — https://github.com/catchorg/Catch2/blob/devel/docs/command-line.md
./Tests "~[v1]"                 # exclude v1 contracts
./Tests "[release]"             # release-truth
./Tests "[hf][ringing][regression]"
./Tests "[DryPath]"
./Tests "[v1][contract]"        # expect failures until fix phases
```

### Anti-Patterns to Avoid

- **Weakening green tests to EXPECT current bugs:** Violates BASE-04 and locked CONTEXT.
- **Hard-coding `113/113` or any total:** Violates BASE-06; `docs/RELEASE_CHECKLIST.md` already contains this debt [VERIFIED: docs/RELEASE_CHECKLIST.md].
- **Treating `set -e` early exit as “all green” without a status table:** Verifier must report which gates are red, including expected contract failures.
- **Fixing DSP/UI in Phase 19:** Out of scope; would invalidate baseline.
- **Relying on `check-legal-metadata.sh` alone for REVERB X / reverbx filename:** Current banned list does not include those strings; shipping-policy contracts must cover them [VERIFIED: `scripts/check-legal-metadata.sh`, `source/ui/PedalFaceplatePaint.cpp`, `CMakeLists.txt`].

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Test discovery / registration | Manual CMake per-file lists | Existing `file(GLOB_RECURSE … tests/*.cpp)` + `catch_discover_tests` | New `tests/*.cpp` auto-register when cmake submodule healthy |
| Legal scan | New parallel grep framework | `scripts/check-legal-metadata.sh` + dedicated shipping-policy Catch2/source scan | Script already CI-wired; contracts cover gaps it misses |
| ENAB ProperSRC gates | Duplicate ctest filters | Invoke `scripts/enab-acceptance-gates.sh` | Already curated TEST-11 / ProperSRC regex |
| Mock reverb for pressure energy | Rewrite chain | `GatedBloomChain::setReverbEngineForTests` + tiny `IReverbEngine` test double | Hook already exists [VERIFIED: `source/GatedBloomChain.h`] |
| Metrics DAW capture | Manual listening as only baseline | Offline `PluginProcessor` + factory preset load | Reproducible, commitable, no DAW dependency |

**Key insight:** Phase 19 value is a durable red/green truth surface. Reuse the pamplejuce Catch2/ctest path and existing scripts; invent only the contract assertions and verifier composition.

## Common Pitfalls

### Pitfall 1: Breaking BASE-04 while adding contracts
**What goes wrong:** Editing `ParameterCurvesTest` / MIDI tests / ReleaseTruth MIDI case so “everything fails together,” or accidentally changing production code.
**Why it happens:** Desire to have a single source of truth for the new contract.
**How to avoid:** Only add new `[v1][contract]` files; run `./Tests "~[v1]"` (or tagged green suites) in verification steps.
**Warning signs:** Diffs under preexisting green test files; green suite starts failing for non-contract reasons.

### Pitfall 2: Contract fails for the wrong reason
**What goes wrong:** Oversized-block test fails due to prepare mismatch; bypass fails due to ramp still active; pressure test fails because mock not injected.
**Why it happens:** Setup not matching §8.4 sequences.
**How to avoid:** Follow milestone §8.4 recipes exactly; settle bypass >5 ms; prepare at 512 then call 2048; inject energy-tracking reverb for pressure.
**Warning signs:** Assertion messages about NaN/size rather than wet-zero / APVTS change / gain snap.

### Pitfall 3: Verifier reports false green on human gates
**What goes wrong:** Skipped AU pluginval / DAW smoke printed as PASS.
**Why it happens:** `set -e` + optional steps omitted without status lines.
**How to avoid:** Explicit `human_needed` section; optional pluginval only when `RUN_PLUGINVAL=1`; never map “skipped” → pass.
**Warning signs:** Checklist checkboxes flipped without evidence; verify-v1 exit 0 while contracts red.

### Pitfall 4: cmake submodule prevents building Tests
**What goes wrong:** `include(Tests)` fails; `ctest` shows 0 tests; metrics/contracts cannot run.
**Why it happens:** Local `cmake/` working tree has all module files deleted; parent submodule SHA `38bc0f5…` is not present in the submodule object store [VERIFIED: `git status` / `git cat-file`].
**How to avoid:** Wave 0 restore: checkout a valid cmake-includes commit containing `Tests.cmake` (e.g. current `d5cb9b3`) and restore deleted files; align parent submodule pointer.
**Warning signs:** `ctest -N` → Total Tests: 0; configure error on missing `Tests.cmake`.

### Pitfall 5: Full ctest red confuses “phase success”
**What goes wrong:** Executors treat Phase 19 as failed because contracts fail.
**Why it happens:** Habit that “all tests green = done.”
**How to avoid:** Phase 19 success = contracts exist and fail for intended reasons + green suite still passes + verifier truthfully reports red gates.
**Warning signs:** PRs that `#ifdef` out contracts or mark them `[.]` skipped.

## Code Examples

### Defect → contract map (current broken loci)

| Defect | Broken locus (current) | v1-correct assertion | Expected failure mode |
|--------|------------------------|----------------------|------------------------|
| Pressure release flip | `PressureSendPad::mouseUp` sets `send_connected=false` → `PressureSend::computeGain` returns 1.0 (always-on) [VERIFIED: `source/ui/PressureSendPad.cpp`, `source/PressureSend.h`] | After press→release: connected stays true, amount→0, mock reverb accepts no new energy, wet tail & dry nonzero (§8.4) | New wet energy after release OR disconnected=true |
| Oversized-block dry fallback | `if (numSamples > preparedMaxBlock_) { … constexpr auto wet = 0.0f; …}` + `dryBuffer.setSize` in process [VERIFIED: `source/PluginProcessor.cpp` ~205–275] | Prepare 48k/512; 2048 one-shot vs 4×512 max abs diff `< 2e-5`; wet nonzero | Wet zero / large diff |
| True bypass unity | Settled path still mono-sums (non-extended) and applies `OutputStage`/`outputGain` while crossfading dry/wet [VERIFIED: processBlock mix loop ~339–372; ADR-V1-10 wants final per-channel dry×(1−mix)+processed×mix ignoring output when settled] | Stereo anti-phase or distinct channels; bypass settled; each channel matches input `<1e-6`; not collapsed | Channel collapse and/or output gain applied |
| PostHard one-sample snap | `if (floorGain <= 0 && !isOpen) { gain = 0; return; }` [VERIFIED: `source/NoiseGate.h` 54–58] | First closed sample gain in (0,1); Δgain ≤0.05; ≤1e-4 within 1 ms | First closed sample already 0 |
| Input anchors | `inputGainDb = 9 + smoothstep*(−12)` → +9…−3 [VERIFIED: `source/ParameterCurves.h`]; no dB formatter on layout [VERIFIED: `ParameterLayout.cpp`] | −9 / 0 / +9 at 0 / 0.5 / 1 (ADR-V1-08) | Anchors mismatch |
| MIDI APVTS mutation | `sendParam->store(norm)` in `processBlock` [VERIFIED: `PluginProcessor.cpp` 176–188] | After CC1: APVTS `send_amount` unchanged; DSP pressure still changes (Phase 22 will supply RT path — contract may assert purity now and note DSP-effect gap, or use a temporary observable if available) | APVTS changes (current green tests expect this) |
| Banned shipping strings/filenames | Faceplate `"REVERB X"`; binary `reverbx-faceplate.png` in CMakeLists [VERIFIED: `PedalFaceplatePaint.cpp`, `CMakeLists.txt`] | No product-facing `REVERB X`; no shipping resource name containing `reverbx`; detect `dryBuffer.setSize` / raw `sendParam->store` in processBlock body (source scan like ReleaseTruth) | Strings/filenames still present |

### MIDI purity contract sketch

```cpp
TEST_CASE("v1 MIDI CC1 must not mutate APVTS send_amount", "[v1][contract][midi-apvts][MIDI-03]")
{
    // setup connected, send_amount=0, prepare…
    const float before = *apvts.getRawParameterValue(ParameterIDs::sendAmount);
    midi.addEvent(juce::MidiMessage::controllerEvent(1, 1, 100), 0);
    plugin.processBlock(buffer, midi);
    REQUIRE(*apvts.getRawParameterValue(ParameterIDs::sendAmount) == Catch::Approx(before));
}
// Fails today because processBlock stores CC1 into APVTS.
// Do NOT modify MidiSendAmountTest / ReleaseTruth MIDI case in Phase 19.
```

### PostHard ramp contract sketch

```cpp
TEST_CASE("v1 PostHard close uses sub-ms ramp not snap", "[v1][contract][posthard][CORE-10]")
{
    sendbloom::NoiseGate gate;
    gate.prepare(48000.0, sendbloom::GateProfile::PostHard);
    gate.process(/*open env*/, -40.0f);
    REQUIRE(gate.getIsOpen());
    const float g0 = gate.getGain();
    const float g1 = gate.process(0.0f, -40.0f); // first close sample
    REQUIRE(g1 > 0.0f);
    REQUIRE(g1 < 1.0f);
    REQUIRE(std::abs(g0 - g1) <= 0.05f);
}
```

### verify-v1.sh composition (recommended)

```bash
#!/usr/bin/env bash
set -uo pipefail
# Intentionally not blanket set -e: collect statuses, print table, exit non-zero if any automated gate failed.
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${BUILD_DIR:-$ROOT/Builds}"
# 1) legal  2) configure if needed  3) build  4) ctest (discover count via ctest -N)
# 5) bash "$ROOT/scripts/enab-acceptance-gates.sh"
# 6) optional pluginval if RUN_PLUGINVAL=1
# 7) echo "HUMAN_NEEDED:" list (AU pluginval, Win/Linux if not run, DAW smoke, signing, notarization, license decision)
# Discover: ctest --test-dir "$BUILD" -N | tail -1
# Never echo a hard-coded total.
```

[CITED: MILESTONE-SPEC §8.5 Durable verifier]

### Baseline metrics approach (discretion — recommended)

1. Load each of 8 factory presets via `setCurrentProgram` / XML (pattern in `ReleaseTruthTest`).
2. Render N blocks of deterministic sine/pluck at 48 kHz / 512 with fixed seeds.
3. Record per-preset: peak, RMS, optional band energy; store tables in `19-BASELINE.md` or `19-BASELINE-METRICS.md`.
4. Record method (command / test name) so Phase 20+ can re-run for deltas.
5. Also snapshot preset parameter matrix (already partially visible in XML) into baseline.

Factory baseline truth to record (not fix): pressure presets `Firm_Pressure` / `Hot_Clip` load `send_connected=1` with `send_amount` 0.85/1.0 (not at-rest 0) [VERIFIED: preset XML]. Default layout `send_amount` default is `1.0f` [VERIFIED: `ParameterLayout.cpp`].

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Hard-coded checklist `113/113` | Runtime-discovered counts | Phase 19 (BASE-06) | Checklist/verifier must stop encoding totals |
| Encode bugs into green tests | Separate failing `[v1][contract]` files | Phase 19 | BASE-04 preserved; fix phases flip contracts green later |
| Ad-hoc local ctest + legal | `scripts/verify-v1.sh` durable entry | Phase 19 / used again in 27 | Single automated gate surface |

**Deprecated/outdated:**

- Treating ReleaseTruth MIDI “updates send_amount” as v1 correctness — it is pre-v1 documentation of a defect [VERIFIED: ReleaseTruthTest].
- Treating ParameterCurvesTest `+9…-3` as canonical — superseded by ADR-V1-08 `-9/0/+9` for v1 contracts.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | MIDI purity contract can fail on APVTS mutation alone in Phase 19; proving “DSP pressure still changes” may need Phase 22 RT path or a temporary test double | Code Examples / MIDI | Contract may only cover MIDI-03 purity half of §8.4 until Phase 22 |
| A2 | Metrics via offline PluginProcessor renders are sufficient for BASE-07 without DAW | Standard Stack / metrics | User may want WAV fixtures; still within discretion if committed & reproducible |
| A3 | Parent git should point cmake submodule at a reachable commit with Tests.cmake (d5cb9b3-class), not missing `38bc0f5` | Pitfalls / Environment | Wave 0 scope may include submodule pointer fix |

**If A1 materializes:** Still ship APVTS purity + source-policy `sendParam->store` scan in Phase 19; document DSP-effect half as blocked until RT MIDI exists.

## Open Questions

1. **Submodule pointer repair scope**
   - What we know: local `cmake/` files deleted; parent records unreachable `38bc0f5`; `d5cb9b3` has `Tests.cmake`.
   - What's unclear: whether CI currently relies on a different machine state / cached deps.
   - Recommendation: Wave 0 task — restore files + align submodule SHA before any baseline metrics capture.

2. **MIDI §8.4 “DSP pressure changes” without RT path**
   - What we know: today CC1 only writes APVTS; no separate RT modulation lane yet.
   - What's unclear: whether Phase 19 must fail only purity, or also fail a positive DSP-effect assertion.
   - Recommendation: Prefer purity + source scan now; note positive DSP half as Phase 22 companion (A1).

3. **Single vs split contract translation units**
   - Discretion: one `V1ContractHarnessTest.cpp` vs per-defect files.
   - Recommendation: Per-defect files for clearer ctest names and parallel plan execution.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| cmake | configure/build/tests | ✓ | 3.30.3 | — |
| ctest | gates | ✓ | 3.30.3 | — |
| clang++ | build Tests | ✓ | Apple 21.0.0 | — |
| ninja | optional generator | ✗ (not probed as installed) | — | Unix Makefiles / Xcode |
| pluginval | optional VST3 gate | ✗ locally | — | `RUN_PLUGINVAL=0`; mark human/CI |
| `cmake/Tests.cmake` on disk | Catch2 target | ✗ (deleted in working tree) | — | **Restore submodule (blocking)** |
| Builds with discovered tests | baseline counts / metrics | ✗ (`ctest -N` → 0) | — | Reconfigure after cmake restore |
| Catch2 via CPM | Tests | ✓ when Tests.cmake present | 3.8.1 pin | — |

**Missing dependencies with no fallback:**

- Healthy `cmake/` submodule checkout including `Tests.cmake` (blocks all automated Phase 19 verification)

**Missing dependencies with fallback:**

- Local pluginval → optional env gate + `human_needed`
- ninja → use default CMake generator

## Validation Architecture

> `workflow.nyquist_validation` is enabled (`true`) in `.planning/config.json`.

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 + ctest (`Tests` executable) |
| Config file | `cmake/Tests.cmake` (via `include(Tests)` in root `CMakeLists.txt`) |
| Quick run command | `ctest --test-dir "$BUILD_DIR" -C Release -R 'release|DryPath|HF ringing|ProperSRC' --output-on-failure` (tune after restore) |
| Full suite command | `ctest --test-dir "$BUILD_DIR" -C Release --output-on-failure` |
| Contract-only | `"$BUILD_DIR/Tests" "[v1][contract]"` (expect fail) |
| Green-without-contracts | `"$BUILD_DIR/Tests" "~[v1]"` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| BASE-01 | Baseline artifact fields | doc + script discovery | `ctest -N`; `git rev-parse`; write `19-BASELINE.md` | ❌ Wave 0/Plan01 |
| BASE-02 | REQUIREMENTS completeness | doc audit / Catch2 optional | Diff ID sets vs MILESTONE-SPEC | ✅ mostly (verify) |
| BASE-03 | phase + artifact map | unit/doc | Extend `RequirementsTraceabilityTest` | ⚠️ partial |
| BASE-04 | green ProperSRC/HF/DryPath/release | regression | `./Tests "~[v1]"` + ENAB script | ✅ existing |
| BASE-05 | verify-v1.sh runs gate set | smoke script | `bash scripts/verify-v1.sh` | ❌ Wave 0 |
| BASE-06 | no hard-coded totals | script/doc lint | grep checklist/scripts for `\d+/\d+` | ⚠️ checklist debt |
| BASE-07 | preset metrics recorded | offline render + doc | metrics helper or Catch2 diagnostic | ❌ Wave 0 |
| BASE-08 | human_needed labels | script output | verify-v1 human section | ❌ Wave 0 |
| Defect contracts | seven defect classes | contract (expect fail) | `./Tests "[v1][contract]"` | ❌ Plan 02 |

### Sampling Rate

- **Per task commit:** green filter `./Tests "~[v1]"` (or ENAB + `[release]` + `[DryPath]`) must stay green; contracts remain red
- **Per wave merge:** `bash scripts/verify-v1.sh` produces truthful status table
- **Phase gate:** All BASE-01…08 artifacts present; contracts fail for intended reasons; no production DSP/UI diffs

### Wave 0 Gaps

- [ ] Restore `cmake/` submodule files / valid SHA so `include(Tests)` works
- [ ] Reconfigure `Builds` and confirm `ctest -N` discovers tests (currently 0)
- [ ] `scripts/verify-v1.sh` — does not exist yet
- [ ] `19-BASELINE.md` (+ metrics) — does not exist yet
- [ ] `tests/V1Contract*.cpp` — do not exist yet
- [ ] Extend `RequirementsTraceabilityTest` / tables for BASE-03 artifact tags
- [ ] Remove or reword hard-coded `113/113` in `docs/RELEASE_CHECKLIST.md` (BASE-06)

## Security Domain

> `security_enforcement` enabled; ASVS level 1.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | N/A — audio plugin harness |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | yes (light) | Deterministic MIDI/audio fixtures; bounds in Catch2 asserts; no untrusted network I/O in harness |
| V6 Cryptography | no | N/A for Phase 19 |
| V5 / supply chain | yes | Do not add unverified packages; Catch2 only via existing CPM pin; no npm Catch2 |

### Known Threat Patterns for this stack

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Slopsquat / wrong-ecosystem package | Tampering | No new deps; legitimacy gate; CPM pin only |
| Hiding failing gates as pass | Repudiation | Honest verifier status; `human_needed` never auto-pass |
| XML entity expansion in preset load | Tampering | Existing `XmlDocumentEntityExpansionTest`; don't weaken |
| Zip slip in resources | Tampering | Existing `ZipDecompressionBoundsTest`; don't weaken |

## Recommended Plan Breakdown (2–4 plans)

### Plan 19-01 — Wave 0 + Baseline + Traceability + Metrics (BASE-01, BASE-02, BASE-03, BASE-07)
1. Restore `cmake` submodule / `Tests.cmake`; reconfigure Builds; record discovered test count.
2. Author `19-BASELINE.md` (commit, branch, VERSION `0.0.1`, build/CI names, manual gaps, preset matrix notes).
3. Capture factory-preset offline metrics; commit method + numbers.
4. Verify REQUIREMENTS completeness vs MILESTONE-SPEC; extend traceability test/tags for artifact mapping.
5. Strip hard-coded totals from checklist wording (BASE-06 start).

### Plan 19-02 — Failing defect contracts (BASE-04 + defect list)
1. Add dedicated `[v1][contract][…]` Catch2 files for all seven defect classes using §8.4 / ADR recipes.
2. Prove green suite: `./Tests "~[v1]"` (and ENAB) still passes.
3. Document expected failure messages/reasons in baseline or plan notes.
4. **No** production DSP/UI changes; **no** edits that weaken preexisting green tests.

### Plan 19-03 — Durable verifier + human gates (BASE-05, BASE-06, BASE-08)
1. Add `scripts/verify-v1.sh` composing legal + build + ctest + ENAB (+ optional pluginval).
2. Discover counts at runtime; print red/green table including expected contract reds.
3. Explicit `human_needed` section (AU pluginval, Win/Linux if not run, DAW smoke, signing/notarization, license decision).
4. Ensure exit code reflects automated gate truth without claiming human gates passed.

*(Optional merge of 19-01 metrics into 19-03 if planner prefers 2 plans; do not merge contracts with verifier — contracts are the largest risk surface for BASE-04.)*

## Sources

### Primary (HIGH confidence)
- Codebase: `source/PluginProcessor.cpp`, `NoiseGate.h`, `ParameterCurves.h`, `PressureSendPad.cpp`, `PressureSend.h`, `CMakeLists.txt`, `PedalFaceplatePaint.cpp`
- Existing tests: `MidiSendAmountTest.cpp`, `ParameterCurvesTest.cpp`, `ReleaseTruthTest.cpp`, `PostGateTimingTest.cpp`, `RequirementsTraceabilityTest.cpp`
- Scripts: `scripts/enab-acceptance-gates.sh`, `scripts/check-legal-metadata.sh`
- Planning: `19-CONTEXT.md`, `REQUIREMENTS.md`, `ROADMAP.md` Phase 19, `MILESTONE-SPEC` §8 / ADR-V1-08/10 / BASE-01…08
- Catch2 Context7 `/catchorg/catch2` — tags, CLI filters, `catch_discover_tests`
- `cmake` submodule `Tests.cmake` @ `d5cb9b3` (Catch2@3.8.1 CPM pin)

### Secondary (MEDIUM confidence)
- `docs/RELEASE_CHECKLIST.md` honesty model + stale `113/113`
- `.github/workflows/build_and_test.yml` matrix + VST3-only pluginval
- classify-confidence: Context7 verified → MEDIUM tier from seam (treated as authoritative for Catch2 API)

### Tertiary (LOW confidence)
- Exact optimal metrics feature set (RMS vs spectrum) — discretion / A2
- MIDI DSP-effect half of §8.4 before Phase 22 — A1

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — existing Catch2/CMake/JUCE path verified in-repo
- Architecture: HIGH — defect loci and harness boundary verified in source
- Pitfalls: HIGH — BASE-04 / cmake submodule / false-green human gates grounded in evidence
- Metrics/MIDI DSP half: MEDIUM/LOW — discretion + Phase 22 dependency

**Research date:** 2026-07-12
**Valid until:** 2026-08-11 (30 days; stack stable) / re-check immediately if cmake submodule pointer changes
