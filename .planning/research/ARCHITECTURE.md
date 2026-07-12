# Architecture Research

**Domain:** SendBloom v2.0 — fixed-rate (32,768 Hz) reverb SRC integration in a JUCE 8 realtime audio plugin
**Researched:** 2026-07-08
**Confidence:** HIGH

## Executive Recommendation

Split the monolithic `SchroederTank32` into a **rate-agnostic tank core** and a **bandlimited SRC adapter**. Keep the existing per-sample wet chain (gates, send, OD) but move reverb + SRC to **block processing** inside `GatedBloomChain`. Use **dual-engine crossfade** (20–50 ms) when `authentic_color` toggles. Measure SRC group delay in `prepare()` and adopt **conditional PDC** (`setLatencySamples` nonzero only when ProperSRC is active).

The broken v1 authentic path (`processAuthentic` accumulator + hold + anti-image SVF) must not be patched — it becomes `LegacyAccumulator` in a diagnostics-only `Authentic32Mode` enum while production ships host-rate primary with ProperSRC gated behind acceptance tests.

---

## Standard Architecture

### System Overview

**Current (v1.0):**

```
┌─────────────────────────────────────────────────────────────────────────┐
│ PluginProcessor::processBlock (per-sample outer loop)                    │
├─────────────────────────────────────────────────────────────────────────┤
│  InputStage → EnvelopeDetector → GatedBloomChain::processSample          │
│       │                              │                                   │
│       │                    preGate → Send → IReverbEngine               │
│       │                              │                                   │
│       │                    SchroederTank32 (host OR authentic in one class)│
│       │                              │                                   │
│       │                    WetOverdrive → postGate                       │
│       └──────── ParallelWetMixer / bypass / OutputStage                 │
└─────────────────────────────────────────────────────────────────────────┘

SchroederTank32 authentic branch (broken):
  host sample ──► accumulator step @ 32768/hostRate ──► processTank @ 32k delays
              ──► linear hold ──► antiImage SVF @ hostRate ──► out
```

**Target (v2.0 Proper SRC):**

```
┌─────────────────────────────────────────────────────────────────────────┐
│ PluginProcessor::processBlock                                            │
├─────────────────────────────────────────────────────────────────────────┤
│  InputStage + envelope (per-sample OK)                                   │
│  GatedBloomChain::processBlock                                           │
│       │                                                                  │
│       ├─ per-sample: preGate, PressureSend  →  wetPreBuf[n]             │
│       │                                                                  │
│       ├─ block: IReverbEngine::processBlock(wetPreBuf → wetPostBuf)     │
│       │         ┌──────────────────────────────────────────────┐        │
│       │         │ SchroederTank32 (facade / router)             │        │
│       │         │  ├─ HostRateEngine  → SchroederTankCore@host  │        │
│       │         │  └─ FixedRateAdapter@32768                   │        │
│       │         │       upsampler (host→32768)                  │        │
│       │         │       SchroederTankCore @ 32768               │        │
│       │         │       downsampler (32768→host)                │        │
│       │         │       EngineCrossfade mixes host ↔ 32k path   │        │
│       │         └──────────────────────────────────────────────┘        │
│       │                                                                  │
│       └─ per-sample: WetOverdrive, postGate on wetPostBuf[n]            │
│  ParallelWetMixer / bypass / OutputStage (unchanged contract)           │
└─────────────────────────────────────────────────────────────────────────┘
```

### Component Responsibilities

| Component | Responsibility | Implementation |
|-----------|----------------|----------------|
| `SchroederTankCore` | Single-rate Schroeder tank: predelay → 4 APF → 4 combs → modulated tank AP | Extract `processTank` + `updateCoeffs` from `SchroederTank32`; `prepare(fixedRate)` sets delay integers from `SchroederTank32DelayTable` without host scaling branches |
| `HostRateReverbEngine` | Host-rate reverb using core with scaled delays | Thin wrapper: `scaleDelay(d, hostRate/32768)` at prepare; implements `IReverbEngine` |
| `FixedRateAdapter` | Bandlimited host ↔ 32,768 Hz bridge around core | Owns r8brain upsampler + `SchroederTankCore` @ 32768 + downsampler; block-only hot path |
| `RateConverterPair` | r8brain lifecycle, buffer sizing, latency query | MIT `r8brain-free-src`; preallocated `CDSPResampler` instances in `prepare()` |
| `SchroederTank32` | Production `IReverbEngine` facade; routes host vs authentic | **Modified:** delegates to `HostRateReverbEngine` + `FixedRateAdapter`; removes inline `processAuthentic` |
| `EngineCrossfade` | Clickless 32k Color engine swap | 20–50 ms wet-path crossfade between host and fixed outputs; dual-run only during fade |
| `Authentic32Mode` | Diagnostics path selector (not user-facing) | `Off` / `LegacyAccumulator` / `ProperSRC`; test harness + RC1 safety |
| `IReverbEngine` | Reverb polymorphism for chain + tests | **Extended:** `processBlock()` + `getReportedLatencySamples()` |
| `GatedBloomChain` | Wet signature chain slot | **Modified:** block wet reverb stage; per-sample gates/OD preserved |
| `PluginProcessor` | Host integration, PDC reporting | **Modified:** `setLatencySamples()` reflects SRC latency when ProperSRC active |

---

## Recommended Project Structure

```
source/
├── reverb/
│   ├── SchroederTankCore.h          # NEW — fixed-rate tank, no authentic/host branches
│   ├── SchroederTank32DelayTable.h  # existing — kInternalRate = 32768
│   ├── SchroederAllpass.h           # existing
│   ├── DampedComb.h                 # existing
│   ├── HostRateReverbEngine.h       # NEW — core @ host with scaled delays
│   ├── FixedRateAdapter.h           # NEW — SRC sandwich + core @ 32768
│   ├── SchroederTank32.h            # MODIFIED — facade + LegacyAccumulator diag
│   ├── EngineCrossfade.h            # NEW — engine output crossfade helper
│   ├── Authentic32Mode.h              # NEW — enum + mode resolver
│   └── IReverbEngine.h              # MODIFIED — processBlock + latency API
├── src/
│   └── RateConverterPair.h          # NEW — r8brain wrapper (upsample/downsample)
├── GatedBloomChain.h                # MODIFIED — processBlock wet path
├── PluginProcessor.cpp              # MODIFIED — chain block call, conditional PDC
└── Fdn8Reverb.h                     # unchanged — ignores authentic_color

tests/
├── SchroederTankCoreTest.cpp        # NEW — parity vs v1 tank @ 32k offline
├── FixedRateAdapterTest.cpp         # NEW — SRC round-trip, HF imaging metrics
├── ReverbEngineCrossfadeTest.cpp    # NEW — toggle clicklessness
├── LatencyTest.cpp                  # MODIFIED — mode-aware PDC assertions
└── AuthenticPathDiagnosticsTest.cpp # NEW — three-path harness
```

### Structure Rationale

- **`reverb/` subfolder:** Isolates tank atoms, core, adapters, and crossfade from chain routing — matches v1 `05-RESEARCH.md` decomposition (delay table → atoms → engine → chain).
- **`RateConverterPair` separate from adapter:** r8brain buffer sizing and latency math are reusable; adapter owns param wiring and core calls.
- **Facade stays `SchroederTank32`:** `GatedBloomChain` and `ReverbEngineSwapTest` keep stable type names; Fdn8 swap tests unaffected.

---

## Architectural Patterns

### Pattern 1: Core + Adapter (Fixed-Rate Bridge)

**What:** Tank DSP runs at exactly one sample rate per instance. Rate conversion is strictly outside the core.

**When to use:** Any time host rate ≠ 32,768 Hz (always for authentic color in production).

**Trade-offs:**
- (+) Eliminates `useAuthenticPath` / `hostRate` branches inside tank math
- (+) SRC quality and latency become adapter concerns, testable in isolation
- (−) Two engine instances during crossfade (brief CPU spike)
- (−) Block buffers and r8brain scratch memory increase `prepare()` footprint

**Example:**

```cpp
// FixedRateAdapter — conceptual block API
void processBlock (const float* hostIn, float* hostOut, int numHostSamples,
                   float rt60, float darkMix) noexcept
{
    const int nUp = upsampler.process (hostIn, numHostSamples, upBuf);
    core.processBlock (upBuf, upBuf, nUp, rt60, darkMix); // in-place @ 32768
    downsampler.process (upBuf, nUp, hostOut, numHostSamples);
}
```

### Pattern 2: Block Reverb, Per-Sample Chain Shell

**What:** Gates, envelope follower, and wet OD stay per-sample; only reverb+SRC runs block-wise.

**When to use:** SendBloom's wet path — reverb is the expensive, rate-sensitive stage; gates/OD are cheap and already host-synchronous.

**Trade-offs:**
- (+) Minimal change to `PressureSend`, `NoiseGate`, `WetOverdrive` contracts
- (+) r8brain gets efficient multi-sample calls
- (−) `GatedBloomChain` gains a scratch buffer (`wetPre`, `wetPost`)
- (−) Parameter smoothing for `rt60`/`darkMix` inside a block: hold block-start values or per-sample coeff update inside core (match v1: per-sample `updateCoeffs` is safest for parity)

**Recommendation:** Per-sample coeff updates inside `SchroederTankCore::processBlock` loop initially (parity with v1 `processSample`); optimize to block-hold later only if profiling demands it.

### Pattern 3: Dual-Engine Crossfade on Mode Toggle

**What:** On `authentic_color` edge, run host engine and fixed adapter in parallel; crossfade wet outputs over 20–50 ms; then idle the inactive engine.

**When to use:** Any reverb engine swap where internal delay state cannot be morphed (Schroeder tank state is not interchangeable across rates).

**Trade-offs:**
- (+) Avoids discontinuities and state corruption from instant delay-length resets
- (+) Reuses `BypassCrossfade` / `SmoothedValue` patterns already in codebase
- (−) ~2× reverb CPU during fade window (~20–50 ms — acceptable)
- (−) Must reset idle engine after fade completes to avoid stale tail on next toggle

**Example:**

```cpp
// EngineCrossfade — wet output mix during authentic_color transition
wetOut = hostEngineOut * (1.0f - fadeToFixed) + fixedAdapterOut * fadeToFixed;
```

Use existing `authenticColorTarget` smoother (15 ms in `SmoothedParameterBank`) as the **target**, but override with a dedicated 20–50 ms engine crossfade smoother triggered on boolean edge — param smoother alone is too fast for reverb state swap.

### Pattern 4: Conditional Plugin Delay (PDC)

**What:** `PluginProcessor::prepareToPlay` measures round-trip SRC latency; `setLatencySamples()` reports it only when ProperSRC + `authentic_color` active.

**When to use:** When r8brain group delay is non-negligible and parallel dry/wet routing must stay sample-aligned.

**Trade-offs:**
- (+) Honest host compensation vs v1 "zero latency" lie when SRC is on
- (−) Breaks v1 `LatencyTest` contract — tests must become mode-aware
- (−) Latency may change with host rate → recompute in `prepareToPlay`, not cached globally

**Recommendation:** Default RC1 ships `Authentic32Mode::Off` (32k Color disabled) → latency stays 0 for release candidate. Enable conditional PDC when ProperSRC graduates from diagnostics.

---

## Data Flow

### Wet Path (Production, ProperSRC enabled)

```
monoIn[n] (per sample)
    ↓
preGate (optional) → PressureSend
    ↓
wetPreBuf[n]  ─────────────────────────────┐
    │ (end of block)                         │
    ↓                                        │
RateConverterPair::upsample                   │
    hostRate → 32768 Hz                       │
    ↓                                        │
SchroederTankCore::processBlock @ 32768     │  GatedBloomChain
    fixed delay table, 9-bit quant when on    │  ::processBlock
    ↓                                        │
RateConverterPair::downsample                 │
    32768 → hostRate                          │
    ↓                                        │
wetPostBuf[n] ← EngineCrossfade ← host path ─┘
    ↓
WetOverdrive → postGate
    ↓
ParallelWetMixer(dry, wet)
```

### Host-Rate Path (`authentic_color` off)

```
wetPreBuf[n] → HostRateReverbEngine::processBlock
                 (SchroederTankCore @ hostRate, scaled delays)
             → wetPostBuf[n]
```

No SRC, no extra latency, no crossfade — this is the RC1 primary path.

### Engine Toggle Flow

```
authentic_color: OFF → ON
    1. Detect edge in SchroederTank32 or EngineCrossfade controller
    2. fadeToFixed: 0 → 1 over 20–50 ms (dedicated smoother)
    3. Each block: hostEngine.processBlock + fixedAdapter.processBlock
    4. Mix outputs; after fade complete: hostEngine.reset()
    5. Update setLatencySamples(srcLatency) when fade completes

authentic_color: ON → OFF
    Reverse fade; reset fixedAdapter; setLatencySamples(0)
```

### Key Data Flows

1. **Parameter flow:** `ParameterSnapshot` → `SmoothedParameterBank` (block start targets) → per-sample gate/send; `rt60`/`darkMix`/`authenticColor` fed to reverb block (coeffs updated per-sample inside core for v2.0 parity).
2. **Latency flow:** `RateConverterPair::getRoundTripLatency(hostRate)` → `FixedRateAdapter::getLatencySamples()` → `SchroederTank32::getLatencySamples()` → `PluginProcessor::setLatencySamples()` when ProperSRC active.
3. **Diagnostics flow:** `Authentic32Mode` selects host / legacy accumulator / proper SRC without exposing a third UI mode; HF imaging harness compares magnitude spectrum 12–20 kHz on impulse at 48 kHz host.

---

## Scaling Considerations

| Scale | Architecture Adjustments |
|-------|---------------------------|
| Single plugin instance | Monolith in `source/reverb/` is fine; no service split |
| Host rates 44.1–192 kHz | `RateConverterPair` constructed per `prepare(hostRate)`; r8brain handles arbitrary ratios; recompute latency each prepare |
| Block sizes 32–1024 (pluginval stress) | Preallocate `getMaxOutLen(maxBlock)` scratch in `prepare()`; never resize in `processBlock` |
| CPU budget (guitar FX) | Dual-engine crossfade only during 20–50 ms; authentic off = zero SRC cost |

### Scaling Priorities

1. **First bottleneck:** Per-sample outer loop in `PluginProcessor` + dual reverb during crossfade — profile with 48 kHz / 512 block before optimizing coeff updates.
2. **Second bottleneck:** r8brain filter quality vs latency — if PDC unacceptable, try shorter transition band in `CDSPResampler` constructor before abandoning SRC.

---

## Anti-Patterns

### Anti-Pattern 1: Patching the Accumulator Path

**What people do:** Tune `kAuthenticAntiImageLpHz` or add more SVF stages to hide 14–15 kHz imaging.

**Why it's wrong:** Imaging is structural (non-bandlimited upsample); lowpass masks HF doesn't fix intermodulation.

**Do this instead:** Keep accumulator as `LegacyAccumulator` diagnostic only; ship ProperSRC or keep 32k Color off (RC1).

### Anti-Pattern 2: Sample-at-a-Time r8brain Calls

**What people do:** Call resampler `process()` with `l0=1` inside `processSample` to avoid chain refactor.

**Why it's wrong:** r8brain is asynchronous block processor; per-sample calls cause variable output cadence, higher CPU, and fragile latency accounting.

**Do this instead:** Block `processBlock` in adapter; per-sample chain only before/after reverb.

### Anti-Pattern 3: Instant Delay-Length Switch on Toggle

**What people do:** Reuse v1 `resetDelayLengths(authentic)` on param change inside audio thread.

**Why it's wrong:** Delay discontinuity + incompatible tank state → clicks and burst noise.

**Do this instead:** `EngineCrossfade` with dual engines; reset idle engine after fade.

### Anti-Pattern 4: Heap Allocation in SRC Process Path

**What people do:** `std::vector` resize per block for r8brain output length.

**Why it's wrong:** Violates CHN-05 realtime contract; fails stress tests.

**Do this instead:** Size buffers in `prepare()` using `getMaxOutLen(maxBlock)` + headroom for rate ratio.

### Anti-Pattern 5: Reporting Zero Latency With SRC Active

**What people do:** Keep `setLatencySamples(0)` to preserve v1 tests while enabling ProperSRC.

**Why it's wrong:** Parallel wet/dry paths drift; automation and PDC-aware hosts misalign.

**Do this instead:** Conditional PDC when authentic on; update `LatencyTest` and document in ADR-003.

---

## Integration Points

### New Components

| Component | File | Depends On |
|-----------|------|------------|
| `SchroederTankCore` | `source/reverb/SchroederTankCore.h` | `DampedComb`, `SchroederAllpass`, delay table |
| `HostRateReverbEngine` | `source/reverb/HostRateReverbEngine.h` | `SchroederTankCore` |
| `RateConverterPair` | `source/src/RateConverterPair.h` | r8brain (CMake `FetchContent` or `third_party/`) |
| `FixedRateAdapter` | `source/reverb/FixedRateAdapter.h` | `RateConverterPair`, `SchroederTankCore` |
| `EngineCrossfade` | `source/reverb/EngineCrossfade.h` | `juce::SmoothedValue` |
| `Authentic32Mode` | `source/reverb/Authentic32Mode.h` | — |
| Diagnostics harness | `tests/AuthenticPathDiagnosticsTest.cpp` | All three paths |

### Modified Components

| Component | Change | Risk |
|-----------|--------|------|
| `IReverbEngine.h` | Add `processBlock(...)`, default `getLatencySamples()` → 0 | Fdn8Reverb needs stub `processBlock` (loop `processSample`) |
| `SchroederTank32.h` | Extract core; delegate authentic to `FixedRateAdapter`; retain `LegacyAccumulator` behind `Authentic32Mode` | RT60 test regressions — run full verb suite after each step |
| `GatedBloomChain.h` | Add `processBlock`; wet scratch buffers; call `reverb->processBlock` | Routing tests must stay green |
| `PluginProcessor.cpp` | Call `chain.processBlock` instead of per-sample chain (or hybrid) | Largest integration diff |
| `CMakeLists.txt` | r8brain sources + include path | License MIT — verify `docs/CLEAN_ROOM.md` still accurate |
| `LatencyTest.cpp` | Mode-aware expectations | Document RC1 vs v2.0 policy |
| `docs/RELEASE_CHECKLIST.md` | Update 32k Color truth section when ProperSRC ships | — |

### Internal Boundaries

| Boundary | Communication | Notes |
|----------|---------------|-------|
| `GatedBloomChain` ↔ `IReverbEngine` | `processBlock(mono in/out, numSamples, rt60, darkMix, authenticColor)` | `authenticColor` bool still passed; engine facade picks path |
| `SchroederTank32` ↔ `FixedRateAdapter` | Composition; adapter never exposed to chain directly | Chain sees one engine type |
| `FixedRateAdapter` ↔ `RateConverterPair` | Upsample/downsample block pointers + latency query | Adapter owns core; pair owns r8brain state |
| `PluginProcessor` ↔ latency | `prepareToPlay` → measure → `setLatencySamples` | Re-run on sample-rate change |
| Tests ↔ `Authentic32Mode` | Friend or test-only setter on processor/engine | Not exposed in APVTS |

### Suggested Build Order

Dependencies flow top-down; each step should keep CI green.

| Step | Deliverable | Gate |
|------|-------------|------|
| **1** | `SchroederTankCore` extraction + unit tests | Impulse/RT60 parity with pre-refactor host tank at same rate |
| **2** | `HostRateReverbEngine` wrapping core | `SchroederTank32Test` passes with core-backed host path |
| **3** | r8brain CMake + `RateConverterPair` | Offline round-trip sine sweep; no heap in process |
| **4** | `FixedRateAdapter` (SRC sandwich, no crossfade) | HF imaging metric < legacy accumulator at 48 kHz |
| **5** | `IReverbEngine::processBlock` + `Fdn8Reverb` stub | `ReverbEngineSwapTest` green |
| **6** | Wire adapter into `SchroederTank32` authentic branch | `ReleaseTruthTest` authentic distinctness preserved |
| **7** | `GatedBloomChain::processBlock` | `PluginBasics`, `RealtimeStressTest` green |
| **8** | `PluginProcessor` block chain integration | Full ctest suite |
| **9** | `EngineCrossfade` + `Authentic32Mode` | Toggle stress test (no click); three-path diagnostics |
| **10** | Latency measure + `setLatencySamples` policy + ADR-003 | `LatencyTest` updated; RELEASE_CHECKLIST truth |

**RC1 safety (parallel track, step 0):** Set all presets `authentic_color=0`; default `Authentic32Mode::Off` in production builds until step 4 gate passes.

---

## `IReverbEngine` Contract (Proposed)

```cpp
class IReverbEngine
{
public:
    virtual void prepare (double sampleRate, int maxBlockSize) noexcept = 0;

    // Retain for Fdn8 tests and incremental migration
    virtual float processSample (float input, float rt60Seconds,
                                 float darkMix, bool authenticColor) noexcept = 0;

    // Primary hot path for v2.0+
    virtual void processBlock (const float* input, float* output, int numSamples,
                               float rt60Seconds, float darkMix,
                               bool authenticColor) noexcept = 0;

    virtual int getLatencySamples() const noexcept { return 0; }
};
```

Default `processBlock` implementation (for `Fdn8Reverb`): loop `processSample`. `SchroederTank32` overrides with block-aware host + adapter paths.

---

## Latency / PDC Decision Matrix

| Policy | `authentic_color` off | `authentic_color` on (ProperSRC) | RC1 fit |
|--------|----------------------|----------------------------------|---------|
| **A — Conditional PDC** (recommended) | `setLatencySamples(0)` | `setLatencySamples(roundTripSrc)` | Off by default → RC1 safe |
| **B — Zero always** | 0 | 0 | Preserves v1 tests; misaligns parallel buses when 32k on |
| **C — Defer 32k ship** | 0 | N/A until PDC solved | Safest marketing; delays milestone value |
| **D — Low-latency SRC preset** | 0 | Reduced latency, more imaging | Fallback if A audibly bothers parallel mix |

Measure with impulse through full adapter at 44.1 / 48 / 96 kHz; record in ADR-003. Use `CDSPResampler::getInputRequiredForOutput` and round-trip impulse peak alignment as cross-check.

---

## Sources

- `source/SchroederTank32.h` — current host/authentic monolith; `processAuthentic` accumulator anti-pattern
- `source/IReverbEngine.h` — per-sample-only contract to extend
- `source/GatedBloomChain.h` — reverb slot; per-sample chain insertion point
- `source/PluginProcessor.cpp` — `setLatencySamples(0)`; per-sample outer loop
- `source/SchroederTank32DelayTable.h` — `kInternalRate = 32768.0`
- `.planning/PROJECT.md` — v2.0 milestone requirements, RC1 strategy, r8brain-first decision
- `.planning/milestones/v1.0-phases/05-schroedertank32-reverb/05-RESEARCH.md` — tank decomposition precedent
- `.planning/milestones/v1.0-phases/08-full-integration-realtime-fallback/08-RESEARCH.md` — zero-latency audit, `IReverbEngine` pattern
- `docs/RELEASE_CHECKLIST.md` — honest 32k Color behavior documentation
- [r8brain CDSPResampler](https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html) — block async SRC, `getLatency`, `getInputRequiredForOutput`
- [r8brain-free-src GitHub](https://github.com/avaneev/r8brain-free-src/) — MIT license, streaming usage notes

---
*Architecture research for: SendBloom v2.0 Proper 32k SRC integration*
*Researched: 2026-07-08*
