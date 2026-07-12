# Phase 15: Three-Path Diagnostics - Research

**Researched:** 2026-07-08
**Domain:** C++ / JUCE DSP test harness — HF imaging diagnostics across host-rate, LegacyAccumulator, and ProperSRC reverb paths
**Confidence:** HIGH

## Summary

Phase 15 delivers an engineering-only diagnostics harness that proves ProperSRC architecturally fixes the accumulator HF imaging bug before Phase 18 user enablement. The codebase already contains ~80% of the required machinery: `Authentic32Mode` diagnostics API on `SchroederTank32` (Phase 14), Goertzel tail metrics in `HighFrequencyRingingDiagnosticsTest.cpp`, fixture generators, and SRC-06 imaging reduction proof in `FixedRateAdapterTest.cpp`. The gap is consolidation into an explicit **three-path** matrix (host / legacy / proper), adding the missing **RMS >14 kHz** metric (DIAG-03), multi-rate ProperSRC invariance gates (DIAG-04), and fixture×rate finiteness coverage (TEST-08).

Runtime verification (2026-07-08) shows SRC-06 passes and four of five HF regression tests pass; **TEST-11 currently fails** on `HF ringing authentic bright guitar 10k RMS near host-rate level` — measured ratio 1.456 vs threshold 1.4. SRC-05 LegacyAccumulator parity tests also fail post–Phase 14 because `SchroederTank32::processSample(..., authenticColor=true)` now routes to ProperSRC, not the legacy accumulator. Phase 15 must fix SRC-05 comparisons to use `setAuthentic32ModeForDiagnostics(LegacyAccumulator)` and resolve the TEST-11 threshold gap (DSP tuning or documented threshold adjustment).

**Primary recommendation:** Create `tests/HfDiagnosticsHelpers.h` + `tests/AuthenticPathDiagnosticsTest.cpp` with `SchroederTank32::processBlock` rendering at three paths, extract shared metrics from `HighFrequencyRingingDiagnosticsTest.cpp`, and gate ProperSRC with absolute HF ceilings plus legacy-fails/proper-passes differential assertions. Keep `HighFrequencyRingingDiagnosticsTest.cpp` as the full-chain TEST-11 regression suite via `GatedBloomChain`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
All implementation at Claude's discretion. Reuse HighFrequencyRingingDiagnosticsTest patterns and Phase 13 Authentic32Mode diagnostics API. Harness is engineering/test-only, not user-facing.

### Claude's Discretion
All implementation at Claude's discretion. Reuse HighFrequencyRingingDiagnosticsTest patterns and Phase 13 Authentic32Mode diagnostics API. Harness is engineering/test-only, not user-facing.

### Deferred Ideas (OUT OF SCOPE)
- Engine crossfade — Phase 16
- User enablement — Phase 18
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DIAG-01 | Three-path render harness compares host-rate, LegacyAccumulator, and ProperSRC paths | `SchroederTank32` + `setAuthentic32ModeForDiagnostics()`; render via `processBlock`; new `AuthenticPathDiagnosticsTest.cpp` with `ReverbPath` enum and side-by-side CSV matrix |
| DIAG-02 | Fixtures: impulse, guitar pluck, 220 Hz decay, 880 Hz decay, swept sine 200–8000 Hz | Already in `HighFrequencyRingingDiagnosticsTest.cpp` — extract to shared helpers |
| DIAG-03 | Metrics: RMS >10 kHz, RMS >14 kHz, dominant tail peak frequency, narrowband dominance, spectral centroid | `HfMetrics` struct exists; add `rmsAbove14k`; Goertzel via `ChainTestHelpers.h::goertzelPower` |
| DIAG-04 | ProperSRC host-rate invariance: 44.1/48/96 kHz renders have broadly similar HF metrics within documented tolerance | Multi-rate render loop on ProperSRC path only; ratio-to-48k-reference tolerances (see Multi-Rate Tolerance Strategy) |
| TEST-08 | Fixed32SRC produces finite output for all fixtures and host rates | Extend `FixedRateAdapterTest` pattern: 5 fixtures × 4 rates (44.1/48/88.2/96 kHz) via `SchroederTank32` ProperSRC `processBlock` |
| TEST-11 | Existing HF ringing regression tests pass with ProperSRC at 48 kHz | `HighFrequencyRingingDiagnosticsTest.cpp` `[hf][ringing][regression]` cases; **currently 1/4 failing** — threshold or DSP fix required |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Three-path reverb rendering | Test harness (`SchroederTank32`) | `GatedBloomChain` for TEST-11 only | Imaging is an SRC/reverb phenomenon; isolate from gates/OD for DIAG-01–04 |
| Diagnostics mode selection | `SchroederTank32` (production facade) | `FixedRateAdapter` (internal routing) | Phase 14 exposed `setAuthentic32ModeForDiagnostics()` on facade; tests must not reach through APVTS |
| HF spectral metrics | Test helpers (`HfDiagnosticsHelpers.h`) | — | Offline Goertzel scans; not runtime DSP |
| Multi-rate finiteness | Test harness | `FixedRateAdapter` | Validates SRC sandwich across host rates; reverb core stays at 32,768 Hz internally |
| Full-chain HF regression | `GatedBloomChain` test render | `PluginProcessor` (out of scope) | TEST-11 validates production wet path with gates/OD as deployed |
| CSV diagnostic output | Test stdout (`INFO` / `std::cout`) | — | Engineering-only; no UI or file I/O in plugin |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| Catch2 | 3.8.1 | Unit/integration test framework | Already in `cmake/Tests.cmake` via CPM [VERIFIED: `cmake/Tests.cmake`] |
| JUCE `juce_dsp` | 8.0.12 | DSP under test (`SchroederTank32`, `FixedRateAdapter`) | Project scaffold [VERIFIED: test run output] |
| r8brain-free-src | (bundled Phase 13) | ProperSRC resampling inside `FixedRateAdapter` | SRC-01; not invoked directly in tests |
| In-tree Goertzel | `ChainTestHelpers.h` | Tail-band power measurement | Already used by HF tests; no FFT dependency [VERIFIED: `tests/ChainTestHelpers.h`] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `ReverbTestHelpers.h` | in-tree | `maxAbsDiff`, RT60, impulse render | Path output comparison |
| `ParameterCurves.h` | in-tree | `sizeToRT60(0.5f)` for consistent render params | All harness renders |
| CTest | (CMake 4.3.3) | CI test discovery | `catch_discover_tests` in `cmake/Tests.cmake` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Goertzel tail scans | FFT (KissFFT, etc.) | FFT adds dependency and bin-width coupling; Goertzel gives exact Hz targeting for 14825 Hz imaging band |
| `SchroederTank32` reverb-only harness | Full `GatedBloomChain` for all DIAG tests | Full chain masks SRC defects with OD harmonics; keep chain only for TEST-11 |
| New metrics library | Hand-rolled RMS/centroid | Already implemented in HF test — extract, don't replace |

**Installation:** None — no new packages for this phase.

**Version verification:**
```bash
# Catch2 version pinned in cmake/Tests.cmake
grep Catch2 cmake/Tests.cmake   # gh:catchorg/Catch2@3.8.1
```

## Package Legitimacy Audit

> No new external packages are installed in Phase 15. Existing dependencies were verified in Phases 11–14.

| Package | Registry | Verdict | Disposition |
|---------|----------|---------|-------------|
| (none new) | — | — | N/A |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
                    ┌─────────────────────────────────────┐
                    │     AuthenticPathDiagnosticsTest     │
                    │  (fixtures × 3 paths × metrics)      │
                    └─────────────────┬───────────────────┘
                                      │
              ┌───────────────────────┼───────────────────────┐
              ▼                       ▼                       ▼
     ┌────────────────┐    ┌────────────────────┐   ┌─────────────────┐
     │ Path: HostRate │    │ Path: LegacyAccum  │   │ Path: ProperSRC │
     │ authColor=false│    │ authColor=true     │   │ authColor=true  │
     │ (no diag mode) │    │ diag=LegacyAccum   │   │ diag=cleared    │
     └───────┬────────┘    └─────────┬──────────┘   └────────┬────────┘
             │                       │                         │
             ▼                       ▼                         ▼
     ┌──────────────────────────────────────────────────────────────────┐
     │                    SchroederTank32::processBlock                  │
     │  hostEngine (host-rate core)  │  fixedRate_ (Authentic32Mode)    │
     └──────────────────────────────────────────────────────────────────┘
                                      │
                                      ▼
                    ┌─────────────────────────────────────┐
                    │   HfDiagnosticsHelpers::measureTail    │
                    │   Goertzel scans → HfMetrics struct    │
                    └─────────────────────────────────────┘
```

### Recommended Project Structure

```
tests/
├── HfDiagnosticsHelpers.h          # NEW — fixtures, HfMetrics, renderPath(), CSV
├── AuthenticPathDiagnosticsTest.cpp  # NEW — DIAG-01–04, TEST-08 matrix
├── HighFrequencyRingingDiagnosticsTest.cpp  # REFACTOR — import helpers; TEST-11
├── FixedRateAdapterTest.cpp        # FIX SRC-05 to use diagnostics mode
└── ChainTestHelpers.h              # unchanged — goertzelPower, rms
```

### Pattern 1: Three-Path Render Harness

**What:** Single `SchroederTank32` instance per path label; render identical input through host-rate, legacy accumulator, and ProperSRC; compare `HfMetrics` side by side.

**When to use:** All DIAG-01–04 assertions; CSV diagnostic matrix.

**Path configuration:**

| Path | `authenticColor` | Diagnostics API | Internal route |
|------|------------------|-----------------|--------------|
| HostRate | `false` | (ignored) | `HostRateReverbEngine` |
| LegacyAccumulator | `true` | `setAuthentic32ModeForDiagnostics(LegacyAccumulator)` | `FixedRateAdapter` → `LegacyAccumulatorPath` |
| ProperSRC | `true` | `clearAuthentic32ModeForDiagnostics()` | `FixedRateAdapter` → r8brain SRC + `SchroederTankCore` |

**Example:**
```cpp
// Source: source/SchroederTank32.h, tests/SchroederTank32BlockTest.cpp
enum class ReverbPath { HostRate, LegacyAccumulator, ProperSRC };

void applyPathDiagnostics (sendbloom::SchroederTank32& tank, ReverbPath path)
{
    switch (path)
    {
        case ReverbPath::LegacyAccumulator:
            tank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);
            break;
        case ReverbPath::ProperSRC:
            tank.clearAuthentic32ModeForDiagnostics();
            break;
        case ReverbPath::HostRate:
            tank.clearAuthentic32ModeForDiagnostics();
            break;
    }
}

std::vector<float> renderTankPath (sendbloom::SchroederTank32& tank,
                                   const std::vector<float>& input,
                                   ReverbPath path,
                                   double sampleRate,
                                   int blockSize)
{
    tank.prepare (sampleRate, blockSize);
    applyPathDiagnostics (tank, path);
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const bool authentic = path != ReverbPath::HostRate;

    std::vector<float> out (input.size(), 0.0f);
    std::vector<float> inBlock (static_cast<size_t> (blockSize), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (blockSize), 0.0f);

    for (size_t offset = 0; offset < input.size(); offset += static_cast<size_t> (blockSize))
    {
        const int n = static_cast<int> (std::min (static_cast<size_t> (blockSize), input.size() - offset));
        std::copy_n (input.begin() + static_cast<std::ptrdiff_t> (offset), n, inBlock.begin());
        tank.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f, authentic);
        std::copy_n (outBlock.begin(), n, out.begin() + static_cast<std::ptrdiff_t> (offset));
    }
    return out;
}
```

**Critical:** Use `processBlock`, not per-sample loops, for ProperSRC parity with production (`INTEG-01`) [VERIFIED: `source/SchroederTank32.h` lines 84–102].

### Pattern 2: HF Metric Suite (`HfMetrics`)

**What:** Tail-window spectral analysis on rendered wet signal.

**Constants (from existing tests):**

| Constant | Value | Source |
|----------|-------|--------|
| `kRenderSamples` | 24000 | HF test |
| `kTailStart` | `kRenderSamples - 4800` | Last 20% onset |
| `kTailCount` | 2400 | 50 ms @ 48 kHz |
| `kSampleRate` (default) | 48000 | HF test |
| `kBlockSize` | 512 | HF test |
| Bright render | `darkMix=0`, `distn=0` (reverb-only) | DIAG focus |

**Metrics (DIAG-03):**

| Metric | Implementation | Notes |
|--------|----------------|-------|
| RMS >10 kHz | `bandRmsAbove(scanFull, 10000.0)` | Exists as `rmsAbove10k` |
| RMS >14 kHz | `bandRmsAbove(scanFull, 14000.0)` | **Add** `rmsAbove14k` |
| Dominant tail peak | Max Goertzel bin 4–16 kHz scan, 25 Hz step | `peakFrequency` |
| Narrowband dominance | Peak power / mean neighbor power (75–200 Hz offset) | `narrowbandDominanceRatio()` |
| Spectral centroid | Σ(f·P)/Σ(P) over 4–16 kHz | `spectralCentroid` |

**Supplementary imaging probe:** Goertzel at 14825 Hz (14–15 kHz whistle band) — used by SRC-06 and TEST-11 [VERIFIED: `tests/FixedRateAdapterTest.cpp` SRC-06, `tests/HighFrequencyRingingDiagnosticsTest.cpp`].

### Pattern 3: Differential Path Gates

**What:** Legacy must fail imaging gates that ProperSRC passes — proves architectural fix, not just absolute thresholds.

| Gate | Legacy expectation | ProperSRC expectation | Source |
|------|-------------------|----------------------|--------|
| 14825 Hz tail RMS | > 0 (measurable imaging on guitar pluck) | < `0.0022` | SRC-06 ratio + HF test |
| 14825 Hz vs legacy | — | ≤ 30% of legacy power | SRC-06 [VERIFIED: passes] |
| Narrowband dominance | May exceed 10 on some fixtures | < `10.0` on guitar pluck | HF regression |
| RMS >10k vs host | N/A (baseline) | ratio < `1.4` (or tuned) | TEST-11 — **currently failing at 1.456** |

### Pattern 4: TEST-11 Full-Chain Regression

**What:** Keep `HighFrequencyRingingDiagnosticsTest.cpp` rendering through `GatedBloomChain::processSample` with `authenticColor=true` (production ProperSRC default).

**Why separate from DIAG harness:** Production wet path includes envelope, gates, pressure send, and wet OD. TEST-11 validates end-user signal chain, not isolated reverb.

**Refactor:** Import fixtures/metrics from `HfDiagnosticsHelpers.h`; delete duplicated `makeGuitarPluck`, `measureTail`, etc.

**GatedBloomChain diagnostics gap:** Chain does not expose `setAuthentic32ModeForDiagnostics`. For three-path **chain** tests (optional), add test-only helper:

```cpp
// Option A — cast after prepare (chain defaults to SchroederTank32)
auto* tank = dynamic_cast<sendbloom::SchroederTank32*> (chain.getReverbEngineForTests());
```

If `getReverbEngineForTests()` does not exist, add `GatedBloomChain::getReverbForTests()` returning raw `IReverbEngine*` — minimal surface, not APVTS [ASSUMED: planner discretion per CONTEXT].

### Anti-Patterns to Avoid

- **Comparing LegacyAccumulatorPath to `processSample(..., true)` without diagnostics mode:** Post–Phase 14, `authenticColor=true` defaults to ProperSRC; SRC-05 tests currently fail for this reason [VERIFIED: test run 2026-07-08].
- **Per-sample rendering for ProperSRC tests:** Under-tests block SRC boundary behavior; use `processBlock`.
- **Impulse-only for legacy imaging comparison:** Anti-image SVF masks 14825 Hz on impulse; use guitar pluck (documented in SRC-06 comment) [VERIFIED: `FixedRateAdapterTest.cpp` line 508].
- **Identical HF metrics across host rates:** SRC group delay and foldback frequency shift with rate; use ratio tolerances, not bit-exact match (DIAG-04).
- **Exposing diagnostics via APVTS:** Phase 14 integrability test explicitly forbids this [VERIFIED: `Phase14IntegrabilityTest.cpp`].

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tail-band power at specific Hz | Custom DFT/FFT wrapper | `sendbloom::test::goertzelPower` | O(n) per target frequency; exact Hz for 14825 imaging probe |
| RMS from power sum | Ad-hoc scaling | `rmsFromPower(power, count)` pattern in HF test | Consistent √(2P/N) convention across tests |
| Legacy accumulator DSP | Re-implement hold/accumulator | `LegacyAccumulatorPath` via diagnostics mode | SRC-05 parity already validated path exists |
| Multi-rate SRC | Test-only resampler | `SchroederTank32` + `FixedRateAdapter` production path | Tests must exercise real code under test |
| CSV reporting framework | External CSV lib | `std::cout` header + row per config | Engineering diagnostics only; zero deps |

**Key insight:** The acceptance contract is already encoded in test constants (`1.4` ratio, `0.0022` imaging RMS, `10.0` dominance, `0.30` SRC-06 reduction). Phase 15 organizes them into a three-path matrix — not reinvent metrics.

## Multi-Rate Tolerance Strategy (DIAG-04)

### Scope

- **Rates under test:** 44,100 / 48,000 / 96,000 Hz per DIAG-04 requirement [VERIFIED: `.planning/REQUIREMENTS.md`]
- **Path:** ProperSRC only (`clearAuthentic32ModeForDiagnostics()`, `authenticColor=true`)
- **Primary fixture:** `guitar_pluck` (excites 14–15 kHz imaging; same as SRC-06)
- **Secondary fixture:** `impulse` (finiteness sanity; weaker imaging discriminator)
- **Reference rate:** 48,000 Hz — canonical project sample rate

### Tolerance Philosophy

ProperSRC bandlimits the 32,768 Hz core to the host Nyquist rate. Residual HF differences across host rates come from: (1) r8brain filter group delay / passband ripple, (2) imaging foldback position shifting with host rate, (3) Goertzel bin quantization at different tail sample counts. Expect **broad similarity**, not identity.

### Proposed Metric Tolerances (vs 48 kHz reference)

Document as named `constexpr` in `AuthenticPathDiagnosticsTest.cpp` for planner traceability:

| Metric | Tolerance | Rationale |
|--------|-----------|-----------|
| `rms_above_10k` | Ratio to 48k ∈ [0.65, 1.55] | ±55% band; authentic HF may vary with SRC ratio; looser than TEST-11 host comparison |
| `rms_above_14k` | Ratio to 48k ∈ [0.65, 1.55] | Same; 14k+ is imaging-sensitive band |
| `imaging_14825_rms` | Absolute < `0.0022` at **each** rate | Existing HF ceiling; rate-invariant whistle gate |
| `peak_frequency_hz` | \|Δ\| ≤ 1500 Hz vs 48k | Foldback peak shifts; Goertzel 25 Hz resolution |
| `spectral_centroid` | \|Δ\| ≤ 3000 Hz vs 48k | Centroid sensitive to broadband tail shape |
| `narrowband_dominance` | < `10.0` at **each** rate | Absolute whistle gate (not ratio) |
| `rms_total` (tail) | Ratio to 48k ∈ [0.5, 2.0] | RT60/tail level may differ slightly with latency; wide guard |

### Cross-Rate Consistency Check (optional hard gate)

Max/min ratio across {44.1, 48, 96} kHz for `rms_above_10k` ≤ **1.6** on guitar pluck — catches rate-dependent imaging blowups without over-constraining.

### Rates Outside DIAG-04

TEST-08 requires **88,200 Hz** additionally (SRC-03 four-rate set). Include 88.2 in finiteness loops but omit from DIAG-04 invariance assertions unless requirements expand.

### Calibration Workflow

1. Run CSV matrix at 44.1/48/96 kHz ProperSRC; capture baseline metrics.
2. If tolerances fail on current ProperSRC build, widen only with documented DSP justification — prefer fixing SRC over loosening `imaging_14825_rms` or `narrowband_dominance` ceilings.
3. Record final constants in test file comments linking to DIAG-04.

## Common Pitfalls

### Pitfall 1: Legacy Path Compared to Wrong Production Output

**What goes wrong:** SRC-05 and any legacy A/B test compare `LegacyAccumulatorPath` to `SchroederTank32(..., authenticColor=true)` which now returns ProperSRC.

**Why it happens:** Phase 14 switched production authentic path to ProperSRC; tests not updated.

**How to avoid:** Always call `setAuthentic32ModeForDiagnostics(LegacyAccumulator)` before rendering legacy path.

**Warning signs:** SRC-05 failures; legacy and proper metrics identical in CSV.

### Pitfall 2: TEST-11 Threshold Too Tight for ProperSRC Coloration

**What goes wrong:** Authentic bright path legitimately has more >10 kHz energy than host-rate (different damping/calibration); ratio 1.456 vs 1.4 fails.

**Why it happens:** Threshold written for accumulator-era behavior; ProperSRC preserves more authentic HF without whistle.

**How to avoid:** Re-evaluate `kAuthenticVsHostRmsAbove10kMaxRatio` against measured ProperSRC distribution; consider 1.5f with comment, or split gate: reverb-only ratio vs full-chain.

**Warning signs:** Only 10k RMS ratio fails; imaging band and dominance pass.

### Pitfall 3: Impulse Fixture Masks Imaging

**What goes wrong:** Legacy vs proper looks similar on impulse; false confidence.

**Why it happens:** Anti-image SVF attenuates 14–15 kHz on impulse tails.

**How to avoid:** Primary gates on `guitar_pluck`; impulse for finiteness only.

**Warning signs:** SRC-06 comment already documents this [VERIFIED: `FixedRateAdapterTest.cpp`].

### Pitfall 4: Per-Sample vs Block Render Mismatch

**What goes wrong:** Diagnostics pass in sample loop but fail in production block path.

**Why it happens:** r8brain expects multi-sample blocks; per-sample `processBlock(..., 1)` works but differs from 512-block stress.

**How to avoid:** Default block size 512; add one variable-block spot check (sizes 1, 64, 512) in TEST-08.

**Warning signs:** `SchroederTank32BlockTest` passes but chain block fails.

### Pitfall 5: Duplicated Metric Code Drifts

**What goes wrong:** HF test and new diagnostics test use different tail windows or RMS formulas.

**How to avoid:** Single `HfDiagnosticsHelpers.h` source of truth.

**Warning signs:** Same config produces different CSV values across test files.

## Code Examples

### Goertzel Imaging Probe (14825 Hz)

```cpp
// Source: tests/ChainTestHelpers.h, tests/HighFrequencyRingingDiagnosticsTest.cpp
const auto imagingPower = sendbloom::test::goertzelPower (wet, sampleRate, 14825.0,
                                                          kTailStart, kTailCount);
const auto imagingRms = std::sqrt (2.0 * imagingPower / static_cast<double> (kTailCount));
REQUIRE (imagingRms < 0.0022f);
```

### SRC-06 Legacy vs Proper Reduction

```cpp
// Source: tests/FixedRateAdapterTest.cpp [SRC-06]
REQUIRE (properPower <= legacyPower * 0.30);  // ≥70% reduction
```

### Diagnostics Mode on SchroederTank32

```cpp
// Source: source/SchroederTank32.h, tests/SchroederTank32BlockTest.cpp
legacyTank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);
properTank.clearAuthentic32ModeForDiagnostics();  // defaults to ProperSRC
```

### Multi-Rate Finiteness Loop (TEST-08)

```cpp
// Source: tests/FixedRateAdapterTest.cpp [ProperSRC][multiRate] pattern, extended
constexpr std::array<double, 4> kHostRates { 44100.0, 48000.0, 88200.0, 96000.0 };
for (const auto rate : kHostRates)
    for (const auto& [name, input] : fixtures)
    {
        auto wet = renderTankPath (tank, input, ReverbPath::ProperSRC, rate, 512);
        for (float s : wet) REQUIRE (std::isfinite (s));
    }
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `processAuthentic` accumulator in `SchroederTank32` | `FixedRateAdapter` with `Authentic32Mode` | Phase 13–14 | Legacy behind diagnostics enum only |
| Per-sample authentic render | `processBlock` + r8brain SRC | Phase 14 | TEST-08/11 must use block API |
| HF tests as ad-hoc CSV | Three-path `AuthenticPathDiagnosticsTest` | Phase 15 (planned) | DIAG-01 formalized |
| SRC-05 tank parity | Broken post–Phase 14 | 2026-07-08 observed | Fix in Phase 15 |

**Deprecated/outdated:**
- `SchroederTank32::processAuthentic` (private, dead in production path) — legacy logic lives in `LegacyAccumulatorPath` [VERIFIED: grep shows only `LegacyAccumulatorPath.h` calls `processAuthentic`]

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `kAuthenticVsHostRmsAbove10kMaxRatio` may increase to 1.5f for TEST-11 pass | Multi-Rate / Pitfall 2 | Too loose → audible HF excess ships |
| A2 | DIAG-04 ±55% RMS ratio tolerances sufficient for ProperSRC | Multi-Rate Tolerance | Too tight → flaky CI; too loose → rate bugs slip through |
| A3 | `GatedBloomChain` test accessor needed only if chain-level three-path tests desired | Pattern 4 | Reverb-only harness sufficient for DIAG-01–04 |
| A4 | 88.2 kHz omitted from DIAG-04 invariance (finiteness only) | Multi-Rate | Requirement ambiguity — REQ says 44.1/48/96 only |

## Open Questions

1. **TEST-11 threshold: tune DSP or relax ratio?**
   - What we know: Ratio 1.456 vs 1.4; imaging band and dominance pass.
   - What's unclear: Whether additional HF is musically intended authentic color vs defect.
   - Recommendation: Measure reverb-only ratio first; if <1.4 at tank level, OD/gate adds HF in chain — split thresholds.

2. **SRC-05 repair scope**
   - What we know: Tests fail comparing legacy path to default authentic output.
   - Recommendation: Update SRC-05 to use `setAuthentic32ModeForDiagnostics(LegacyAccumulator)` on tank; include in Phase 15 Wave 0.

3. **CSV artifact persistence**
   - What we know: Current HF test prints CSV to stdout only.
   - Recommendation: Keep stdout; optional CI artifact upload is out of scope per CONTEXT.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build tests | ✓ | 4.3.3 | — |
| Catch2 | Test framework | ✓ | 3.8.1 (CPM) | — |
| Node.js | gsd-tools (research) | ✓ | v22.22.3 | — |
| Built `Tests` binary | Run diagnostics | ✓ | `build/Tests` | `cmake --build build` |
| CTest | CI discovery | ✓ | `/opt/homebrew/bin/ctest` | `./Tests "[tag]"` |

**Missing dependencies with no fallback:** none

**Missing dependencies with fallback:** none

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (CPM FetchContent) |
| Quick run command | `cd build && ./Tests "[diagnostics]" -r compact` |
| Full suite command | `cd build && ctest --output-on-failure` |
| HF regression only | `cd build && ./Tests "[hf][ringing][regression]" -r compact` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| DIAG-01 | Three-path side-by-side render + CSV | integration | `./Tests "[DIAG-01]" -r compact` | ❌ Wave 0 |
| DIAG-02 | Five fixtures in matrix | integration | `./Tests "[DIAG-02]" -r compact` | ❌ Wave 0 (fixtures exist in HF test) |
| DIAG-03 | RMS>10k/14k, peak, dominance, centroid | unit/integration | `./Tests "[DIAG-03]" -r compact` | ❌ Wave 0 (partial in HF test) |
| DIAG-04 | ProperSRC 44.1/48/96 kHz invariance | integration | `./Tests "[DIAG-04]" -r compact` | ❌ Wave 0 |
| TEST-08 | ProperSRC finite all fixtures × rates | integration | `./Tests "[TEST-08]" -r compact` | ❌ Wave 0 (partial: impulse×4 rates in FixedRateAdapterTest) |
| TEST-11 | HF ringing regressions at 48 kHz | regression | `./Tests "[hf][ringing][regression]" -r compact` | ✅ (1 assertion failing) |

### Sampling Rate

- **Per task commit:** `./Tests "[diagnostics]" -r compact`
- **Per wave merge:** `./Tests "[hf]" -r compact`
- **Phase gate:** `ctest --output-on-failure` green before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `tests/HfDiagnosticsHelpers.h` — shared fixtures, `HfMetrics`, `renderTankPath`, `rmsAbove14k`
- [ ] `tests/AuthenticPathDiagnosticsTest.cpp` — DIAG-01–04, TEST-08 tagged cases
- [ ] Refactor `tests/HighFrequencyRingingDiagnosticsTest.cpp` — import helpers; fix TEST-11 threshold
- [ ] Fix `tests/FixedRateAdapterTest.cpp` SRC-05 — diagnostics mode for legacy tank comparison
- [ ] Optional: `GatedBloomChain::getReverbForTests()` if chain-level three-path needed

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | no (test-only offline renders) | — |
| V6 Cryptography | no | — |

### Known Threat Patterns for Test Harness

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Diagnostics API leak to APVTS/UI | Information disclosure | Phase 14 integrability test forbids exposure; keep `setAuthentic32ModeForDiagnostics` test-only |
| Arbitrary file write from CSV | Tampering | Stdout only; no file I/O in tests unless explicitly scoped |

## Sources

### Primary (HIGH confidence)
- `source/SchroederTank32.h` — diagnostics API, `processBlock` routing
- `source/FixedRateAdapter.h` — `Authentic32Mode` switch
- `source/Authentic32Mode.h` — Off / LegacyAccumulator / ProperSRC enum
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — fixtures, metrics, thresholds
- `tests/FixedRateAdapterTest.cpp` — SRC-06, multi-rate finiteness patterns
- `tests/ChainTestHelpers.h` — Goertzel implementation
- `tests/SchroederTank32BlockTest.cpp` — diagnostics mode block render
- `.planning/REQUIREMENTS.md` — DIAG-01–04, TEST-08, TEST-11 definitions
- Test run 2026-07-08 — `[hf]` 5/6 pass; `[SRC-06]` pass; `[SRC-05]` fail

### Secondary (MEDIUM confidence)
- `.planning/research/ARCHITECTURE.md` — `AuthenticPathDiagnosticsTest.cpp` placement
- `.planning/research/PITFALLS.md` — three-path CSV gate pattern
- `.planning/phases/14-block-level-integration/14-RESEARCH.md` — diagnostics hook design

### Tertiary (LOW confidence)
- Multi-rate tolerance numeric bands — proposed pending baseline CSV capture [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — entirely in-tree; verified by file reads and test execution
- Architecture: HIGH — Phase 14 API exists; patterns proven in sibling tests
- Pitfalls: HIGH — SRC-05 and TEST-11 failures reproduced locally
- DIAG-04 tolerances: MEDIUM — proposed constants need baseline calibration run

**Research date:** 2026-07-08
**Valid until:** 2026-08-08 (test harness stable); re-run if `SchroederTank32` or HF thresholds change
