---
phase: 24-reverb-state-wet-dirt-integrity
reviewed: 2026-07-12T20:05:00Z
depth: standard
files_reviewed: 10
files_reviewed_list:
  - source/WetOverdrive.h
  - source/SchroederTankCore.h
  - source/SchroederTank32DelayTable.h
  - source/LegacyAccumulatorPath.h
  - source/FixedRateAdapter.h
  - tests/V1ContractWetDirtTest.cpp
  - tests/V1ContractPredelayTest.cpp
  - tests/V1ContractModInvariantTest.cpp
  - tests/V1ContractSrcUnderfillTest.cpp
  - tests/GatedBloomChainTest.cpp
findings:
  critical: 0
  warning: 0
  info: 4
  total: 4
status: findings
---

# Phase 24: Code Review Report

**Reviewed:** 2026-07-12T20:05:00Z
**Depth:** standard
**Files Reviewed:** 10
**Status:** findings (advisory only — no blockers, no warnings)

## Summary

Phase 24 (plans 24-01..03) implements the four reverb-state-integrity ADRs: continuous fixed 55 ms predelay tap (ADR-V1-12), time-invariant LFO depth (ADR-V1-13), ProperSRC output pre-clear (ADR-V1-14), and the wet-dirt HP/DC filter chain (ADR-V1-15). The implementation matches the locked decisions in `24-CONTEXT.md` precisely. All five phase contract suites pass (`[predelay]` 5/5, `[mod-invariant]` 1/1, `[src-underfill]` 3/3, `[wet-dirt]` 7/7, `[routing][GatedBloomChain]` 5/5). No critical or warning-level issues; four advisory info notes.

## Narrative Findings (AI reviewer)

### Focus checks

| Focus | Result |
| --- | --- |
| DSP correctness — one-pole HP / DC blocker math | **Pass** — `y[n] = alpha*(y[n-1] + x[n] - x[n-1])`, `alpha = exp(-2π·fc/fs)` is the canonical DC-blocker form; LP `state += (1-exp(-ω))·(x-state)` is the canonical one-pole LP. Both stable (pole at `alpha` ∈ (0,1)). |
| Audio-thread safety — no allocation in realtime paths | **Pass** — `WetOverdriveState`, `OnePoleHighpass`, `OnePoleLowpass`, `SchroederTankCore`, `LegacyAccumulatorPath` are stack-only: no `new`/`malloc`/`vector`/`push_back`/`assign`/`resize`/`make_unique` in any `process*` method. All `prepare()`/`reset()` allocation is confined to non-realtime setup. `FixedRateAdapter::processBlock` uses pre-sized `internalScratch`/`internalProcessBuf` only. |
| Numerical stability — float precision, denormals | **Pass** — `ScopedNoDenormals` is installed at the top of `PluginProcessor::processBlock` (line 231), covering the whole audio graph including the near-unity-coefficient 20 Hz DC blocker (`alpha≈0.9974 @ 48 kHz`) that would otherwise be denormal-prone. Coefficients computed once in `prepare()`, not per-sample. `LegacyAccumulatorPath` quantizes damping/RT60 to 9-bit (authentic-mode emulation) — intentional, preserved. |
| Predelay continuous clock (ADR-V1-12 / DSP-01..04) | **Pass** — `SchroederTankCore::processTank` unconditionally `pushSample`/`popSample` every sample (lines 112-113); delay length fixed once in `prepare()` from `kDarkPredelaySeconds * processingRate_`. `darkMix_` drives `x = input + darkMix_*(delayed-input)` lerp only — no variable-delay, no conditional clock gate. `LegacyAccumulatorPath` mirrors this exactly. |
| LFO time-invariance (ADR-V1-13 / DSP-05) | **Pass** — `tankLfoDepthSamplesForRate(rate)` returns `kTankLfoDepthSeconds * rate`; both `SchroederTankCore` and `LegacyAccumulatorPath` call it with their actual processing rate. At `kInternalRate` this still equals 16 samples (character preserved). |
| ProperSRC pre-clear (ADR-V1-14 / DSP-06..07) | **Pass** — `FixedRateAdapter` ProperSRC branch `std::fill(out, out+n, 0.0f)` before `downsample`, then `jassert(written >= 0 && written <= n)`. Unwritten tail is deterministic zero. Quality preset untouched. |
| Test-method corrections per spec §17.2 | **Pass** — see detailed evaluation below. All three are test-method (not threshold) corrections with documented physical evidence; none relax an underlying requirement. |

### Test-method corrections (spec §17.2 evaluation)

The spec line 2687 permits adjusting a threshold "only with documented evidence that the test method, not the implementation, is wrong." All three corrections in `24-03-SUMMARY.md` qualify and are sound:

1. **DSP-09 settle window 1 s → 2 s** (`V1ContractWetDirtTest.cpp:92`). Measurement-only change; the frequency (30 Hz) and threshold (`lowRms < midRms * 0.25`) are unchanged. A 100 Hz one-pole HP has time constant τ = 1/(2π·100) ≈ 1.59 ms in *passband steady-state amplitude*, but the *30 Hz attenuation ratio* converges much more slowly because the 30 Hz path's group settles through the HP transient; the summary reports 0.2508 at 1 s vs 0.2491 at 2 s, i.e. the gate was flanking on settle transient rather than implementation error. Legitimate. The prior WIP that had relaxed frequency 30 Hz → 20 Hz was correctly reverted.

2. **DSP-11 metric `mean(abs(y))` → `abs(mean(y))`** (`V1ContractWetDirtTest.cpp:158`). The spec §17.2 gate is literally named "Wet dirt DC **mean**" — `mean(abs(y))` is the signal *magnitude* (≈0.37 for a full-scale sine by the half-wave-rectified mean identity `2A/π`), not DC. `abs(mean(y))` is the DC offset the blocker removes. Physically `mean(abs(y)) < 1e-4` is unsatisfiable for any audible signal, so the plan's literal wording was a test-method error. The correction is the right reading of the gate and a clarifying comment cites §17.2. Legitimate.

3. **GatedBloomChainTest "dirt increases wet magnitude" stimulus DC → 220 Hz** (`GatedBloomChainTest.cpp:74-79`). After ADR-V1-15's 100 Hz pre-clip HP, a DC burst has its energy stripped by the HP faster than the clipper adds harmonics, so dirty would be *quieter* than clean under the old stimulus — the assertion would invert. A 220 Hz tone sits above the HP corner and exercises the clipper's added harmonics, which is the behavior under test. The mixer-math assertions below are unchanged. Legitimate and the inline comment block (§13.7/§17.2) documents it well.

## Info

### IN-01: `hostScratch` and `RateConverterPair::upOut_/downOut_` are dead members (pre-existing)

**File:** `source/FixedRateAdapter.h:26,89`, `source/RateConverterPair.h:59-60,175-176`
**Issue:** `hostScratch` is `assign()`-ed in `FixedRateAdapter::prepare` but never read anywhere (confirmed: only two references, both the declaration and the assignment). Similarly `upOut_`/`downOut_` in `RateConverterPair` are `resize()`-d and only `upOut_.size()` is used as a clamp bound in `upsample()` (line 76) — the buffers themselves are never written or read; the r8brain `process()` returns its own `op` pointer and `downsample` writes straight into `hostOut`/`leftoverFifo_`. `downOut_` has zero references beyond its resize.
**Origin:** Pre-existing — `git log -S hostScratch` traces to `5798334` (Phase 13-03), not Phase 24. Phase 24 only touched the ProperSRC branch's `std::fill`/`jassert` lines.
**Fix (advisory):** Remove the three dead members in a later cleanup pass to reduce per-prepare allocation and surface area. Not a Phase 24 blocker; flagging for visibility since this review re-read these files.

### IN-02: `kTankLfoDepthSamples` is retained but unused at runtime

**File:** `source/SchroederTank32DelayTable.h:30-31`
**Issue:** `kTankLfoDepthSamples` is declared as a `constexpr float` (the old fixed-sample-depth value, `16.0f`) but is no longer referenced by `SchroederTankCore` or `LegacyAccumulatorPath` after ADR-V1-13 — both now call `tankLfoDepthSamplesForRate(processingRate)`. The 24-02 summary states it was "retained as compile-time alias for documentation only," which is a reasonable transitional choice, but it is now a footgun: future code could grab the constant instead of the rate-scaled helper and silently reintroduce the bug ADR-V1-13 fixed.
**Fix (advisory):** Either (a) add a comment marking it deprecated/`[[deprecated("use tankLfoDepthSamplesForRate")]]`, or (b) delete it once no external consumers remain. Verify with `grep -rn kTankLfoDepthSamples source/` — if the only hits are the declaration and the `kTankLfoDepthSeconds`-derived initializer, deletion is safe.

### IN-03: DSP-09 contract is the tightest of the wet-dirt gates and depends on settle margin

**File:** `tests/V1ContractWetDirtTest.cpp:92-99`
**Issue:** The DSP-09 gate `lowRms < midRms * 0.25f` at 30 Hz vs 1 kHz sits at the edge of the 100 Hz one-pole HP's steady-state discrimination. The summary reports a measured ratio of 0.2491 at the 2 s settle — margin is ~0.4% below the 0.25 threshold. This is mathematically expected for a 100 Hz one-pole HP (theoretical |H(30Hz)|/|H(1kHz)| magnitude ratio is ~0.248 in steady state), so the gate is correctly calibrated *and* intentionally tight, but it leaves little headroom against platform `sinf`/`expf`/rounding variance. A different toolchain or `-ffast-math` could flip it.
**Note:** This is by design (the gate exists to *detect* HP regressions), so no change is required. Worth knowing if the suite flakes on a new CI image — extend `kSettle` (not loosen the ratio) per the §17.2 method-correction rule.

### IN-04: `LegacyAccumulatorPath` predelay uses `kInternalRate` not `hostRate_` for delay length

**File:** `source/LegacyAccumulatorPath.h:33-34`
**Issue:** `LegacyAccumulatorPath::prepare` sets the predelay delay as `kDarkPredelaySeconds * kInternalRate` (32768), not `kDarkPredelaySeconds * hostRate_`. This is intentional and correct for this path: the tank runs at the internal 32 kHz rate (`syncCombProcessingRate` pins combs to `kInternalRate`; `processTank` advances at `kInternalRate`), and the host↔internal resampling happens in `processAuthentic` via the `inputAccumulator` ratio, so the delay line is in internal samples. But because `scaleDelay` here returns the raw `delayAt32k` unscaled (line 105, unlike `SchroederTankCore`'s rate-scaled version), the predelay line is the one place that *must* be authored in internal samples. It is.
**Status:** Correct as written — noting it only because the divergence from `SchroederTankCore::prepare` (which scales by `processingRate_`) is non-obvious and a future refactor that "unifies" the two could introduce a wall-clock predelay regression on the legacy diagnostics path.

## Out of scope (verified untouched)

- Reverb character (comb delays, damping maps, `kTankLfoHz`, tank gain `0.85f`/`0.25f` comb normalization) — unchanged.
- `kProperSrcQuality` (r8brain transition band / attenuation / phase) — unchanged.
- `dirt_os` oversampling — confirmed absent from `GatedBloomChain.h` source text and UI toggle disabled (DSP-13 test asserts both).
- `[shipping-policy]` — correctly left red (Phase 25 scope).

## Methodology

- Static review of all 10 listed files plus supporting headers (`GatedBloomChain.h`, `DampedComb.h`, `SchroederAllpass.h`, `RateConverterPair.h`) and the milestone spec §17.2 / §18.
- Runtime verification: executed all five phase contract filters against `Builds/Tests` — all green (`[predelay]` 5/5 96007 assertions, `[mod-invariant]` 1/1, `[src-underfill]` 3/3, `[wet-dirt]` 7/7 4111 assertions, `[routing][GatedBloomChain]` 5/5).
- Allocation audit: `grep` for `new|malloc|alloc|push_back|resize|assign|emplace|make_unique|vector<` across the five DSP source files' realtime methods — only `prepare`-time `assign` in `FixedRateAdapter` (pre-sized scratch), none in any `process*` path.
- Git archaeology: `git log -S` / `git diff 6e40f94^..d9b0cbf` to attribute dead members to Phase 13 vs Phase 24.

---
_Reviewed: 2026-07-12T20:05:00Z_
_Reviewer: ZCode (standard depth)_
