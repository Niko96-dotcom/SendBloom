---
phase: 24-reverb-state-wet-dirt-integrity
verified: 2026-07-12T20:30:00Z
verifier: ZCode
status: passed
binary: Builds/Tests
head: ee28164
full_suite: "259 test cases | 251 passed | 7 failed | 1 skipped | 27765559 assertions"
phase_24_contracts: all green
shipping_policy: red (expected — Phase 25 scope)
requirements_total: 15
requirements_verified: 15
requirements_gaps: 0
---

# Phase 24 Verification: Reverb State & Wet-Dirt Integrity

**Phase goal:** Predelay, modulation, ProperSRC underfill, and wet-dirt filtering are correct without retuning the reverb character.

**Result:** PASSED. All 15 requirements (DSP-01..DSP-15) are verified by passing automated tests. All three success criteria are met. All Phase 20-23 regression gates stay green. `[shipping-policy]` correctly remains red. The three test-method corrections in plan 24-03 are sound per spec §17.2 line 2687.

## Requirement Verification Matrix

Every requirement ID from the phase frontmatter (DSP-01..DSP-15) is accounted for and verified.

| Req | Description | Verified | Command | Result |
|-----|-------------|----------|---------|--------|
| DSP-01 | Predelay line clocked continuously in bright and dark | YES | `Builds/Tests "[DSP-01]"` | All tests passed (2 assertions in 1 test case) |
| DSP-02 | Dark mode uses fixed 55 ms tap blended by dark mix | YES | `Builds/Tests "[DSP-02]"` | All tests passed (3 assertions in 2 test cases) |
| DSP-03 | Re-enabling Dark emits no stale frozen burst | YES | `Builds/Tests "[DSP-03]"` | All tests passed (1 assertion in 1 test case) |
| DSP-04 | Bright/dark automation finite and click-bounded | YES | `Builds/Tests "[DSP-04]"` | All tests passed (96001 assertions in 1 test case) |
| DSP-05 | LFO modulation depth invariant in seconds across rates | YES | `Builds/Tests "[DSP-05]"` | All tests passed (6 assertions in 1 test case) |
| DSP-06 | ProperSRC output is pre-cleared | YES | `Builds/Tests "[DSP-06]"` | All tests passed (50 assertions in 2 test cases) |
| DSP-07 | ProperSRC unwritten samples remain zero | YES | `Builds/Tests "[DSP-07]"` | All tests passed (49 assertions in 1 test case) |
| DSP-08 | Existing ProperSRC imaging/HF gates stay green | YES | `Builds/Tests "[verb][FixedRateAdapter][SRC-06]"` + `Builds/Tests "[hf]"` | SRC-06: 2 assertions in 1 case; HF: 144006 assertions in 6 cases |
| DSP-09 | Wet dirt 100 Hz pre-clip high-pass | YES | `Builds/Tests "[DSP-09]"` | All tests passed (3 assertions in 2 test cases) |
| DSP-10 | Wet dirt 20 Hz post-clip DC blocker | YES | `Builds/Tests "[DSP-10]"` | All tests passed (1 assertion in 1 test case) |
| DSP-11 | Wet dirt long-run DC offset below gate | YES | `Builds/Tests "[DSP-11]"` | All tests passed (1 assertion in 1 test case) |
| DSP-12 | Dry path unaffected by all wet filtering | YES | `Builds/Tests "[DSP-12]"` + `Builds/Tests "[DryPath]"` | DSP-12: 5 assertions in 1 case; DryPath: 16389 assertions in 4 cases |
| DSP-13 | `dirt_os` stays disabled and unimplemented | YES | `Builds/Tests "[DSP-13]"` | All tests passed (4101 assertions in 2 test cases) |
| DSP-14 | `authentic_color` off by default | YES | `Builds/Tests "[release][safe]"` | All tests passed (24020 assertions in 3 test cases) |
| DSP-15 | All factory presets keep `authentic_color=0` | YES | `Builds/Tests "[release][safe]"` | All tests passed (24020 assertions in 3 test cases) |

Aggregate contract filters (the phase-accepted groupings) all green:
- `[v1][contract][predelay]` → 96007 assertions in 5 test cases
- `[v1][contract][mod-invariant]` → 6 assertions in 1 test case
- `[v1][contract][src-underfill]` → 99 assertions in 3 test cases
- `[v1][contract][wet-dirt]` → 4111 assertions in 7 test cases

## Success Criteria Check

### Criterion 1 — Predelay continuous clock / fixed 55 ms tap / no stale burst / bounded automation: MET

Source evidence (`source/SchroederTankCore.h`):
- Line 29: `predelayLine.setDelay(... kDarkPredelaySeconds ... * processingRate_)` — fixed delay set once in `prepare`, not per-block.
- Lines 112-114: unconditional `pushSample` / `popSample` every sample + `x = input + darkMix_ * (delayed - input)` lerp. No `predelaySamples > 0.5f` conditional gate remains.
- Line 95: `darkMix_ = juce::jlimit(0.0f, 1.0f, darkMix)` — blend only; `updateCoeffs` no longer mutates delay length.
- `source/LegacyAccumulatorPath.h` line 33 mirrors the same fixed-delay/continuous-clock topology (SRC-05 parity).

Tests: `[predelay]` 5/5 green (DSP-01..04 tags each resolve). DSP-03 stale-burst-after-bright-silence passes. DSP-04 automation ramp passes (96001 assertions).

### Criterion 2 — Mod depth invariant in seconds / ProperSRC pre-clear / unwritten zero / imaging green: MET

Source evidence:
- `source/SchroederTank32DelayTable.h` line 29: `kTankLfoDepthSeconds = 16.0 / 32768.0`; line 33: `tankLfoDepthSamplesForRate(double rate)`. At `kInternalRate` this still equals 16 samples (character preserved). `kTankLfoHz` unchanged.
- `source/FixedRateAdapter.h` lines 46 & 65: `std::fill(out, out + n, 0.0f)` before `downsample` in the ProperSRC branch (and Off branch already cleared).

Tests: `[mod-invariant]` 1/1, `[src-underfill]` 3/3, `[verb][FixedRateAdapter][SRC-06]` 1/1, `[hf]` 6/6 (144006 assertions) all green.

### Criterion 3 — Wet dirt HP/DC / DC below gate / dry unaffected / dirt_os off / authentic_color off: MET

Source evidence (`source/WetOverdrive.h`):
- Line 45: `kPreClipHpHz = 100.0f`; line 47: `kPostClipDcBlockHpHz = 20.0f` (new named constant).
- Line 189: `class OnePoleHighpass` (allocation-free, mirrors `OnePoleLowpass`).
- Lines 240 & 244: dirty chain wired in `processFilteredBranch` — `preClipHp.process` then `postClipDcBlock.process`, blend semantics in `process(wet, distnBlend)` unchanged.
- `source/GatedBloomChain.h`: `dirt_os` / `dirtOs` references = 0 (the only `dirt_os` token in `source/` is the parameter-ID string in `ParameterIDs.h`, never read in the DSP path).

Tests: `[wet-dirt]` 7/7 green (DSP-09..13). `[release][safe]` 3/3 green (DSP-14/15 — authentic_color default off and all presets recall off). `[DryPath]` 4/4 green (dry unaffected).

## Test-Method Corrections Evaluation (spec §17.2 line 2687)

Three corrections made in plan 24-03, all documented in `24-03-SUMMARY.md`. Evaluated against the rule that a threshold may be adjusted only with evidence that the test method (not the implementation) is wrong. All three are sound:

1. **DSP-09 settle window 1 s → 2 s** (`tests/V1ContractWetDirtTest.cpp:92`). Measurement-only; frequency (30 Hz) and threshold (`lowRms < midRms * 0.25f`) unchanged. The 100 Hz one-pole HP's 30 Hz attenuation ratio converges slowly; at 1 s the ratio was 0.2508 (just over gate), at 2 s it is 0.2491 (under gate). A prior WIP relaxation of frequency 30 Hz → 20 Hz was correctly reverted. Legitimate test-method fix.

2. **DSP-11 metric `mean(abs(y))` → `abs(mean(y))`** (`tests/V1ContractWetDirtTest.cpp:158`). The spec §17.2 gate is named "Wet dirt DC mean < 1e-4" — the DC offset. `mean(abs(y))` is signal magnitude (≈0.37 for a full-scale sine) and is physically unsatisfiable under `<1e-4`. `abs(mean(y))` is the DC the blocker removes (measured 2.1e-5). Clarifying comment cites §17.2. Legitimate.

3. **GatedBloomChainTest "dirt increases wet magnitude" stimulus DC → 220 Hz tone** (`tests/GatedBloomChainTest.cpp:74-79`). ADR-V1-15's 100 Hz pre-clip HP strips DC/sub-bass, so a DC burst now makes dirty quieter than clean. A 220 Hz tone sits above the HP corner and exercises the clipper's added harmonics — the behavior under test. Mixer-math assertions unchanged. Legitimate.

None relax an underlying requirement. All are test-method (not threshold) corrections with documented physical evidence, exactly as spec §17.2 permits.

## Regression Gates (must stay green): MET

| Filter | Exit | Result |
|--------|------|--------|
| `[pressure-release]` | 0 | green |
| `[oversized-block]` | 0 | green |
| `[true-bypass]` | 0 | green |
| `[midi-apvts]` | 0 | green |
| `[input-anchors]` | 0 | green |
| `[posthard]` | 0 | green |
| `[per-sample]` | 0 | green |
| `[realtime]` | 0 | green |
| `[authentic]` | 0 | green |
| `[DryPath]` | 0 | green (16389 assertions / 4 cases) |

## Must-Stay-Red Gate: MET

`Builds/Tests "[v1][contract][shipping-policy]"` → exit 42 (non-zero). Correctly remains red — Phase 25 scope.

## Full-Suite Failure Inventory

Full suite: **259 test cases | 251 passed | 7 failed | 1 skipped | 27765559 assertions.**

The 7 failures are all pre-existing and unrelated to Phase 24, matching the verification-context baseline exactly:

| Failure | File:Line | Category | Phase 24? |
|---------|-----------|----------|-----------|
| V1ContractShippingPolicyTest | :100, :111 | Expected red (Phase 25 scope) | No |
| ReleaseTruthTest | :298 | `chain.processBlock` source-text static check | No |
| XmlDocumentEntityExpansionTest | :33, :48, :75 | JUCE library security test (environment/version) | No |
| ZipDecompressionBoundsTest | :115 | JUCE library security test (environment/version) | No |

None are Phase 24 requirements. All present at HEAD before plan 24-03.

## Cross-Reference: Plan Frontmatter Requirement IDs

- Plan 24-01 claims DSP-01, DSP-02, DSP-03, DSP-04 → all 4 verified (tags resolve, `[predelay]` 5/5 green).
- Plan 24-02 claims DSP-05, DSP-06, DSP-07, DSP-08 → all 4 verified (tags resolve, `[mod-invariant]`/`[src-underfill]`/`[SRC-06]`/`[hf]` green).
- Plan 24-03 claims DSP-09, DSP-10, DSP-11, DSP-12, DSP-13, DSP-14, DSP-15 → all 7 verified (tags resolve, `[wet-dirt]` 7/7 + `[release][safe]` 3/3 green).

Union of plan requirement IDs = DSP-01..DSP-15 with no gaps and no orphan IDs. Every ID maps to at least one passing automated test.

## Advisory Notes (non-blocking)

1. **Doc-staleness in tracking files.** `REQUIREMENTS.md` still marks DSP-09..DSP-15 as `[ ]` / "Pending" (lines 101-107) and the traceability table (lines 306-312) lists them "Pending", despite the implementation, tests, and `24-03-SUMMARY.md` all confirming completion. `ROADMAP.md` line 151 shows "Plans: 2/3 plans executed" and line 156 lists `24-03-PLAN.md` as `[ ]`. These are tracking-file lags, not real gaps — the code and tests are complete and green. The orchestrator should refresh REQUIREMENTS.md (DSP-09..15 → `[x]`/Complete) and ROADMAP.md (24-03 → `[x]`, "3/3 plans executed") when committing this verification.

2. **Code-review advisories (IN-01..IN-04 in 24-REVIEW.md)** are all info-level and non-blocking: dead `hostScratch`/`upOut_`/`downOut_` members (pre-existing, Phase 13), `kTankLfoDepthSamples` retained-but-unused runtime constant (deprecate or delete later), DSP-09 gate tightness (~0.4% margin, by design), and `LegacyAccumulatorPath` predelay authored in internal samples (correct but non-obvious). None affect Phase 24 goal achievement.

## Conclusion

Phase 24 goal achieved. All 15 requirements (DSP-01..DSP-15) verified by passing automated tests with direct evidence. All three success criteria met. No reverb character retuned (comb delays, damping maps, `kTankLfoHz`, tank gain, `kProperSrcQuality` all unchanged). `dirt_os` stays disabled; `authentic_color` stays off by default and in all factory presets. Phase 20-23 regression gates all green; `[shipping-policy]` correctly red. The only follow-up is the advisory doc-tracking refresh noted above.
