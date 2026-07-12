# Phase 14: Block-Level Integration - Research

**Researched:** 2026-07-08
**Domain:** JUCE audio plugin DSP integration — block-level reverb/SRC inside per-sample wet chain
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

_No explicit locked decisions — infrastructure phase with auto-generated context._

### Claude's Discretion

All implementation at Claude's discretion per ROADMAP INTEG-01–04 and TEST-09. Preserve v1.0 gate/send/OD/dry behavior exactly. 32k Color toggle stays off-by-default (Phase 11).

### Deferred Ideas (OUT OF SCOPE)

- Three-path diagnostics — Phase 15
- Engine crossfade — Phase 16
- PDC / latency policy — Phase 17
- User-facing 32k Color enablement — Phase 18
- New UI controls
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| INTEG-01 | `IReverbEngine` extended with `processBlock()` for fixed-rate path; `processSample()` retained for host-rate compatibility | Default `processBlock` loops `processSample`; `SchroederTank32` overrides to delegate `authenticColor=true` to `FixedRateAdapter::processBlock` with `Authentic32Mode::ProperSRC` |
| INTEG-02 | `GatedBloomChain` processes reverb+SRC at block level; gates, pressure send, and wet OD remain at host rate | Two-phase block API: per-sample gate+send → `reverb->processBlock` → per-sample OD+post-gate; scratch buffers in `prepare()` |
| INTEG-03 | Wet overdrive, dry routing, gate behavior, and pressure send unchanged by this milestone | Keep gate/send/OD/dry code paths identical; only reverb call site changes when `authenticColor=true`; `authenticColor=false` preserves exact per-sample host path |
| INTEG-04 | No new UI controls; existing 32k Color toggle preserved (off by default) | Map APVTS `authentic_color` only — no new `ParameterIDs`; diagnostics `Authentic32Mode` via test-only setter, not APVTS |
| TEST-09 | Fixed32SRC does not allocate in `processBlock()` (realtime stress) | Extend static alloc-token scan + 10k-block `RealtimeStressTest` with `authentic_color=1`; reuse Phase 13 `FixedRateAdapter` alloc-free contract |
</phase_requirements>

## Summary

Phase 14 wires the Phase 13 `FixedRateAdapter` (ProperSRC sandwich) into the live plugin path through `GatedBloomChain`, without changing the v1.0 wet-chain topology for gates, pressure send, wet overdrive, or dry routing. Today `PluginProcessor::processBlock` runs a **per-sample** loop that calls `GatedBloomChain::processSample`, which in turn calls `reverb->processSample` on `SchroederTank32` — including the legacy accumulator authentic path inside the tank [VERIFIED: `source/PluginProcessor.cpp`, `source/GatedBloomChain.h`, `source/SchroederTank32.h`]. Phase 13 already delivers a realtime-safe, alloc-free `FixedRateAdapter::processBlock` with r8brain SRC at 32,768 Hz [VERIFIED: `source/FixedRateAdapter.h`, Phase 13 verification report].

The integration splits processing **by responsibility**, not by replacing the whole chain with blocks. Gates (`NoiseGate`), `PressureSend`, and `WetOverdrive` stay **per-sample at host rate** because they depend on per-sample envelope and smoothed parameters. Reverb + SRC move to **block level** only when `authentic_color` is active, calling a new `IReverbEngine::processBlock` that `SchroederTank32` implements by delegating to `FixedRateAdapter` with `Authentic32Mode::ProperSRC`. When `authentic_color` is off (RC1 default), the existing per-sample `processSample` → `HostRateReverbEngine` path must remain bit-for-bit equivalent to pre-integration behavior for all non-reverb stages.

**Primary recommendation:** Add `IReverbEngine::processBlock` with a default per-sample fallback; embed `FixedRateAdapter` in `SchroederTank32`; add `GatedBloomChain::processBlock` with preallocated scratch buffers; refactor `PluginProcessor` to call the chain once per host block while keeping dry routing per-sample; gate TEST-09 with static alloc scans + extended `RealtimeStressTest`.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Host block I/O, dry tap, stereo mix | PluginProcessor (audio thread) | — | JUCE `processBlock` entry; dry buffer copy unchanged |
| Parameter snapshot + smoothing | PluginProcessor / SmoothedParameterBank | — | One snapshot per block; per-sample smoothing for gates/level |
| Envelope + gate + send + OD | GatedBloomChain (per-sample) | — | Envelope and gate profiles are sample-accurate; INTEG-02 mandates host-rate |
| Reverb tank (host rate) | IReverbEngine::processSample | SchroederTank32 → HostRateReverbEngine | RC1 path when `authentic_color=0` |
| Reverb tank + SRC (32k) | IReverbEngine::processBlock | SchroederTank32 → FixedRateAdapter | ProperSRC only in production; block SRC requires r8brain FIFO continuity |
| Authentic32Mode diagnostics | SchroederTank32 (test setter) | Phase 15 harness | Not APVTS; LegacyAccumulator for A/B only |
| Realtime alloc safety | All `processBlock` bodies | CMake RTSan (optional) | CHN-05 + TEST-09; prepare-only heap |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8 (submodule) | `AudioProcessor`, DSP primitives | Project scaffold [VERIFIED: `CMakeLists.txt`] |
| r8brain-free-src | pin `e71c31bf` | hostRate ↔ 32768 SRC | Phase 13 `cmake/R8brain.cmake`; MIT [VERIFIED: Phase 13 verification] |
| Catch2 | 3.8.1 | Unit/integration tests | `cmake/Tests.cmake` [VERIFIED: `cmake/Tests.cmake`] |
| C++20 | — | Language standard | `target_compile_features` on Tests/SharedCode |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| ctest | system | Test orchestration | CI + local `ctest --test-dir build` |
| RTSan (Clang ≥20) | optional | Runtime alloc detection | Optional CI/debug `-DWITH_REALTIME_SANITIZER=ON` [VERIFIED: `cmake/Sanitizers.cmake`] |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Extend `IReverbEngine` in place | Separate `IFixedRateReverb` interface | Duplicates engine swap tests; breaks `GatedBloomChain` polymorphism |
| Block entire `GatedBloomChain` including gates | Per-sample gates + block reverb only | Violates INTEG-02/03; envelope is inherently per-sample |
| Replace `SchroederTank32` authentic accumulator in Phase 14 | Keep accumulator until Phase 16 crossfade | Production path must use ProperSRC when color on; accumulator stays behind `Authentic32Mode::LegacyAccumulator` for Phase 15 only |

**Installation:** No new packages — reuse existing `SharedCode` linkage to r8brain INTERFACE target from Phase 13.

**Version verification:** No new npm/PyPI packages. Existing pins confirmed in repo (`Catch2@3.8.1` via CPM, r8brain git pin in `cmake/R8brain.cmake`).

## Package Legitimacy Audit

> Phase 14 installs **no new external packages**. Existing dependencies verified in Phase 13.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| _(none new)_ | — | — | — | — | — | No installs this phase |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
PluginProcessor::processBlock(buffer)
        │
        ├─► capture ParameterSnapshot (once)
        ├─► copy dry tap → dryBuffer
        │
        ▼
   per host block ─────────────────────────────────────────────┐
        │                                                        │
        ├─► per-sample: input gain → monoIn, envelope           │
        │                                                        │
        ▼                                                        │
 GatedBloomChain::processBlock(...)                              │
        │                                                        │
        ├─► [per-sample] pre-gate? → PressureSend → wetSendBuf[] │
        │                                                        │
        ├─► authentic_color == false?                            │
        │      └─► per-sample: reverb->processSample (host)      │
        │          → OD → post-gate → wetOut[]                   │
        │                                                        │
        └─► authentic_color == true?                             │
               └─► reverb->processBlock(wetSendBuf → revBuf)     │
                   (FixedRateAdapter ProperSRC @ 32768)          │
               └─► [per-sample] OD → post-gate → wetOut[]      │
        │                                                        │
        ▼                                                        │
   per-sample: ParallelWetMixer + bypass + OutputStage ──────────┘
        │
        ▼
   buffer (wet+dry mixed)
```

### Recommended Project Structure

```
source/
├── IReverbEngine.h           # + processBlock() virtual
├── SchroederTank32.h         # embed FixedRateAdapter; route authentic path
├── GatedBloomChain.h         # + processBlock(); scratch buffers
├── FixedRateAdapter.h        # unchanged API from Phase 13
├── Authentic32Mode.h         # diagnostics enum; test setter on tank
├── PluginProcessor.cpp       # call chain.processBlock per host block
tests/
├── RealtimeStressTest.cpp    # TEST-09 extension
├── GatedBloomChainTest.cpp   # v1 parity cases (authentic off)
└── BlockIntegrationTest.cpp  # new: optional focused INTEG tests
```

### Pattern 1: IReverbEngine::processBlock() extension

**What:** Add block API to the existing polymorphic reverb interface. Default implementation loops `processSample` so `Fdn8Reverb` and `HostRateReverbEngine` need no block-specific logic.

**When to use:** Any engine behind `GatedBloomChain`; override only where block SRC is required (`SchroederTank32`).

**Example:**

```cpp
// source/IReverbEngine.h
class IReverbEngine
{
public:
    virtual ~IReverbEngine() = default;
    virtual void prepare (double sampleRate, int maxBlockSize) noexcept = 0;

    virtual float processSample (float input, float rt60Seconds,
                                 float darkMix, bool authenticColor) noexcept = 0;

    virtual void processBlock (const float* input, float* output, int numSamples,
                               float rt60Seconds, float darkMix,
                               bool authenticColor) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample (input[i], rt60Seconds, darkMix, authenticColor);
    }
};
```

**SchroederTank32 override contract [VERIFIED: `source/FixedRateAdapter.h` API]:**

| `authenticColor` | `processBlock` behavior |
|------------------|-------------------------|
| `false` | Default loop → `hostEngine.processSample` (same as today) |
| `true` | `fixedRate_.processBlock(in, out, n, rt60, darkMix, diagnosticsMode_)` where production `diagnosticsMode_` defaults to `Authentic32Mode::ProperSRC` |

Add non-APVTS diagnostics hook for Phase 15:

```cpp
void setAuthentic32ModeForDiagnostics (Authentic32Mode mode) noexcept
{
    diagnosticsMode_ = mode;
}
```

Map APVTS: `authentic_color=true` → force `ProperSRC` in production (ignore diagnostics override unless test sets it explicitly).

### Pattern 2: GatedBloomChain block vs per-sample split

**What:** Single `processBlock` entry that preserves v1 sample order for gate/send/OD while batching only the reverb+SRC stage.

**When to use:** Always from `PluginProcessor` after Phase 14 (replaces inner per-sample `chain.processSample` loop).

**Example:**

```cpp
void GatedBloomChain::processBlock (const float* monoIn, const float* envelope,
                                    float* wetOut, int n,
                                    float rt60, float darkMix, bool authenticColor,
                                    float distnBlend, float sendGain,
                                    bool gatePreSoft, float thresholdDb) noexcept
{
    if (n > maxBlockSize_)
        return;

    if (! authenticColor)
    {
        // INTEG-03: identical to current processSample loop
        for (int i = 0; i < n; ++i)
            wetOut[i] = processSample (monoIn[i], envelope[i], rt60, darkMix, false,
                                       distnBlend, sendGain, gatePreSoft, thresholdDb);
        return;
    }

    for (int i = 0; i < n; ++i)
    {
        auto wet = monoIn[i];
        if (gatePreSoft)
            wet *= preGate.process (envelope[i], thresholdDb);
        wetSendScratch_[i] = PressureSend::process (wet, sendGain);
    }

    reverb->processBlock (wetSendScratch_.data(), reverbScratch_.data(), n,
                        rt60, darkMix, true);

    for (int i = 0; i < n; ++i)
    {
        auto wet = overdrive.process (reverbScratch_[i], distnBlend);
        if (! gatePreSoft)
            wet *= postGate.process (envelope[i], thresholdDb);
        wetOut[i] = wet;
    }
}
```

**Scratch buffers:** `wetSendScratch_`, `reverbScratch_` — `std::vector<float>` sized in `prepare(sampleRate, maxBlockSize)` only.

**Parameter note:** Use block-constant `rt60`/`darkMix` taken from the **first sample** of the block (or last — pick one and document). Smoothed params change slowly (15 ms authentic color smooth); mid-block toggles of `authentic_color` are handled in Phase 16 — for Phase 14, branch on block-start value or split block when toggle detected [ASSUMED].

### Pattern 3: PluginProcessor wiring

**What:** Replace the inner `chain.processSample` call with one `chain.processBlock` per host block. Dry routing, bypass, extended stereo stay per-sample after wet vector is filled.

**Example sketch:**

```cpp
// After building monoIn[] and envelope[] arrays for the block:
std::vector<float> wetBlock (numSamples);
chain.processBlock (monoIn.data(), envelope.data(), wetBlock.data(), numSamples,
                    rt60AtBlockStart, darkAtBlockStart, authenticAtBlockStart,
                    distnAtBlockStart, sendAtBlockStart, gatePreSoft, thresholdDb);

for (int sample = 0; sample < numSamples; ++sample)
{
    const auto wet = wetBlock[sample];
    // existing ParallelWetMixer / bypass / OutputStage per channel — unchanged
}
```

Alternative with stack arrays: if `numSamples <= prepared max`, use member scratch in `PluginProcessor` or `GatedBloomChain` to avoid per-block heap (TEST-09).

### Pattern 4: FixedRateAdapter wiring when authentic_color on

**What:** `SchroederTank32` owns a `FixedRateAdapter` member, prepared in `prepare(hostRate, maxBlockSize)` alongside existing host engine.

**Production mapping:**

| User / APVTS | `Authentic32Mode` | Path |
|--------------|-------------------|------|
| `authentic_color=0` | _(unused)_ | Host-rate `processSample` |
| `authentic_color=1` | `ProperSRC` | `FixedRateAdapter::processBlock` |
| Test diagnostics | `LegacyAccumulator` | Phase 15 A/B only — not production |

**Do not** call `SchroederTank32::processAuthentic` accumulator in production after Phase 14 — ProperSRC replaces it when color is on (VERB-05 evolves in Phase 18 docs; INTEG-03 preserves gate/send/OD, not accumulator timbre).

### Anti-Patterns to Avoid

- **Calling `FixedRateAdapter` per sample with `n=1`:** Destroys r8brain FIFO efficiency and may violate SRC continuity at block boundaries [CITED: Phase 13 research — downsampler leftover FIFO].
- **Heap in `GatedBloomChain::processBlock`:** `vector::resize`, `make_unique`, `std::string` — fails TEST-09/CHN-05 [VERIFIED: `tests/FixedRateAdapterTest.cpp` static scan pattern].
- **Routing `authentic_color` to `LegacyAccumulator` in production:** Fails SRC-06 imaging contract; legacy is diagnostics-only [VERIFIED: `Authentic32Mode.h`, REQUIREMENTS SRC-04/05].
- **Moving gates inside `processBlock` reverb:** Breaks per-sample envelope accuracy and INTEG-03 parity.
- **Changing `ParameterIDs` or UI for diagnostics mode:** Violates INTEG-04 and SRC-04.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Block SRC host ↔ 32768 | Per-sample accumulator in `SchroederTank32` | `FixedRateAdapter` + `RateConverterPair` | Phase 13 proved imaging fix; accumulator fails SRC-06 |
| Default `processBlock` for test engines | Custom block code in `Fdn8Reverb` | `IReverbEngine` default loop | One override point (`SchroederTank32`) |
| Alloc detection | Custom malloc hooks only | Static token scan + existing stress tests + optional RTSan | Phase 13 pattern already in repo |
| v1 parity proof | Ad-hoc listening tests | Existing `GatedBloomChainTest`, `DryPathIntegrityTest`, `ReleaseTruthTest` host-rate renders | Automated regression already exists |

**Key insight:** Phase 14 is a **wiring** phase — the DSP hard problems (SRC, core extraction) are solved in Phases 12–13. The planner should not reopen r8brain or tank math.

## Common Pitfalls

### Pitfall 1: Mid-block `authentic_color` toggle without crossfade

**What goes wrong:** Discontinuity or state mismatch between host engine and `FixedRateAdapter` when toggle occurs mid-buffer.

**Why it happens:** Smoothed `authenticColorTarget` can change within a 512-sample block; Phase 16 crossfade not yet implemented.

**How to avoid:** For Phase 14, detect toggle and either (a) process sub-blocks at boundaries, or (b) use block-start bool and document deviation until Phase 16. `RealtimeStressTest` toggles every 50 blocks — safe.

**Warning signs:** `RealtimeStressTest` authentic toggle case throws or produces non-finite samples.

### Pitfall 2: Scratch buffer underrun on `numSamples > maxBlockSize`

**What goes wrong:** Silent return or OOB write — `FixedRateAdapter` already guards `n > maxBlockSize_` with early return [VERIFIED: `source/FixedRateAdapter.h` line 39].

**How to avoid:** Mirror guard in `GatedBloomChain`; extend `RealtimeStressTest` "larger-than-prepared block" case with `authentic_color=1`.

### Pitfall 3: False TEST-09 pass from adapter-only coverage

**What goes wrong:** `FixedRateAdapter` is alloc-free but new glue code allocates (`wetSendScratch_.resize` in `processBlock`).

**How to avoid:** Static scan `GatedBloomChain::processBlock` and `SchroederTank32::processBlock` bodies; run plugin-level stress with `authentic_color=1`.

### Pitfall 4: Breaking v1 parity by refactoring dry path

**What goes wrong:** INTEG-03 failure — dry THD, gate timing, or send trail tests fail.

**How to avoid:** Touch only reverb call site; run full chain test suite with `authentic_color=0` unchanged.

**Warning signs:** `GatedBloomChain send release preserves tank energy`, `dry path THD unchanged`, `PostGateTimingTest` regressions.

## Code Examples

### SchroederTank32::processBlock delegation

```cpp
// Source: pattern derived from source/FixedRateAdapter.h, source/SchroederTank32.h
void processBlock (const float* input, float* output, int numSamples,
                   float rt60Seconds, float darkMix, bool authenticColor) noexcept override
{
    if (! authenticColor)
    {
        IReverbEngine::processBlock (input, output, numSamples, rt60Seconds, darkMix, false);
        return;
    }

    const auto mode = diagnosticsModeOverride_.value_or (Authentic32Mode::ProperSRC);
    fixedRate_.processBlock (input, output, numSamples, rt60Seconds, darkMix, mode);
}
```

### TEST-09 static alloc scan (extend Phase 13 pattern)

```cpp
// Source: tests/FixedRateAdapterTest.cpp lines 614-671
TEST_CASE ("Integrated processBlock has no heap allocation tokens", "[realtime][TEST-09][static]")
{
    const auto chainText = readSourceFile ("source/GatedBloomChain.h");
    const auto tankText  = readSourceFile ("source/SchroederTank32.h");
    for (const auto& text : { chainText, tankText })
    {
        const auto body = extractProcessBlockBody (text);
        REQUIRE (body.find (".resize(") == std::string::npos);
        REQUIRE (body.find ("make_unique") == std::string::npos);
    }
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Authentic path: accumulator + hold in `SchroederTank32` | ProperSRC via `FixedRateAdapter` when color on | Phase 14 integration | HF imaging fixed (SRC-06); production toggle still off-default |
| Reverb only via `processSample` | `processBlock` for fixed-rate; `processSample` for host | Phase 14 | Enables r8brain block FIFO |
| Plugin per-sample chain only | Block reverb + per-sample gate/OD | Phase 14 | TEST-09 scope moves to plugin path |

**Deprecated/outdated:**
- `SchroederTank32::processAuthentic` as production path when `authentic_color=1` — superseded by ProperSRC (accumulator retained as `LegacyAccumulator` for diagnostics only).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Block-constant `rt60`/`darkMix` for reverb within a host block is acceptable | Pattern 2 | Subtle timbre delta during fast parameter automation; unlikely for SendBloom controls |
| A2 | Mid-block `authentic_color` toggles can use block-start bool until Phase 16 crossfade | Pitfall 1 | Brief glitch on toggle — acceptable for Phase 14; stress test toggles infrequently |
| A3 | `std::optional<Authentic32Mode>` or member default is sufficient for Phase 15 diagnostics access | Pattern 1 | May need richer harness API in Phase 15 — minor refactor |
| A4 | `Fdn8Reverb` default `processBlock` loop is adequate for test engine swap | Pattern 1 | None for production — Fdn8 is test-only |

## Open Questions (RESOLVED)

1. **Mono input arrays in PluginProcessor** — RESOLVED
   - Decision: Member buffers (`monoScratch_`, `envelopeScratch_`, `wetScratch_`) sized in `prepareToPlay` — matches `dryBuffer` pattern and satisfies TEST-09 (plan 14-03).

2. **Bit-exact vs tolerance parity for `authentic_color=0`** — RESOLVED
   - Decision: Tolerance parity via existing GatedBloomChain/DryPath/PostGate tests; max abs diff 1e-5f for block-vs-sample harness in plan 14-02; no new bit-exact harness unless regression appears.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake + Ninja/Make | Build | ✓ | (project default) | — |
| Catch2 | Tests | ✓ | 3.8.1 (CPM) | — |
| ctest | TEST-09 CI | ✓ | system | Run `Tests` binary directly |
| Clang ≥20 + RTSan | Optional alloc audit | ✗ (not verified) | — | Static token scan + stress tests (primary gate) |
| pluginval | TEST-12 (later phase) | ✓ in CI | strictness 10 | Phase 14 focuses on TEST-09 |

**Missing dependencies with no fallback:**
- None blocking Phase 14 execution.

**Missing dependencies with fallback:**
- RTSan toolchain — static scan + 10k stress tests per Phase 13 precedent.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (Catch.cmake discovery) |
| Quick run command | `ctest --test-dir build -R "RealtimeStress|TEST-09|GatedBloomChain" --output-on-failure` |
| Full suite command | `ctest --test-dir build --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| INTEG-01 | `IReverbEngine::processBlock` routes fixed-rate path | unit/integration | `ctest -R "ReverbEngine|SchroederTank32"` | ✅ partial — extend for `processBlock` |
| INTEG-02 | Block reverb + per-sample gate/OD | unit | `ctest -R "GatedBloomChain"` | ✅ — add block case |
| INTEG-03 | v1 gate/send/OD/dry unchanged | regression | `ctest -R "GatedBloomChain|DryPath|PostGate|send release"` | ✅ |
| INTEG-04 | No new APVTS params; color off default | static + preset | `ctest -R "RequirementsTraceability|PluginBasics"` | ✅ |
| TEST-09 | No heap alloc in integrated `processBlock` | static + stress | `ctest -R "allocation tokens|10k varying block stress|authentic color toggling"` | ✅ partial — extend |

### Sampling Rate

- **Per task commit:** `ctest -R "GatedBloomChain|allocation tokens" --output-on-failure`
- **Per wave merge:** `ctest --test-dir build --output-on-failure`
- **Phase gate:** Full suite green + TEST-09 cases with `authentic_color=1` before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `tests/BlockIntegrationTest.cpp` (optional) — `GatedBloomChain::processBlock` with `authentic_color=1` finite output
- [ ] Extend `tests/RealtimeStressTest.cpp` — TEST-09 tag; 10k blocks with `authentic_color=1` as primary stress
- [ ] Static alloc scan covering `GatedBloomChain.h` + `SchroederTank32.h` `processBlock` bodies
- [ ] `SchroederTank32::processBlock` unit test — `authenticColor=false` matches sample loop; `true` delegates to adapter

## v1.0 Parity Strategy

**Scope of parity (INTEG-03):** Gate profiles, pressure send curves, wet overdrive blend, dry tap unity, post-gate keying from input envelope, send release tank trail — **unchanged**. Only the reverb engine call behind the gate/send changes when `authentic_color=1`.

**Verification tiers:**

| Tier | `authentic_color` | Tests | Expectation |
|------|-------------------|-------|-------------|
| 1 — RC1 primary | 0 (default) | `GatedBloomChainTest`, `DryPathIntegrityTest`, `DryNeverGatedTest`, `PostGateTimingTest`, `WetOverdriveDiagnosticsTest`, `RealtimeStressTest` (existing) | All pass unchanged; host-rate `processSample` path preserved |
| 2 — Plugin integration | 0 | `ReleaseTruthTest` `renderHostRateChain`, full plugin impulse renders | Wet chain RMS/timing within existing tolerances |
| 3 — Color on (non-parity) | 1 | `RealtimeStressTest` authentic cases, new block integration test | Finite output, no alloc; timbre vs accumulator **not** parity target (ProperSRC is intentional change) |

**Implementation rule:** Extract current `GatedBloomChain::processSample` body into the `!authenticColor` branch of `processBlock` verbatim — do not refactor gate/send/OD logic.

**Factory presets:** `authentic_color=0` per SAFE-01/02 — no preset changes in Phase 14.

## TEST-09 Stress Test Approach

**Requirement text:** "Fixed32SRC does not allocate in `processBlock()`" [VERIFIED: `.planning/REQUIREMENTS.md` line 64].

**Three-layer gate (match Phase 13 SRC-02 pattern):**

1. **Static source scan** — Extend `FixedRateAdapterTest.cpp` pattern to `GatedBloomChain.h` and `SchroederTank32.h` `processBlock` bodies; forbid `resize(`, `make_unique`, `push_back`, `emplace_back` inside realtime bodies [VERIFIED: `tests/FixedRateAdapterTest.cpp:614-671`].

2. **Adapter-level stress (regression)** — Keep existing `FixedRateAdapter ProperSRC realtime stress with random block sizes` (10k iterations, block 1–512) as Phase 13 safety net.

3. **Plugin-level stress (TEST-09 primary)** — Extend `RealtimeStressTest.cpp`:
   - Promote / duplicate 10k varying-block case with `authentic_color=1` and tag `[TEST-09]`
   - Assert all samples finite and `peak < 4.0f` (existing bound)
   - Keep `authentic color toggling` case (2000 blocks) for state stress
   - Add `larger-than-prepared block` with `authentic_color=1`

4. **Optional RTSan build** — `-DWITH_REALTIME_SANITIZER=ON` with Homebrew Clang; mark `[[clang::nonblocking]]` on `GatedBloomChain::processBlock` and `PluginProcessor::processBlock` [VERIFIED: `cmake/Sanitizers.cmake`] — not required for phase pass if layers 1–3 green.

**Block size pattern (reuse):**

```cpp
constexpr std::array<int, 8> kBlockSizes { 32, 64, 128, 256, 512, 1024, 256, 128 };
// 10'000 iterations: block = kBlockSizes[block % 8]
```

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | N/A — offline audio plugin |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | yes | `juce::jlimit` on DSP outputs; `n <= maxBlockSize` guards |
| V6 Cryptography | no | N/A |

### Known Threat Patterns for {stack}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Buffer overrun on variable block | Tampering | `maxBlockSize` guards in chain/adapter |
| Denormal CPU load | DoS | `ScopedNoDenormals` already in `PluginProcessor` [VERIFIED: line 165] |
| Non-realtime alloc on audio thread | DoS | TEST-09 static + stress gates |

## Sources

### Primary (HIGH confidence)
- `source/IReverbEngine.h`, `source/GatedBloomChain.h`, `source/FixedRateAdapter.h`, `source/SchroederTank32.h`, `source/PluginProcessor.cpp` — current integration points
- `.planning/phases/13-fixed-rate-adapter-r8brain/13-VERIFICATION.md` — Phase 13 alloc-free ProperSRC evidence
- `tests/RealtimeStressTest.cpp`, `tests/FixedRateAdapterTest.cpp` — stress and static scan patterns

### Secondary (MEDIUM confidence)
- `.planning/REQUIREMENTS.md` INTEG-01–04, TEST-09 — phase contract
- `.planning/ROADMAP.md` Phase 14 success criteria

### Tertiary (LOW confidence)
- None requiring validation — codebase-primary phase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new deps; Phase 13 building blocks verified
- Architecture: HIGH — explicit INTEG requirements + existing class boundaries
- Pitfalls: HIGH — Phase 13 documented FIFO/alloc pitfalls; v1 tests define parity

**Research date:** 2026-07-08
**Valid until:** 2026-08-07 (stable JUCE/C++ integration; r8brain pin fixed)
