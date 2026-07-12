# Phase 3: Ugly Signature Chain - Research

**Researched:** 2026-07-06
**Domain:** Parallel gated wet-path routing, placeholder DSP stubs, Catch2 routing proof tests (JUCE 8 / C++20)
**Confidence:** HIGH

## Summary

Phase 3 replaces Phase 2 `DummyDspHooks` with a **complete but crude** parallel signal chain that proves SendBloom's product behavior before any DSP polish. The routing topology is locked: **clean dry tap at unity** runs parallel to a **gated wet path** (gate stub → pressure-send stub → placeholder reverb → tanh dirt) summed via `ParallelWetMixer` where the **level knob scales wet return only** (sin equal-power curve already in `ParameterCurves::levelEqualPower`) [VERIFIED: source/ParameterCurves.h].

The wet-path order follows planning corpus and CONTEXT: **input → [PreSoft gate stub if gate_pre] → pressure-send multiply → placeholder feedback-delay reverb → tanh dirt → [PostHard gate stub if gate_post, keyed from input detector] → wet return** [CITED: RESEARCH_CORPUS.md R4 routing; 03-CONTEXT.md locked topology]. Post gate MUST use an envelope follower on the **input signal**, not the wet tail — this is CHN-03 and the signature "edited sample" chop [CITED: REQUIREMENTS.md CHN-03; RESEARCH_CORPUS.md R3].

Implementation builds on Phase 2 assets: `ParameterSnapshot`, `SmoothedParameterBank`, `BypassCrossfade`, preallocated `dryBuffer`, and per-sample smoother pattern in `PluginProcessor::processBlock` [VERIFIED: source/PluginProcessor.cpp]. No new external packages; all DSP uses JUCE `juce_dsp` (`DelayLine`, optional reference to `DryWetMixer` sin3dB math) already linked in CMake [VERIFIED: CMakeLists.txt].

Tone quality is explicitly irrelevant — placeholder reverb is a **single-channel feedback delay** (not SchroederTank32), gate stubs are crude threshold multipliers (not BYOD FSM yet), and dirt is fixed tanh on wet only. Phase 4–7 swap real modules without changing proven routing.

**Primary recommendation:** Add `GatedBloomChain` (header-only or `.h/.cpp`), `ParallelWetMixer`, four stub classes (`StubInputEnvelope`, `StubNoiseGate`, `PlaceholderReverb`, `PlaceholderWetDirt`, `StubPressureSend`), wire into `PluginProcessor::processBlock`, delete `DummyDspHooks`, and prove routing with Catch2 tests for dry-clean, wet-only-dirt, input-keyed chop, and send-trail persistence before DAW smoke.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Routing Topology (locked by roadmap)
- Parallel: clean dry tap + gated wet path (pre/post gate stubs → pressure send stub → placeholder reverb → tanh dirt → mix)
- Dry guitar clean at unity; wet-only dirt when distn raised
- Post gate keyed from input detector, not wet tail
- Pressure-send release to zero does not clear reverb tank

#### Placeholder DSP
- Placeholder reverb: simple feedback delay or crude tank — not SchroederTank32 (Phase 5)
- Placeholder dirt: fixed tanh on wet path only
- Gate stubs: PreSoft/PostHard behavior approximated crudely for audible chop

### Claude's Discretion
Exact stub algorithms, buffer sizes, ParallelWetMixer implementation, and unit test fixtures at planner discretion within architecture routing pseudocode from planning corpus.

### Deferred Ideas (OUT OF SCOPE)
- Real InputStage/OutputStage (Phase 4)
- SchroederTank32 reverb (Phase 5)
- Real WetOverdrive (Phase 6)
- Real PressureSend (Phase 7)
</user_constraints>

## Project Constraints (from .cursor/rules/)

From `.claude/.cursor/rules/gsd-project.md` — planner must verify compliance:

- **Tech stack:** JUCE 8, C++20, CMake, Catch2, pluginval — pamplejuce-style repository pattern
- **Realtime safety:** Zero heap allocation, locks, logging, file I/O, or UI access in `processBlock()` after `prepare()`
- **License:** Personal/portfolio — prefer MIT-compatible references only; no GPL forks as base
- **Legal:** Clean-room implementation; no trademarked names in metadata
- **Audio contract:** Mono-authentic processing; stereo buses sum to mono internally unless Extended stereo enabled; zero reported plugin latency in v1
- **Parameter stability:** APVTS IDs immutable after release; all continuous params smoothed per architecture spec
- **GSD workflow:** Phase work via GSD commands; do not bypass planning artifacts

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CHN-01 | `GatedBloomChain` wires full topology per architecture pseudocode | Wet-path order diagram below; stub class contracts; integration replaces `DummyDspHooks` in `processBlock` |
| CHN-02 | `ParallelWetMixer` keeps dry at unity; level controls wet return via sin curve | `ParameterCurves::levelEqualPower` + parallel topology (dry=1.0, wet×sin); distinct from full DryWetMixer dual-scaling [CITED: juce_DryWetMixer.cpp sin3dB] |
| CHN-03 | Post gate keyed from input detector, not wet tail (edited-sample behavior) | `StubInputEnvelope` on input sidechain; post-gate gain from input envelope only; routing test with burst→silence while reverb tail active |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Dry tap (unity parallel path) | API / Backend (audio thread) | — | Copied from input before wet processing; never gated or distorted |
| Wet path stub chain | API / Backend (audio thread) | — | All FX routing in `GatedBloomChain::processSample` or block processor |
| Input envelope (gate sidechain) | API / Backend (audio thread) | — | Runs on gained input; drives gate stubs, not wet self-detection |
| ParallelWetMixer | API / Backend (audio thread) | — | Sums dry tap + scaled wet return per sample |
| ParameterSnapshot / smoothers | API / Backend (audio thread) | — | Block-rate capture; per-sample `SmoothedParameterBank` (Phase 2) |
| Bypass crossfade | API / Backend (audio thread) | Host bypass API | Wraps final mixed output (Phase 2 `BypassCrossfade`) |
| Routing proof unit tests | Build / CI | — | Catch2 offline render of chain classes; no DAW required |
| Real InputStage / gates / reverb | — | Phase 4–7 | Out of scope; stubs only |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce_dsp` | 8.x (submodule) | `DelayLine`, future `DryWetMixer` reference | Already linked [VERIFIED: CMakeLists.txt] |
| JUCE `juce_audio_basics` | 8.x | `SmoothedValue`, `Decibels`, `AudioBuffer` | Phase 1–2 foundation |
| JUCE `juce_audio_processors` | 8.x | APVTS integration | Phase 2 complete |
| Catch2 | 3.8.1 (CPM) | Routing proof unit tests | Phase 1 harness [VERIFIED: cmake/Tests.cmake] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce::dsp::DelayLine<float>` | JUCE built-in | Placeholder reverb feedback loop | `PlaceholderReverb` tank buffer |
| `ParameterCurves::levelEqualPower` | project | Wet return gain from level knob | `ParallelWetMixer` wet scale |
| `ParameterCurves::sizeToRT60` | project | Map size → decay for feedback gain | Placeholder reverb feedback coefficient |
| `ParameterCurves::distnBlend` | project | Wet dirt blend pow(2.8) | `PlaceholderWetDirt` |
| BYOD `Gate.cpp` pattern (study only) | repo-sample | Attack/Hold/Release FSM reference | Phase 4 real gate; Phase 3 uses simplified stub |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom `ParallelWetMixer` | `juce::dsp::DryWetMixer` sin3dB | DryWetMixer scales **both** dry and wet; CHN-02 requires dry at **unity** always — custom mixer matches parallel pedal topology |
| `DelayLine` feedback stub | SchroederTank32 | Locked deferred to Phase 5; stub proves routing only |
| BYOD Gate FSM fork | Simplified threshold×envelope stub | GPL base forbidden; stub sufficient for audible chop proof |
| Per-block chain processing | Per-sample `processSample` | Per-sample matches Phase 2 smoother pattern; easier gate/reverb feedback; acceptable at Phase 3 block sizes |

**Installation:**

```bash
# No new packages — extend existing source/ and tests/
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --target Tests
ctest --test-dir Builds -R "chain|routing|ParallelWet" --output-on-failure
```

**Version verification:**

```bash
# JUCE submodule present at JUCE/ [VERIFIED: Phase 1 scaffold]
# Catch2 3.8.1 via CPMAddPackage in cmake/Tests.cmake [VERIFIED]
```

## Package Legitimacy Audit

> Phase installs no new external packages. All DSP from JUCE submodule and project headers.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| juce-framework/JUCE | GitHub (submodule) | mature | N/A | github.com/juce-framework/JUCE | OK | Approved — Phase 1 |
| catchorg/Catch2 | GitHub (CPM) | mature | N/A | github.com/catchorg/Catch2 | OK | Approved — Phase 1 |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
┌──────────── Host / DAW ────────────────────────────────────────────────────┐
│  APVTS params (level, distn, size, send, gate_pre_post, …)                │
└───────────────────────────────┬────────────────────────────────────────────┘
                                │ ParameterSnapshot::capture (once/block)
                                ▼
┌────────────── processBlock() — audio thread ─────────────────────────────────┐
│                                                                            │
│  input buffer ──┬──► copy ──► dryTapBuffer (unity, never distorted)       │
│                 │                                                          │
│                 └──► × inputGain ──► mono sum ──► inputEnvelope.follow()  │
│                              │                                             │
│                              ▼                                             │
│                    ┌── GatedBloomChain ──────────────────────────────┐    │
│                    │  if gatePreSoft:  × preGateStub(inputEnv)       │    │
│                    │  × pressureSendStub (sendGain; no tank reset)   │    │
│                    │  → placeholderReverb (DelayLine feedback)       │    │
│                    │  → placeholderWetDirt (tanh, distnBlend)        │    │
│                    │  if !gatePreSoft:  × postGateStub(inputEnv)     │    │
│                    └─────────────────────────────────────────────────┘    │
│                              │ wet sample                                  │
│                              ▼                                             │
│                    ParallelWetMixer: out = dryTap + wet × levelWetGain    │
│                              │                                             │
│                              ▼                                             │
│                    BypassCrossfade (5 ms) + outputGain                     │
└────────────────────────────────────────────────────────────────────────────┘
```

**Gate placement (locked):** `gate_pre_post` APVTS choice selects **one** active gate position on wet path only [VERIFIED: ParameterSnapshot.h `gatePreSoft`]. PreSoft: gate before send/reverb. PostHard: gate after reverb+dirt; envelope still from **input**, not wet [CITED: REQUIREMENTS GATE-03, CHN-03].

### Recommended Project Structure

```
source/
├── GatedBloomChain.h          # Orchestrates wet-path stub order; owns stub state
├── ParallelWetMixer.h         # dry unity + wet × sin(level); header-only
├── StubInputEnvelope.h        # Peak follower on input for gate sidechain
├── StubNoiseGate.h            # Crude pre/post profiles (threshold × smoothed gain)
├── StubPressureSend.h         # Multiplies wet feed; never clears reverb on release
├── PlaceholderReverb.h        # DelayLine feedback; size→RT60 maps feedback gain
├── PlaceholderWetDirt.h       # Fixed tanh wet-only; distnBlend mix
├── PluginProcessor.h/.cpp     # Replace DummyDspHooks with GatedBloomChain
└── (Phase 2 files unchanged)

tests/
├── ParallelWetMixerTest.cpp   # CHN-02 sin wet scaling; dry unchanged
├── GatedBloomChainTest.cpp    # CHN-01/03 routing proofs
└── ChainTestHelpers.h         # Offline render, burst/silence fixtures
```

### Pattern 1: GatedBloomChain (wet-path orchestrator)

**What:** Single class owning stub state prepared in `prepareToPlay`; `processSample` runs locked topology per sample.

**When to use:** CHN-01; replaces `DummyDspHooks::processSample`.

**Example:**

```cpp
// Source: RESEARCH_CORPUS R4 routing + 03-CONTEXT locked order
struct GatedBloomChain
{
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        reverb.prepare (sampleRate);
        envelope.prepare (sampleRate);
        preGate.prepare (sampleRate, /* releaseMs */ 150.0f);
        postGate.prepare (sampleRate, /* releaseMs */ 7.0f);
    }

    float processSample (float input,
                         float inputEnvelope,
                         float sizeNorm,
                         float rt60Seconds,
                         float distnBlend,
                         float sendGain,
                         bool gatePreSoft,
                         float thresholdDb) noexcept
    {
        auto wet = input;

        if (gatePreSoft)
            wet *= preGate.process (inputEnvelope, thresholdDb);

        wet *= sendGain; // StubPressureSend: scales feed only; tank untouched when send→0

        wet = reverb.processSample (wet, rt60Seconds);

        wet = PlaceholderWetDirt::process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= postGate.process (inputEnvelope, thresholdDb); // keyed from input, not wet

        return wet;
    }

    PlaceholderReverb reverb;
    StubInputEnvelope envelope;
    StubNoiseGate preGate, postGate;
};
```

### Pattern 2: ParallelWetMixer (parallel topology, not DryWetMixer)

**What:** `out = dryTap + wetProcessed × wetGain` where `wetGain = sin(π/2 × levelNorm)` from smoothed bank; **dry tap stays at unity** regardless of level knob.

**When to use:** CHN-02; distinct from `juce::dsp::DryWetMixer` which applies sin3dB to **both** paths [CITED: juce_DryWetMixer.cpp update()].

**Example:**

```cpp
// Source: ParameterCurves.h + CHN-02 requirement
struct ParallelWetMixer
{
    static float mix (float dryTap, float wetSample, float wetGain) noexcept
    {
        return dryTap + wetSample * wetGain; // dry unity; level scales wet return only
    }
};
```

### Pattern 3: PlaceholderReverb (feedback DelayLine)

**What:** Single `juce::dsp::DelayLine<float>` with feedback; `prepare()` in `prepareToPlay` only; per-sample push/pop feedback loop.

**When to use:** Audible bloom/decay proof before SchroederTank32.

**Example:**

```cpp
// Source: JUCE juce_DelayLine.h + forum feedback pattern [CITED: docs.juce.com DelayLine]
float PlaceholderReverb::processSample (float input, float rt60Seconds) noexcept
{
    const auto delayed = delayLine.popSample (0);
    const auto feedback = rt60ToFeedback (rt60Seconds, sampleRate, delaySamples);
    delayLine.pushSample (0, input + delayed * feedback);
    return delayed; // wet output from tank
}

// CRITICAL: no reset() when sendGain→0 — tank decays naturally (CHN-03 send trail stub)
```

Map `rt60Seconds` from `ParameterSnapshot` (`ParameterCurves::sizeToRT60`) to feedback gain `[ASSUMED]` via `feedback = exp(-6.9078 * delayTime / rt60)` for −60 dB decay.

### Pattern 4: StubInputEnvelope + StubNoiseGate (input-keyed chop)

**What:** One-pole peak follower on **gained input** drives gate gain; post gate never inspects wet amplitude.

**When to use:** CHN-03; Phase 4 replaces with real `EnvelopeDetector` + `NoiseGate`.

**Example:**

```cpp
// Source: BYOD Gate.cpp study pattern (simplified) [CITED: repo-samples/BYOD Gate.cpp]
float StubNoiseGate::process (float inputEnvelope, float thresholdDb) noexcept
{
    const auto thresh = juce::Decibels::decibelsToGain (thresholdDb);
    const auto target = inputEnvelope > thresh ? 1.0f : 0.0f;
    gain += (target - gain) * releaseCoeff; // short release for PostHard (~7 ms)
    return gain;
}
```

PreSoft profile: longer release (~150 ms), soft floor ~0.1 instead of hard 0 `[ASSUMED]` for audible hum suppression stub.

### Pattern 5: PlaceholderWetDirt (wet-only tanh)

**What:** Fixed asymmetric tanh; blend via `distnBlend` already smoothed in bank.

**Example:**

```cpp
// Source: RESEARCH_CORPUS R4 + DummyDspHooks.h precedent
static float process (float wet, float distnBlend) noexcept
{
    const auto driven = std::tanh (wet * 3.0f);
    return wet + distnBlend * (driven - wet);
}
```

Dry tap never passes through this function — satisfies wet-only dirt requirement.

### Pattern 6: PluginProcessor integration

**What:** Replace inner loop dummy call with chain + parallel mixer; keep bypass and output gain.

**Example:**

```cpp
// Adapted from source/PluginProcessor.cpp Phase 2 pattern
for (int sample = 0; sample < numSamples; ++sample)
{
    const auto wetGain = smoothedBank.getNextLevelWetGain();
    const auto distnBlend = smoothedBank.getNextDistnBlend();
    const auto sendGain = smoothedBank.getNextSendGain();
    // ... other smoothers

    for (int channel = 0; channel < numChannels; ++channel)
    {
        const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
        auto monoIn = dryBuffer.getReadPointer (channel)[sample] * inputGain;
        const auto env = chain.getEnvelope().process (std::abs (monoIn));

        const auto wet = chain.processSample (monoIn, env, sizeNorm, rt60, distnBlend,
                                              sendGain, gatePreSoft, thresholdDb);
        auto out = ParallelWetMixer::mix (dryTap, wet, wetGain);
        buffer.getWritePointer (channel)[sample] = out * outputGain * bypassMix;
    }
}
```

Stereo note: Phase 3 may process per-channel independently for ugly proof `[ASSUMED]`; mono sum contract hardens in Phase 4 (IO-03).

### Anti-Patterns to Avoid

- **Post gate keyed from wet tail:** Violates CHN-03; use input envelope only — wet reverb tail must not hold gate open.
- **Clearing reverb buffer when send→0:** Violates locked CONTEXT; only scale input feed to tank.
- **Applying distn/level to dry tap:** Violates core product value; dry stays clean at distn=1.
- **Using DryWetMixer for parallel topology:** Scales dry down as wet rises; wrong for parallel pedal routing.
- **Heap in processBlock:** No `DelayLine` prepare, no buffer resize — all in `prepareToPlay`.
- **Forking BYOD Gate.cpp:** GPL; study FSM only, implement greenfield stub.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Delay buffer for reverb stub | Custom circular buffer | `juce::dsp::DelayLine<float>` | Tested push/pop/feedback; prepare allocates once [CITED: juce_DelayLine.h] |
| Equal-power wet gain math | Ad-hoc sqrt | `ParameterCurves::levelEqualPower` / sin3dB | Matches JUCE DryWetMixer validation [CITED: juce_DryWetMixer.cpp] |
| Parameter smoothing | Custom ramps | `SmoothedParameterBank` (Phase 2) | Already wired; extend only if new targets needed |
| Gate envelope follower (Phase 4) | Full BYOD port | Minimal `StubInputEnvelope` one-pole | Phase 3 scope is routing proof, not gate correctness |
| Schroeder tank | Partial FV-1 clone | Feedback delay stub | Phase 5 locked replacement |

**Key insight:** Phase 3 value is **routing audibility**, not DSP quality. Stubs must be ugly but topologically correct so Phase 4–7 are swap-in replacements behind stable interfaces.

## Common Pitfalls

### Pitfall 1: Level knob attenuates dry path

**What goes wrong:** User raises level expecting more wet; dry guitar gets quieter — wrong pedal behavior.

**Why it happens:** Reusing `DryWetMixer` or applying `dryGain` from equal-power curve to dry tap.

**How to avoid:** `ParallelWetMixer`: dry always 1.0; only multiply wet by `wetGain` [LOCKED: CHN-02].

**Warning signs:** Dry-tap test fails at level=1.0; THD changes on dry when level moves.

### Pitfall 2: Post gate follows wet envelope

**What goes wrong:** Reverb tail keeps gate open after player stops; no "edited sample" chop.

**Why it happens:** Envelope detector connected to post-reverb signal.

**How to avoid:** Single `StubInputEnvelope` on input; pass `inputEnvelope` to post gate [LOCKED: CHN-03].

**Warning signs:** Routing test burst→silence shows wet tail > −20 dB for >100 ms after input stops.

### Pitfall 3: Send release clears reverb tank

**What goes wrong:** Momentary send feels wrong; tail cut abruptly when send pad released.

**Why it happens:** Calling `reverb.reset()` or zeroing delay line when `sendGain→0`.

**How to avoid:** `StubPressureSend` only multiplies **input feed** to reverb; tank state persists [LOCKED: CONTEXT].

**Warning signs:** Send trail test shows energy drop >60 dB within one block of send=0.

### Pitfall 4: DummyDspHooks left partially wired

**What goes wrong:** Double processing or params affecting tone through old tanh/lowpass path.

**Why it happens:** Incremental migration without removing dummy.

**How to avoid:** Delete `DummyDspHooks.h` and tests; single chain path in processor.

**Warning signs:** `grep DummyDspHooks source/` non-empty after Phase 3.

### Pitfall 5: Feedback delay instability

**What goes wrong:** Placeholder reverb self-oscillates or goes silent.

**Why it happens:** Feedback gain ≥1.0 or delay=0 samples.

**How to avoid:** Clamp feedback to ~0.95 max; minimum delay ≥100 samples; map RT60 conservatively `[ASSUMED]`.

**Warning signs:** NaN/inf in buffer; runaway RMS in offline render.

## Code Examples

### Feedback delay placeholder reverb

```cpp
// Source: JUCE docs.juce.com DelayLine + forum feedback pattern
void PlaceholderReverb::prepare (double rate)
{
    sampleRate = rate;
    constexpr int maxDelaySamples = 48000; // ~1 s at 48 kHz
    delayLine.setMaximumDelayInSamples (maxDelaySamples);
    delayLine.setDelay (static_cast<float> (delaySamples));
    juce::dsp::ProcessSpec spec { rate, 512, 1 };
    delayLine.prepare (spec);
}

float PlaceholderReverb::processSample (float input, float rt60Seconds) noexcept
{
    const auto fb = juce::jlimit (0.0f, 0.95f,
        std::exp (-6.9078f * delaySeconds / juce::jmax (0.05f, rt60Seconds)));
    const auto delayed = delayLine.popSample (0);
    delayLine.pushSample (0, input + delayed * fb);
    return delayed;
}
```

### Routing test fixture (burst → silence)

```cpp
// Source: RESEARCH_CORPUS R7 test patterns
static float renderPostGateTail (GatedBloomChain& chain, int silenceSamples)
{
    float tailRms = 0.0f;
    // 480 samples burst at -6 dBFS
    for (int i = 0; i < 480; ++i)
    {
        const auto env = chain.getEnvelope().process (0.5f);
        chain.processSample (0.5f, env, 0.5f, 1.0f, 0.0f, 1.0f, false, -40.0f);
    }
    // silence — input zero; post gate should close even if tail energy remains briefly
    for (int i = 0; i < silenceSamples; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        const auto wet = chain.processSample (0.0f, env, 0.5f, 1.0f, 0.0f, 1.0f, false, -40.0f);
        tailRms += wet * wet;
    }
    return std::sqrt (tailRms / static_cast<float> (silenceSamples));
}
```

### Dry integrity check (wet-only dirt)

```cpp
TEST_CASE ("Dry tap unchanged when distn max", "[chain][routing]")
{
    const auto dryIn = 0.25f;
    const auto wetOnly = ParallelWetMixer::mix (dryIn, /*wet*/0.0f, /*wetGain*/0.0f);
    REQUIRE (wetOnly == Catch::Approx (dryIn)); // no dirt on dry path
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| DummyDspHooks tanh/lowpass | Full GatedBloomChain stubs | Phase 3 | Audible routing proof replaces param preview |
| Serial dry/wet (DryWetMixer) | Parallel dry tap + wet return | Product spec | Dry stays clean at all level settings |
| Gate after reverb (reverb_trickery) | Gate pre AND post options; post keyed from input | SendBloom spec | "Edited sample" chop vs sustain tail |

**Deprecated/outdated:**
- `DummyDspHooks.h` — remove after chain wired.
- Applying `wetLevel` inside dummy tanh — move to `ParallelWetMixer` only.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Wet-path order: preGate? → send → reverb → dirt → postGate? | Pattern 1 | Wrong product feel; rework chain |
| A2 | Feedback delay ~200–400 ms; feedback from RT60 mapping | Pattern 3 | Decay too fast/slow for audible proof |
| A3 | PreSoft stub floor ~−20 dB not −80 dB (crude) | Pattern 4 | Pre gate inaudible in Phase 3 |
| A4 | PostHard stub release ~7 ms | Pattern 4 | Chop too slow for "edited sample" feel |
| A5 | Stereo: per-channel wet processing for ugly phase | Pattern 6 | Phase 4 mono sum may change timbre slightly |
| A6 | Send trail test threshold ≥200 ms energy for Phase 3 ugly stub (500 ms in Phase 7) | Tests | Phase gate criteria mismatch |
| A7 | `SendBloom_engineering_architecture.md` still absent | — | Interface names may differ from final spec |

## Open Questions

1. **Mono sum point for stereo input**
   - What we know: IO-03 deferred to Phase 4; authentic mono-first.
   - What's unclear: Sum before or after input gain in Phase 3 ugly chain.
   - Recommendation: Sum stereo to mono for wet path at chain input; dual-mono output `[ASSUMED]`; document in plan.

2. **Gate toggle: one gate vs dual gates**
   - What we know: GATE-04 says toggle selects placement; CROSS_AGENT mentions two instances.
   - What's unclear: Whether both exist simultaneously with one bypassed.
   - Recommendation: Single active gate per `gate_pre_post` choice [VERIFIED: ParameterSnapshot `gatePreSoft`]; matches Phase 4 plan.

3. **Dark mode in ugly chain**
   - What we know: `darkMode` smoothed in bank; dummy applied HF rolloff.
   - What's unclear: Whether Phase 3 stub needs dark audibility.
   - Recommendation: Optional one-pole darker feedback in `PlaceholderReverb` when `darkTarget>0.5` — planner discretion.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build | ✓ | 4.3.3 | — |
| Ninja | Local build | ✓ (homebrew typical) | — | Unix Makefiles |
| Catch2 | Routing tests | ✓ (CPM) | 3.8.1 | — |
| JUCE 8 submodule | DelayLine, APVTS | ✓ | submodule | — |
| ctest | Test discovery | ✓ | homebrew | Run `./Tests` directly |
| pluginval | Phase gate | ✓ (Phase 1 CI) | strictness 5 | — |

**Missing dependencies with no fallback:** none

**Missing dependencies with fallback:** none

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` |
| Quick run command | `ctest --test-dir Builds -R "chain|routing|ParallelWet" --output-on-failure` |
| Full suite command | `ctest --test-dir Builds --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CHN-01 | Full topology wired; wet path produces reverb+dirt | unit/integration | `ctest --test-dir Builds -R GatedBloomChain` | ❌ Wave 0 |
| CHN-02 | Dry unity; level scales wet only | unit | `ctest --test-dir Builds -R ParallelWetMixer` | ❌ Wave 0 |
| CHN-03 | Post gate keyed from input; chop on silence | unit | `ctest --test-dir Builds -R "input-keyed\|postGate"` | ❌ Wave 0 |
| — | Wet-only dirt (dry unchanged at distn=1) | unit | `ctest --test-dir Builds -R "dry.*clean\|wet-only"` | ❌ Wave 0 |
| — | Send release preserves tank energy | unit | `ctest --test-dir Builds -R "send.*trail\|tank"` | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `ctest --test-dir Builds -R "chain|routing|ParallelWet" --output-on-failure`
- **Per wave merge:** `ctest --test-dir Builds --output-on-failure`
- **Phase gate:** Full suite green + pluginval 5 + ugly-chain DAW smoke (human verify)

### Wave 0 Gaps

- [ ] `source/GatedBloomChain.h` — CHN-01 orchestrator
- [ ] `source/ParallelWetMixer.h` — CHN-02
- [ ] `source/PlaceholderReverb.h` — DelayLine stub
- [ ] `source/StubNoiseGate.h` + `StubInputEnvelope.h` — CHN-03
- [ ] `source/StubPressureSend.h` — send multiply without tank reset
- [ ] `source/PlaceholderWetDirt.h` — tanh wet-only
- [ ] `tests/ParallelWetMixerTest.cpp` — dry unity, wet scaling
- [ ] `tests/GatedBloomChainTest.cpp` — dry clean, wet-only dirt, input-keyed chop, send trail
- [ ] `tests/ChainTestHelpers.h` — shared offline render utilities
- [ ] Remove `DummyDspHooks.h` + `tests/DummyDspHooksTest.cpp` after chain wired
- [ ] Update `PluginProcessor` to use `GatedBloomChain` member (replace `DummyDspState`)

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | N/A — audio effect |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | partial | Trust JUCE buffer bounds; clamp feedback gain and delay; no user file input in DSP |
| V6 Cryptography | no | N/A |

### Known Threat Patterns for JUCE audio plugin

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Buffer over-read/write | Tampering | Use `buffer.getNumSamples()` / channel bounds only |
| Denormal CPU burn | DoS | `ScopedNoDenormals` (already in processor) |
| Heap alloc on audio thread | DoS | prepare-time allocation only; fixed member state |
| NaN propagation from feedback | DoS | Clamp feedback `<0.95`; jlimit output samples `[ASSUMED]` |

## Sources

### Primary (HIGH confidence)

- `source/PluginProcessor.cpp`, `ParameterSnapshot.h`, `SmoothedParameterBank.h`, `ParameterCurves.h` — Phase 2 integration baseline [VERIFIED: workspace]
- `JUCE/modules/juce_dsp/processors/juce_DryWetMixer.cpp` — sin3dB math reference [VERIFIED: workspace]
- `JUCE/modules/juce_dsp/processors/juce_DelayLine.h` — feedback delay API [VERIFIED: workspace]
- `.planning/phases/03-ugly-signature-chain/03-CONTEXT.md` — locked routing decisions [VERIFIED: workspace]

### Secondary (MEDIUM confidence)

- `.planning/RESEARCH_CORPUS.md` R3/R4/R7 — gate routing, wet OD, test patterns [VERIFIED: workspace]
- `.planning/ROADMAP.md` Phase 3 success criteria [VERIFIED: workspace]
- `.planning/repo-samples/Chowdhury-DSP-BYOD/src/processors/other/Gate.cpp` — FSM study (not forked) [VERIFIED: workspace]
- JUCE DelayLine docs — https://docs.juce.com/master/classjuce_1_1dsp_1_1DelayLine.html [CITED: docs.juce.com]

### Tertiary (LOW confidence)

- JUCE forum delay feedback threads — push/pop ordering [ASSUMED: standard pattern]
- Post gate sidechain pattern — general DSP practice [ASSUMED: not independently verified beyond corpus]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — JUCE-only, Phase 2 foundation verified in repo
- Architecture: HIGH — locked in CONTEXT + REQUIREMENTS; codebase integration points confirmed
- Pitfalls: MEDIUM — stub tuning (release ms, feedback gain) is planner discretion

**Research date:** 2026-07-06
**Valid until:** 2026-08-06 (stable JUCE patterns; stub internals may adjust during planning)
