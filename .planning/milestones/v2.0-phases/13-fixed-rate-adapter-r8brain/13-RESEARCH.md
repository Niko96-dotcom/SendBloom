# Phase 13: FixedRateAdapter + r8brain - Research

**Researched:** 2026-07-08
**Domain:** Realtime bandlimited hostRate ↔ 32,768 Hz SRC wrapping `SchroederTankCore` in a JUCE 8 mono plugin
**Confidence:** HIGH

## Summary

Phase 13 delivers the bandlimited bridge that replaces the broken `processAuthentic()` accumulator/hold path. The v1 authentic path in `SchroederTank32.h` steps the tank when `inputAccumulator >= 1.0` at ratio `32768/hostRate`, holds `lastInternalOut`, linearly interpolates, then applies a 12 kHz anti-image SVF — this is **not** bandlimited SRC and produces stable 14–15 kHz imaging (measured at 14825 Hz in `HighFrequencyRingingDiagnosticsTest.cpp`) [VERIFIED: `source/SchroederTank32.h` lines 178–194; `tests/HighFrequencyRingingDiagnosticsTest.cpp` lines 27–28, 356–364].

Phase 12 extracted `SchroederTankCore` as a single-rate tank with `scaleDelay(processingRate/kInternalRate)` and no authentic/host branches [VERIFIED: `source/SchroederTankCore.h`]. Phase 13 wraps that core at **32768 Hz** inside `FixedRateAdapter` using r8brain `CDSPResampler` upsample (host→32768) and downsample (32768→host) in a block `processBlock()` API. An internal `Authentic32Mode` enum (`Off` / `LegacyAccumulator` / `ProperSRC`) selects the diagnostic path without exposing new UI — RC1 remains `Off` by default [CITED: `.planning/phases/13-fixed-rate-adapter-r8brain/13-CONTEXT.md`].

r8brain-free-src 7.1 is header-only for C++11+, MIT-licensed, handles arbitrary ratios (44.1/48/88.2/96 kHz ↔ 32768), and exposes `getLatency()` / `getMaxOutLen()` for buffer sizing [CITED: https://github.com/avaneev/r8brain-free-src/blob/master/README.md; https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html]. Integration uses existing CPM 0.42.0 with a thin `INTERFACE` CMake target — no changes to JUCE linkage [VERIFIED: `cmake/CPM.cmake`, `cmake/Tests.cmake`]. **GatedBloomChain wiring and `IReverbEngine::processBlock()` extension are Phase 14** — Phase 13 proves the adapter in isolation with unit tests including SRC-06's 70% imaging reduction gate on impulse at 48 kHz.

**Primary recommendation:** Add `cmake/R8brain.cmake` (CPM pin `e71c31bf`), implement `RateConverterPair` + `FixedRateAdapter` with prepare-time `CDSPResampler` construction and preallocated `double` scratch buffers, port `processAuthentic` logic into `LegacyAccumulator` for A/B only, and gate ProperSRC with Goertzel power at 14825 Hz versus legacy on impulse.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

All implementation choices at Claude's discretion. Use ROADMAP SRC-01–SRC-06, TEST-10 requirements, Phase 12 SchroederTankCore/HostRateReverbEngine as upstream, and PROJECT.md r8brain-first SRC decision.

**Inherited constraints:**
- r8brain-first prototype (MIT license, proven latency API)
- Zero heap allocation in processBlock after prepare
- Mono processing
- RC1 host-rate path unchanged (authentic_color off default)

### Claude's Discretion

All implementation choices at Claude's discretion. Use ROADMAP SRC-01–SRC-06, TEST-10 requirements, Phase 12 SchroederTankCore/HostRateReverbEngine as upstream, and PROJECT.md r8brain-first SRC decision.

### Deferred Ideas (OUT OF SCOPE)

- Block-level GatedBloomChain integration — Phase 14
- Three-path diagnostics UI — Phase 15
- Engine crossfade — Phase 16
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SRC-01 | `FixedRateAdapter` bandlimited hostRate ↔ 32,768 Hz via r8brain-free-src (MIT) | `RateConverterPair` with `CDSPResampler` up/down; `FixedRateAdapter` SRC sandwich around `SchroederTankCore@32768` |
| SRC-02 | r8brain constructed and buffers preallocated in `prepare()` only — zero heap in `processBlock()` | Constructor `aMaxInLen` drives internal allocation; adapter owns `std::vector<double>` sized via `getMaxOutLen()` at prepare; no `new`/`resize` in hot path |
| SRC-03 | Supports host rates 44.1, 48, 88.2, 96 kHz; variable blocks 1..maxBlock | `prepare(hostRate, maxBlockSize)` rebuilds resamplers; per-rate impulse smoke tests; block-size matrix from `RealtimeStressTest` pattern |
| SRC-04 | Internal `Authentic32Mode`: Off / LegacyAccumulator / ProperSRC — diagnostics only | Enum on adapter or facade; default `Off`; not wired to APVTS |
| SRC-05 | Legacy accumulator retained for A/B regression only | Port `processAuthentic()` from `SchroederTank32.h` into `LegacyAccumulator` helper or mode branch |
| SRC-06 | ProperSRC reduces 14–15 kHz imaging ≥70% vs LegacyAccumulator on impulse at 48 kHz | Goertzel at 14825 Hz (existing test frequency); require `properRms <= legacyRms * 0.30` on tail slice |
| TEST-10 | Reset clears all delay and resampler state | `reset()` calls `CDSPResampler::clear()`, core delay resets, accumulator/LIFO cleared; impulse-after-reset ≈ fresh |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Host ↔ 32,768 Hz bandlimited conversion | DSP adapter (`FixedRateAdapter` / `RateConverterPair`) | — | SRC is wet-path DSP; not UI or host PDC policy (Phase 17) |
| Fixed-rate Schroeder tank math | DSP core (`SchroederTankCore` @ 32768) | — | Tank runs at internal rate inside adapter; delays unscaled at 32768 |
| Legacy accumulator reference path | DSP adapter (diagnostics branch) | `SchroederTank32` (current inline) | A/B only; mirrors existing `processAuthentic` |
| HF imaging acceptance metric | Offline test harness (Catch2) | — | Goertzel power in test code, not runtime |
| CMake dependency vendoring | Build system (`cmake/R8brain.cmake`) | CPM cache | Fetch header-only lib; no runtime tier |
| Plugin block integration | — (Phase 14) | `GatedBloomChain` / `IReverbEngine` | Out of scope; adapter exposes `processBlock` locally |
| PDC / reported latency | — (Phase 17) | `PluginProcessor` | Measure `getLatency()` but do not change RC1 zero-latency default |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| r8brain-free-src | 7.1 (pin `e71c31bf320f84210bb4bdcb57e296c39ce940f9`) | Arbitrary-ratio bandlimited SRC | PROJECT.md ADR-track choice; MIT; handles 32768↔host non-integer ratios; `getLatency()` API [CITED: r8brain README] |
| SchroederTankCore | (in-tree Phase 12) | Fixed-rate tank @ 32768 Hz | Unscaled delay table; no double-scaling [VERIFIED: `source/SchroederTankCore.h`] |
| JUCE juce_dsp | 8 (bundled) | Delay/comb atoms in core | Existing dependency; unchanged |
| CPM | 0.42.0 | Fetch r8brain sources | Matches repo `cmake/CPM.cmake` [VERIFIED: codebase] |
| Catch2 | 3.8.1 | Adapter + SRC regression tests | Existing `cmake/Tests.cmake` [VERIFIED: codebase] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `fft/pffft_double.c` (r8brain bundled) | 7.1 | ~15–25% SRC CPU reduction | After HF gates pass; define `R8B_PFFFT_DOUBLE=1` [CITED: r8brain README] |
| `ChainTestHelpers.h` / `goertzelPower` | (in-tree) | HF imaging metrics | SRC-06 and future Phase 15 diagnostics |
| `ReverbTestHelpers.h` | (in-tree) | RT60 / impulse render | Optional adapter tail sanity |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| r8brain | libsamplerate 0.2.2 | BSD but external link + separate latency measurement; test-only fallback per STACK.md |
| r8brain | JUCE `dsp::Oversampling` | Integer ratios only — cannot do 48k↔32768 [CITED: `.planning/research/STACK.md`] |
| Block `processBlock()` adapter | Per-sample SRC shim | r8brain async pull model breaks per-sample; CPU blowup [CITED: PITFALLS.md Pitfall 4] |
| ProperSRC | Anti-image SVF alone | Masks 14–15 kHz whistle; does not fix SRC [VERIFIED: `kAuthenticAntiImageLpHz` in delay table] |

**Installation (CMake):**

```cmake
# cmake/R8brain.cmake
include(CPM)

CPMAddPackage(
    NAME r8brain-free-src
    GITHUB_REPOSITORY avaneev/r8brain-free-src
    GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
    DOWNLOAD_ONLY YES
)

add_library(r8brain INTERFACE)
target_include_directories(r8brain INTERFACE "${r8brain-free-src_SOURCE_DIR}")

# Root CMakeLists.txt after SharedCode target:
# include(R8brain)
# target_link_libraries(SharedCode INTERFACE r8brain)
```

**Version verification:** r8brain is not an npm package; legitimacy confirmed via official GitHub repository `avaneev/r8brain-free-src` and Voxengo documentation [CITED: URLs above]. Pin commit `e71c31bf` per `.planning/research/STACK.md` (v7.1 changelog entry).

## Package Legitimacy Audit

> C++ header-only dependency — npm registry not applicable. Verified against official GitHub + vendor docs.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| r8brain-free-src | GitHub (not npm) | 10+ yrs project | REAPER/AUDIRVANA users | github.com/avaneev/r8brain-free-src | OK | Approved — pin `e71c31bf` |

**Packages removed due to [SLOP] verdict:** none (npm `r8brain-free-src` lookup is a false positive — library is C++ header-only, not published to npm)

**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
                    prepare(hostRate, maxBlock) — alloc resamplers + scratch only
                                    │
host float block in[n] ─────────────┼──────────────────────────────────────────┐
                                    ▼                                          │
                         ┌─────────────────────┐                                 │
                         │  RateConverterPair  │                                 │
                         │  CDSPResampler UP   │  hostRate → 32768 Hz           │
                         └─────────┬───────────┘                                 │
                                   │ double[] @ ~n * 32768/hostRate              │
                                   ▼                                             │
                         ┌─────────────────────┐                                 │
                         │ SchroederTankCore   │  processSample × nInternal       │
                         │ @ 32768 Hz          │  (or future processBlock)        │
                         └─────────┬───────────┘                                 │
                                   │                                             │
                                   ▼                                             │
                         ┌─────────────────────┐                                 │
                         │  RateConverterPair  │                                 │
                         │  CDSPResampler DOWN │  32768 Hz → hostRate            │
                         └─────────┬───────────┘                                 │
                                   │ consume leftover FIFO before next block    │
                                   ▼                                             │
host float block out[n] ◄──────────┘                                            │
                                                                                 │
Authentic32Mode branch (diagnostics only):                                       │
  Off ──────────────► bypass / not used                                        │
  LegacyAccumulator ► accumulator+hold+SVF (no r8brain) ────────────────────────┘
  ProperSRC ────────► path above
```

### Recommended Project Structure

```
source/
├── Authentic32Mode.h          # enum Off / LegacyAccumulator / ProperSRC
├── RateConverterPair.h        # r8brain up/down, latency, FIFO leftovers
├── FixedRateAdapter.h         # mode switch, core@32768, processBlock/reset
├── SchroederTankCore.h        # existing — prepare(32768.0, maxBlock)
├── SchroederTank32.h          # unchanged in Phase 13 (facade wiring Phase 14+)
cmake/
├── R8brain.cmake              # CPM fetch + INTERFACE target
tests/
├── FixedRateAdapterTest.cpp   # SRC-06 imaging, TEST-10 reset, multi-rate smoke
```

### Pattern 1: RateConverterPair (r8brain lifecycle)

**What:** Owns two `r8b::CDSPResampler` instances and preallocated `double` buffers; exposes upsample/downsample block helpers and round-trip latency.

**When to use:** Any mono host↔fixed-rate bridge; keeps r8brain details out of `FixedRateAdapter`.

**Example:**

```cpp
// Source: https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html
#include "CDSPResampler.h"

class RateConverterPair
{
public:
    void prepare (double hostRate, int maxHostBlock) noexcept
    {
        upsampler = std::make_unique<r8b::CDSPResampler> (hostRate, 32768.0, maxHostBlock);
        const auto maxInternalIn = upsampler->getMaxOutLen (maxHostBlock);
        downsampler = std::make_unique<r8b::CDSPResampler> (32768.0, hostRate, maxInternalIn);
        upOut.resize (static_cast<size_t> (upsampler->getMaxOutLen (maxHostBlock)));
        downOut.resize (static_cast<size_t> (downsampler->getMaxOutLen (maxInternalIn)));
        hostToInternalRatio_ = 32768.0 / hostRate;
    }

    int upsample (const float* hostIn, int nHost, double* internalOut) noexcept
    {
        // convert float→double into scratchIn; pull loop until nInternal produced
        double* op = nullptr;
        return upsampler->process (scratchIn.data(), nHost, op); // copy op → internalOut
    }

    int getRoundTripLatencySamples() const noexcept
    {
        return upsampler->getLatency() + downsampler->getLatency();
    }

    void reset() noexcept
    {
        upsampler->clear();
        downsampler->clear();
        leftoverDown_ = 0;
    }

private:
    std::unique_ptr<r8b::CDSPResampler> upsampler, downsampler;
    std::vector<double> scratchIn, upOut, downOut;
    int leftoverDown_ { 0 };
    double hostToInternalRatio_ { 32768.0 / 48000.0 };
};
```

**Buffer sizing notes [CITED: r8brain CDSPResampler docs]:**
- `aMaxInLen` on upsampler = `maxHostBlock` from `prepare()`.
- `aMaxInLen` on downsampler = `upsampler->getMaxOutLen(maxHostBlock)` (worst-case internal block).
- At 96 kHz host, internal block is **smaller** than host block (32768/96000 ≈ 0.34×); at 44.1 kHz, internal block is **larger** (~0.74×). Size using `getMaxOutLen()`, not manual ratio math alone.
- Maintain **downsampler leftover FIFO** between `processBlock()` calls — r8brain may return more/fewer samples than host block length.

### Pattern 2: FixedRateAdapter SRC sandwich

**What:** Selects `Authentic32Mode`; for `ProperSRC`, runs up → `SchroederTankCore` loop → down; applies authentic coefficient quantization when emulating 32k Color brightness.

**When to use:** Phase 13 standalone tests and future `SchroederTank32` delegation (Phase 14).

**Example:**

```cpp
enum class Authentic32Mode { Off, LegacyAccumulator, ProperSRC };

class FixedRateAdapter
{
public:
    void prepare (double hostRate, int maxBlockSize) noexcept
    {
        hostRate_ = hostRate;
        core.prepare (SchroederTank32DelayTable::kInternalRate, maxBlockSize);
        converters.prepare (hostRate, maxBlockSize);
        hostScratch.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        legacy_.prepare (hostRate, maxBlockSize);
    }

    void processBlock (const float* in, float* out, int n,
                       float rt60, float darkMix, Authentic32Mode mode) noexcept
    {
        if (mode != Authentic32Mode::ProperSRC)
        {
            legacy_.processBlock (in, out, n, rt60, darkMix, mode);
            return;
        }

        // upsample → core (per-sample coeffs for parity) → downsample
        // float↔double conversion on preallocated buffers only
    }

    void reset() noexcept
    {
        converters.reset();
        coreReset(); // add SchroederTankCore::reset or re-call prepare
        legacy_.reset();
    }

private:
    SchroederTankCore core;
    RateConverterPair converters;
    LegacyAccumulatorPath legacy_;
};
```

**Authentic coefficient path:** When testing ProperSRC against legacy authentic color, use `kAuthenticBrightDampingHz` and 9-bit quantization from `SchroederTank32::updateCoeffs` authentic branch — host-path coeffs differ [VERIFIED: `source/SchroederTank32.h` lines 119–142 vs `SchroederTankCore.h` lines 77–94]. Phase 13 SRC-06 uses **impulse + bright (darkMix=0)**; quantization matters less but mode parity should be documented for Phase 15.

### Pattern 3: Authentic32Mode enum

**What:** Internal diagnostics selector — not exposed in APVTS or UI.

| Mode | Behavior | Production use |
|------|----------|----------------|
| `Off` | Adapter inactive; host-rate path only | RC1 default |
| `LegacyAccumulator` | `processAuthentic` math (accumulator + hold + SVF) | A/B regression; must fail SRC-06 |
| `ProperSRC` | r8brain sandwich + core@32768 | Target path; must pass SRC-06 |

**Placement:** `source/Authentic32Mode.h` as `enum class Authentic32Mode : uint8_t`. Default construction to `Off`. Test-only setter or constructor param — no `ParameterIDs` entry [CITED: 13-CONTEXT.md].

### Anti-Patterns to Avoid

- **Constructing `CDSPResampler` in `processBlock()`:** Constructor allocates from `aMaxInLen` [CITED: r8brain docs] — violates SRC-02 and CHN-05 realtime rule.
- **Ignoring downsampler leftovers:** Causes dropouts/repeats at block boundaries [CITED: `.planning/research/PITFALLS.md` Pitfall 6].
- **Using `float` r8brain `process()` without conversion:** API is `double*` only [CITED: CDSPResampler reference].
- **Wiring ProperSRC to `authentic_color` APVTS in Phase 13:** Deferred to Phase 14+; RC1 must stay off-by-default.
- **Patching accumulator instead of A/B retaining it:** SRC-05 requires legacy path for comparison, not production fix.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Arbitrary-ratio bandlimited SRC | Accumulator + linear hold | r8brain `CDSPResampler` | Proven anti-imaging; v1 path fails at 14825 Hz |
| Fractional delay interpolation at one rate | JUCE `LagrangeInterpolator` as SRC | r8brain | Interpolators ≠ full-ratio SRC with stopband control |
| Integer 2^N resampling only | `juce::dsp::Oversampling` | r8brain | 48000/32768 is not power-of-two |
| FFT-based SRC from scratch | Custom windowed sinc | r8brain MIT | Weeks of filter/latency/SIMD work |
| HF imaging metric | FFT bin peak hunt ad hoc | `goertzelPower` @ 14825 Hz | Existing acceptance contract in HF tests |

**Key insight:** The bug is the **bridge**, not the tank. `SchroederTankCore` at 32768 Hz is already validated (Phase 12 RT60 tests); only the hostRate↔32768 conversion needs a library-grade solution.

## Common Pitfalls

### Pitfall 1: Accumulator mistaken for SRC

**What goes wrong:** Stable 14–15 kHz whistle on authentic path; `kImagingBandPeakRmsMax = 0.0022f` regression encodes legacy ceiling.

**Why it happens:** Stepping tank slower than host without bandlimited reconstruction folds energy to `hostRate - 32768` imaging zone (~14825 Hz at 48 kHz).

**How to avoid:** Ship ProperSRC for enablement; keep legacy only under `Authentic32Mode::LegacyAccumulator`.

**Warning signs:** Narrowband dominance ratio > 10; peak frequency ~14825 Hz on impulse tail.

### Pitfall 2: Per-sample r8brain calls

**What goes wrong:** Variable output cadence, wrong tank stepping rate, high CPU.

**Why it happens:** `CDSPResampler` is async pull processor designed for blocks [CITED: r8brain README "Real-Time Applications"].

**How to avoid:** `FixedRateAdapter::processBlock()` only in Phase 13; minimum `n=1` supported for SRC-03 but still one block API call.

**Warning signs:** Tank LFO sounds wrong; CPU spikes; output length ≠ input length inconsistently.

### Pitfall 3: Heap allocation on audio thread

**What goes wrong:** Glitches, realtime stress failures, pluginval risk.

**Why it happens:** `new CDSPResampler`, `std::vector::resize`, or `make_unique` in `processBlock()`.

**How to avoid:** All resamplers and `double`/`float` scratch in `prepare()`; `reset()` calls `clear()` only.

**Warning signs:** ASan hits in tests; `RealtimeStressTest` patterns fail when adapted to adapter-level stress (Phase 14).

### Pitfall 4: Reset incomplete (TEST-10)

**What goes wrong:** Prior tail bleeds into next render; impulse response differs after `reset()`.

**Why it happens:** r8brain internal FIFO + comb delay lines retain state; `clear()` required on both resamplers [CITED: CDSPResampler::clear docs].

**How to avoid:** `reset()` → `converters.reset()` + `core.reset()` (may need new method on core: predelay/comb/APF reset) + legacy accumulator zeroing.

**Warning signs:** Second impulse after reset has pre-echo; Goertzel power non-zero before tail on silent input.

### Pitfall 5: Double delay scaling

**What goes wrong:** RT60 wrong; predelay duration error in ProperSRC path.

**Why it happens:** `SchroederTankCore` prepared at 32768 **and** delays scaled for host rate.

**How to avoid:** `core.prepare(32768.0, maxBlock)` only; never pass `hostRate` to core in ProperSRC mode [VERIFIED: Phase 12 CORE-02].

**Warning signs:** RT60 tests at 32768 pass but adapter impulse decay wrong vs legacy.

## Code Examples

### r8brain pull loop (realtime downsample consume)

```cpp
// Source: https://github.com/avaneev/r8brain-free-src/blob/master/README.md
// Pull until hostOutFilled == numHostSamples
int hostWritten = 0;
while (hostWritten < numHostSamples)
{
    double* op = nullptr;
    const int produced = downsampler->process (internalBuf, nInternal, op);
    const int toCopy = std::min (produced, numHostSamples - hostWritten);
  // copy op[0..toCopy) to hostOut[hostWritten..]; carry remainder in FIFO
    hostWritten += toCopy;
}
```

### SRC-06 imaging comparison (impulse @ 48 kHz)

```cpp
// Source: tests/HighFrequencyRingingDiagnosticsTest.cpp pattern
constexpr auto kSampleRate = 48000.0;
constexpr auto kImagingHz = 14825.0;
constexpr auto kTailStart = 19200uz;  // after reverb build-up
constexpr auto kTailCount = 2400uz;
constexpr float kMinReduction = 0.70f; // SRC-06: proper <= legacy * (1 - 0.70)

const auto legacyIr = renderAdapterImpulse (adapter, Authentic32Mode::LegacyAccumulator);
const auto properIr = renderAdapterImpulse (adapter, Authentic32Mode::ProperSRC);

const auto legacyPower = sendbloom::test::goertzelPower (legacyIr, kSampleRate, kImagingHz, kTailStart, kTailCount);
const auto properPower = sendbloom::test::goertzelPower (properIr, kSampleRate, kImagingHz, kTailStart, kTailCount);

REQUIRE (properPower <= legacyPower * (1.0 - static_cast<double> (kMinReduction)));
```

### Multi-rate prepare smoke

```cpp
// SRC-03: host rates 44.1, 48, 88.2, 96 kHz
for (const double hostRate : { 44100.0, 48000.0, 88200.0, 96000.0 })
{
    FixedRateAdapter adapter;
    adapter.prepare (hostRate, 512);
    std::vector<float> impulse (4096, 0.0f);
    impulse[0] = 1.0f;
    for (size_t i = 0; i < impulse.size(); ++i)
        impulse[i] = adapter.processSampleOrBlock (...); // block API
    for (float s : impulse)
        REQUIRE (std::isfinite (s));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Inline `processAuthentic` in `SchroederTank32` | `FixedRateAdapter` + `SchroederTankCore@32768` | v2.0 Phase 13 | Separates SRC from tank; testable boundary |
| Accumulator + hold + anti-image SVF | r8brain bandlimited up/down | Phase 13 ProperSRC | Targets 70%+ imaging reduction at 14825 Hz |
| Monolithic tank class | Phase 12 core extraction | Phase 12 complete | Enables fixed-rate wrap without branch entropy |
| r8brain 6.x + `r8bbase.cpp` | 7.1 header-only C++11 | r8brain v7.0 | Simpler CMake; no pthread on C++11+ |

**Deprecated/outdated:**
- Anti-image SVF as primary fix — demote after ProperSRC passes HF gates (Phase 18 ENAB-03).
- Reporting zero latency with SRC active — ADR-003 deferred to Phase 17.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | r8brain `process()` steady-state does not heap-allocate per call when resampler object is reused | Standard Stack | Realtime violations; need offline ASan proof in tests |
| A2 | Default `ReqTransBand=2.0` / `ReqAtten=206.91` sufficient for SRC-06 70% imaging reduction on impulse | HF test | May need wider transition band tuning; latency rises |
| A3 | `SchroederTankCore::processSample` loop inside adapter is acceptable for Phase 13 (no core `processBlock` yet) | Architecture | Higher CPU vs block core API; acceptable until Phase 14 optimization |
| A4 | Legacy accumulator can be extracted without changing `SchroederTank32` public behavior in Phase 13 | Scope | Duplication until Phase 14 facade wiring |
| A5 | Round-trip latency non-zero but PDC unchanged in Phase 13 | Pitfalls | Acceptable — RC1 keeps 32k off; measurement lands Phase 17 |

## Open Questions

1. **Should Phase 13 add `SchroederTankCore::reset()` or re-`prepare()` on adapter reset?**
   - What we know: Combs/APF/predelay retain state; TEST-10 requires full clear.
   - What's unclear: Whether JUCE delay atoms expose uniform reset without new helper.
   - Recommendation: Add minimal `reset()` on core mirroring `prepare()` tail reset calls.

2. **Exact tail window for SRC-06 impulse test?**
   - What we know: HF diagnostics use `kTailStart = renderSamples - 4800`, `14825 Hz` tone.
   - What's unclear: Impulse adapter tail may peak earlier than guitar pluck.
   - Recommendation: Use same Goertzel frequency (14825 Hz); scan tail start at `0.4 * irLength` if needed; document chosen window in test.

3. **PFFFT double enable in Phase 13 or defer?**
   - What we know: Adds one `.c` file; quality equal to Ooura FFT.
   - Recommendation: Defer until after SRC-06 passes with default FFT (lower integration risk).

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | r8brain CPM fetch | ✓ | 4.3.3 | — |
| C++20 compiler | SendBloom + r8brain headers | ✓ | (system) | — |
| Node.js | gsd-tools research seam | ✓ | v22.22.3 | — |
| Network (CPM GitHub fetch) | First configure | ✓ [ASSUMED] | — | Vendor copy under `third_party/` per STACK.md |
| Catch2 | Adapter tests | ✓ | 3.8.1 via CPM | — |
| JUCE 8 | SchroederTankCore | ✓ | bundled | — |

**Missing dependencies with no fallback:** none identified for Phase 13 adapter work.

**Missing dependencies with fallback:** air-gapped CI → pin same git commit as local `third_party/r8brain-free-src` mirror.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (Catch.cmake discovery) |
| Quick run command | `ctest -R "FixedRateAdapter" --test-dir build` |
| Full suite command | `ctest --test-dir build` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SRC-01 | Bandlimited 48k↔32768 conversion | unit | `ctest -R "FixedRateAdapter.*ProperSRC.*48"` | ❌ Wave 0 |
| SRC-02 | No heap in `processBlock` | unit/stress | `ctest -R "FixedRateAdapter.*realtime"` | ❌ Wave 0 |
| SRC-03 | Multi-rate + variable block | unit | `ctest -R "FixedRateAdapter.*multiRate"` | ❌ Wave 0 |
| SRC-04 | Authentic32Mode enum values | unit | `ctest -R "Authentic32Mode"` | ❌ Wave 0 |
| SRC-05 | Legacy path still renders | unit | `ctest -R "LegacyAccumulator"` | ❌ Wave 0 |
| SRC-06 | ≥70% imaging reduction @ 14825 Hz impulse | unit | `ctest -R "FixedRateAdapter.*SRC-06"` | ❌ Wave 0 |
| TEST-10 | Reset clears state | unit | `ctest -R "FixedRateAdapter.*reset"` | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `ctest -R "FixedRateAdapter" --test-dir build`
- **Per wave merge:** `ctest --test-dir build`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `cmake/R8brain.cmake` — CPM pin + INTERFACE target
- [ ] `source/Authentic32Mode.h` — enum definition
- [ ] `source/RateConverterPair.h` — r8brain wrapper
- [ ] `source/FixedRateAdapter.h` — adapter + legacy branch
- [ ] `tests/FixedRateAdapterTest.cpp` — SRC-01–06, TEST-10
- [ ] `SchroederTankCore::reset()` (or documented re-prepare pattern) — TEST-10 support
- [ ] Root `CMakeLists.txt` include + `target_link_libraries(SharedCode INTERFACE r8brain)`

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes | Assert `numSamples <= maxBlockSize_`; clamp `rt60`/`darkMix` like core |
| V6 Cryptography | no | — |

### Known Threat Patterns for {stack}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Supply-chain malicious CPM package | Tampering | Pin git commit `e71c31bf`; official `avaneev/r8brain-free-src` repo only |
| Buffer overrun on variable blocks | Tampering | Size via `getMaxOutLen()`; reject `n > maxBlockSize_` |
| Realtime denial via allocation | Denial of Service | Prepare-time allocation only; stress test |

## Project Constraints (from .cursor/rules/)

- **Realtime safety:** Zero heap allocation, locks, logging, file I/O, or UI access in `processBlock()` after `prepare()` [`.claude/.cursor/rules/gsd-project.md`]
- **Tech stack:** JUCE 8, C++20, CMake, Catch2, pluginval — pamplejuce pattern
- **License:** MIT-compatible references only; r8brain MIT fits; cite in Phase 18 TEST-13
- **Audio contract:** Mono-authentic processing; stereo sums to mono internally
- **Parameter stability:** No new APVTS IDs in Phase 13; `Authentic32Mode` internal only
- **GSD workflow:** Phase work via `/gsd-execute-phase` orchestration

## Sources

### Primary (HIGH confidence)

- `source/SchroederTankCore.h`, `source/SchroederTank32.h`, `source/HostRateReverbEngine.h` — Phase 12 upstream, legacy authentic reference
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — 14825 Hz imaging metric, tail windows, Goertzel helpers
- `tests/ReverbTestHelpers.h`, `tests/ChainTestHelpers.h` — RT60 and spectral utilities
- [r8brain-free-src README](https://github.com/avaneev/r8brain-free-src/blob/master/README.md) — MIT, v7.1 header-only, realtime pull model
- [CDSPResampler reference](https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html) — `process()`, `clear()`, `getLatency()`, `getMaxOutLen()`

### Secondary (MEDIUM confidence)

- `.planning/research/STACK.md` — CMake CPM pin, buffer sizing guidance
- `.planning/research/ARCHITECTURE.md` — FixedRateAdapter sandwich, component boundaries
- `.planning/research/PITFALLS.md` — accumulator bug, heap alloc, per-sample SRC risks
- `.planning/REQUIREMENTS.md` — SRC-01–06, TEST-10 acceptance text

### Tertiary (LOW confidence)

- `.planning/research/SUMMARY.md` — latency numbers until measured on target ratios (Phase 17)

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — r8brain + Phase 12 core verified against official docs and codebase
- Architecture: HIGH — boundaries explicit in CONTEXT.md and ARCHITECTURE.md; Phase 14 integration deferred
- Pitfalls: HIGH — accumulator failure mode verified in production HF tests

**Research date:** 2026-07-08
**Valid until:** 2026-08-07 (30 days — r8brain stable; CMake pin frozen)
