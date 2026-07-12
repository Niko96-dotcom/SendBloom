# Phase 2: Parameters & State - Research

**Researched:** 2026-07-06
**Domain:** JUCE 8 APVTS parameter system, ParameterSnapshot, SmoothedValue smoothing, bypass crossfade, curve mapping unit tests, dummy DSP automation hooks
**Confidence:** MEDIUM

## Summary

Phase 2 transforms the Phase 1 passthrough `PluginProcessor` into a fully parameterized, automation-ready plugin shell. The codebase today has no APVTS, no state persistence, and an empty `prepareToPlay` — all parameter infrastructure must be added. JUCE 8 patterns are well-established: `AudioProcessorValueTreeState` with a static `createParameterLayout()`, immutable `ParameterID` strings in `ParameterIDs.h`, state via `copyState()` / `replaceState()`, and realtime-safe reads via `getRawParameterValue()` once per `processBlock()` into a `ParameterSnapshot` struct.

The critical architectural constraint (locked in CONTEXT) is **no APVTS atomic loads inside DSP inner loops**. The snapshot pattern reads all raw values once at block start, applies curve mappings to derived fields (RT60, distn blend, equal-power wet/dry, send curves), then sets `SmoothedValue` targets. Per-sample smoothing uses `juce::SmoothedValue<float>` (linear for gains, multiplicative optional for dB-domain params) with ramp lengths configured in `prepareToPlay`. Bypass requires a **5 ms clickless crossfade** — implement with dual `SmoothedValue` wet/dry gains (or a dedicated `BypassCrossfade` helper), not an instant mute. Register bypass via `getBypassParameter()` so hosts integrate correctly [CITED: JUCE AudioProcessor.h].

`SendBloom_engineering_architecture.md` is referenced in PROJECT.md but **not present in the repo**. Parameter IDs, ranges, and smoothing times are derived from REQUIREMENTS.md (PARM-*), UI requirements (UI-01, UI-04), RESEARCH_CORPUS R8, BUILD_MICROSTEPS, and CROSS_AGENT_SYNTHESIS. Items not confirmed in planning artifacts are tagged `[ASSUMED]` in the Assumptions Log.

**Primary recommendation:** Add `ParameterIDs.h`, `ParameterLayout.cpp`, `ParameterSnapshot.h`, `ParameterCurves.h`, wire APVTS into `PluginProcessor`, implement per-parameter `SmoothedValue` bank + 5 ms bypass crossfade, expose minimal dummy gain/offset hooks driven by smoothed params, and verify all curve mappings with Catch2 tests in `tests/ParameterCurvesTest.cpp`.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Parameter Architecture
- All parameter IDs immutable in `ParameterIDs.h` per engineering architecture
- `ParameterSnapshot` created once per `processBlock()` — no APVTS atomics in DSP inner loops
- Bypass: 5 ms clickless crossfade
- Curve mappings: size→RT60, distn→pow(2.8), level→sin equal-power, send curves per architecture spec

#### Dummy DSP Hooks
- Placeholder processing responds to param changes so DAW automation is audibly verifiable before real DSP (Phase 3+)
- Hooks should be minimal gain/offset or logging-free audible indicators, not full effect chain

### Claude's Discretion

Implementation file layout, exact smoothing time constants from architecture doc, and dummy hook audibility strategy at planner's discretion within architecture constraints.

### Deferred Ideas (OUT OF SCOPE)

- Real DSP modules (gates, reverb, OD) — Phase 3+
- UI bindings — Phase 9
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
| PARM-01 | All APVTS parameter IDs defined in `ParameterIDs.h` per architecture table | Derived parameter table below (architecture doc absent — synthesized from REQUIREMENTS + planning corpus); `ParameterID{"id", 1}` versioning pattern [CITED: JUCE APVTS header] |
| PARM-02 | `createParameterLayout()` exposes all public params with correct ranges and defaults | `AudioProcessorValueTreeState::ParameterLayout` with `AudioParameterFloat/Bool/Choice` [CITED: JUCE APVTS]; PinkGuitarFX sample pattern in repo-samples |
| PARM-03 | `ParameterSnapshot` created once per `processBlock()`; no APVTS atomics in DSP inner loops | Snapshot struct populated from `getRawParameterValue()` at block start; curve mapping applied before smoothing targets set |
| PARM-04 | Per-parameter smoothing matches architecture spec (attack/release times) | `SmoothedValue::reset(sampleRate, seconds)` per param category; recommended ramp table below (architecture doc missing — planner discretion within CONTEXT) |
| PARM-05 | Curve mappings: size→RT60, distn→pow(2.8), level→sin equal-power, send curves | Pure functions in `ParameterCurves.h`; formulas from REQUIREMENTS + RESEARCH_CORPUS R8; Catch2 golden-value tests |
| PARM-06 | Bypass with 5 ms clickless crossfade | Dual `SmoothedValue` crossfade + `getBypassParameter()` [CITED: JUCE AudioProcessor.h]; 5 ms locked in CONTEXT |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| APVTS parameter definitions | API / Backend (processor) | Build (SharedCode) | Parameters owned by `AudioProcessor`; compiled into plugin binary |
| ParameterSnapshot + curve mapping | API / Backend (audio thread) | — | Runs at start of `processBlock()`; must be realtime-safe |
| SmoothedValue advancement | API / Backend (audio thread) | — | Per-sample `getNextValue()` in dummy DSP / bypass crossfade loops |
| Bypass crossfade | API / Backend (audio thread) | Host bypass API | `getBypassParameter()` integrates host bypass; internal smoother handles clicks |
| State save/restore (presets) | API / Backend | Host | `apvts.copyState()` / `replaceState()` in `get/setStateInformation` |
| Curve unit tests | Build / CI | — | Catch2 `Tests` target links SharedCode; no audio thread |
| Dummy DSP hooks | API / Backend (audio thread) | — | Audible automation verification before Phase 3 chain |
| UI parameter attachments | Browser / Client | — | **Deferred Phase 9** — out of scope |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce_audio_processors` | 8.x (submodule) | APVTS, parameter types, state XML | Already linked in CMake SharedCode [VERIFIED: CMakeLists.txt] |
| JUCE `juce_dsp` | 8.x (submodule) | `dsp::DryWetMixer`, `dsp::Gain` reference patterns | Already linked; `sin3dB` rule matches level curve [CITED: juce_DryWetMixer.cpp] |
| JUCE `juce_audio_basics` | 8.x | `SmoothedValue`, `NormalisableRange`, `Decibels` | Core smoothing primitive [CITED: juce_SmoothedValue.h] |
| Catch2 | 3.8.1 (CPM) | Curve mapping unit tests | Established Phase 1 test harness [VERIFIED: cmake/Tests.cmake] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `juce::SmoothedValue<float>` | JUCE built-in | Per-parameter linear ramps | Continuous knobs (in, size, level, distn, out, send) |
| `juce::SmoothedValue<float, ValueSmoothingTypes::Multiplicative>` | JUCE built-in | Log-domain ramps | Optional for dB gain params if zipper audible at linear |
| `juce::dsp::DryWetMixer<float>` | JUCE built-in | Equal-power wet/dry reference | Phase 3 `ParallelWetMixer`; Phase 2 tests validate `sin3dB` math matches `ParameterCurves` |
| `juce::AudioParameterBool` (bypass) | JUCE built-in | Host bypass integration | Return from `getBypassParameter()` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Custom `ParameterSnapshot` | chowdsp `SmoothedBufferValue` | Adds GPL-adjacent dependency risk; PROJECT.md prefers MIT path; JUCE primitives sufficient |
| `dsp::DryWetMixer` in Phase 2 | Inline sin/cos in `ParameterCurves` | Phase 2 only needs curve functions + tests; mixer wiring is Phase 3 |
| Host `processBlockBypassed` only | Internal bypass param + crossfade in `processBlock` | JUCE docs recommend checking bypass param in `processBlock` when `getBypassParameter()` implemented [CITED: AudioProcessor.h] — use single path with crossfade smoother |
| melatonin_parameters | Plain JUCE `NormalisableRange` | External dependency not in scaffold; skew via `NormalisableRange` suffices [CITED: RESEARCH_CORPUS R8] |

**Installation:**

```bash
# No new packages — JUCE + Catch2 already configured
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build Builds --target Tests
ctest --test-dir Builds -R ParameterCurves --output-on-failure
```

**Version verification:**

```bash
# Catch2 pin confirmed in cmake/Tests.cmake:
# CPMAddPackage("gh:catchorg/Catch2@3.8.1")
# JUCE: git submodule in JUCE/ (Phase 1 scaffold)
```

## Package Legitimacy Audit

> Phase installs no new external packages. Dependencies are existing JUCE submodule and CPM Catch2.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| catchorg/Catch2 | GitHub (CPM) | mature | N/A | github.com/catchorg/Catch2 | OK | Approved — Phase 1 |
| juce-framework/JUCE | GitHub (submodule) | mature | N/A | github.com/juce-framework/JUCE | OK | Approved — Phase 1 |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Derived Parameter Table (architecture doc absent)

> `SendBloom_engineering_architecture.md` not found in repo [VERIFIED: workspace grep]. Table synthesized from REQUIREMENTS, UI specs, RESEARCH_CORPUS R8, BUILD_MICROSTEPS, CROSS_AGENT_SYNTHESIS. Planner must lock defaults/ranges at implementation; `[ASSUMED]` rows need user confirmation.

| Parameter ID | UI / Role | Type | Range (normalized unless noted) | Default | APVTS notes | Smoothing ramp |
|--------------|-----------|------|----------------------------------|---------|-------------|----------------|
| `input_gain` | In knob (gain leg) | float | 0.0–1.0 | 0.5 `[ASSUMED]` | Linear; maps to +9…−3 dB via smoothstep [CITED: RESEARCH_CORPUS R8] | 20 ms `[ASSUMED]` |
| `input_threshold` | In knob (threshold leg) / Gate Sens | float | 0.0–1.0 | 0.35 `[ASSUMED]` | Skew for pow(x,1.6); maps −52…−18 dB [CITED: RESEARCH_CORPUS R8] | 50 ms `[ASSUMED]` |
| `size` | Size knob | float | 0.0–1.0 | 0.5 `[ASSUMED]` | `NormalisableRange` skew ≈0.5 for pow(x,2.4) feel [CITED: RESEARCH_CORPUS R8] | 50 ms `[ASSUMED]` |
| `level` | Lvl knob (wet return) | float | 0.0–1.0 | 0.5 `[ASSUMED]` | Linear storage; sin equal-power at DSP read | 20 ms `[ASSUMED]` |
| `distn` | Distn knob | float | 0.0–1.0 | 0.0 | Linear storage; pow(x,2.8) at DSP read [CITED: REQUIREMENTS OD-02] | 20 ms `[ASSUMED]` |
| `output_gain` | Out knob | float | −12.0…+12.0 dB | 0.0 dB | `NormalisableRange` linear dB | 20 ms `[ASSUMED]` |
| `dark_mode` | Dark toggle | bool | — | false (Bright) | `AudioParameterBool` | 15 ms crossfade target `[ASSUMED]` (matches MB-035 dark/bright) |
| `gate_pre_post` | Gate Pre/Post toggle | choice | PreSoft=0, PostHard=1 | PreSoft | `AudioParameterChoice` | Instant (enum) |
| `send_connected` | Pressure send insert mode | bool | — | false | When false, send gain forced to 1.0 [CITED: SEND-01] | Instant |
| `send_amount` | Pressure pad / CC1 | float | 0.0–1.0 | 1.0 `[ASSUMED]` | Linear storage; curve at DSP read | 25 ms [CITED: RESEARCH_CORPUS R5 send release] |
| `send_feel` | Send Feel (advanced) | choice | Firm=0, Soft=1 | Firm | Selects send exponent | Instant |
| `authentic_color` | 32k Color (advanced) | bool | — | true `[ASSUMED]` | Phase 2 stub only | Instant |
| `extended_stereo` | Extended Stereo (disabled v1) | bool | — | false | Exposed but no-op in Authentic | Instant |
| `dirt_os` | Dirt OS (disabled v1) | bool | — | false | Exposed but no-op in Authentic | Instant |
| `bypass` | Host bypass | bool | — | false | `getBypassParameter()`; 5 ms crossfade [LOCKED: CONTEXT] | 5 ms |

**Split input params:** CROSS_AGENT_SYNTHESIS mandates separate `input_gain` + `input_threshold` APVTS IDs (not one dual-purpose param). Single "In" knob UI in Phase 9 may write both; Phase 2 exposes both for automation.

## Architecture Patterns

### System Architecture Diagram

```
┌─────────────── Host / DAW ───────────────────────────────────────────┐
│  Automation / MIDI CC1 ──► APVTS atomic parameters (UI thread safe) │
└───────────────────────────────┬──────────────────────────────────────┘
                                │ getRawParameterValue() × N  (once)
                                ▼
┌─────────────────── processBlock() — audio thread ──────────────────────┐
│  1. ParameterSnapshot::capture(apvts)  ← curve mapping applied here    │
│  2. SmoothedValueBank::setTargets(snapshot)                          │
│  3. Copy input → dryBuffer (for bypass crossfade)                    │
│  4. DummyDspHooks::process(wetBuffer, smoothed values per-sample)    │
│  5. BypassCrossfade::mix(dryBuffer, wetBuffer, bypassSmoother)       │
│  6. Write output buffer                                              │
└──────────────────────────────────────────────────────────────────────┘
                                │
                                ▼
┌─────────────── get/setStateInformation ────────────────────────────────┐
│  apvts.copyState() → XML binary  |  replaceState() on load             │
└──────────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure

```
source/
├── ParameterIDs.h           # constexpr string IDs — immutable after ship
├── ParameterCurves.h          # Pure mapping functions (header-only, testable)
├── ParameterSnapshot.h        # Struct + capture() from APVTS
├── ParameterLayout.h/.cpp     # createParameterLayout() — ranges, defaults, skew
├── SmoothedParameterBank.h  # Owns SmoothedValue members; prepare/setTargets
├── BypassCrossfade.h          # 5 ms wet/dry crossfade helper
├── DummyDspHooks.h          # Minimal audible placeholder processing
├── PluginProcessor.h/.cpp     # APVTS member, wiring, state, processBlock
└── PluginEditor.h/.cpp        # Unchanged minimal editor (Phase 9 adds attachments)

tests/
├── ParameterCurvesTest.cpp  # PARM-05 / TEST-01 curve golden values
├── ParameterSnapshotTest.cpp # Optional: capture produces expected derived fields
└── PluginBasics.cpp         # Extend: APVTS state round-trip smoke
```

### Pattern 1: APVTS Constructor + Layout

**What:** Static `createParameterLayout()` returns `ParameterLayout` by value; processor initializer list constructs APVTS.

**When to use:** All JUCE 8 plugins with automatable parameters.

**Example:**

```cpp
// Source: JUCE/modules/juce_audio_processors/utilities/juce_AudioProcessorValueTreeState.h
class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor()
        : apvts (*this, nullptr, "SendBloomParams", createParameterLayout())
    {}

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
};
```

### Pattern 2: ParameterSnapshot (block-rate capture)

**What:** Read all atomics once, compute derived curve outputs, pass plain floats to smoothers.

**When to use:** Every `processBlock()` — mandatory per PARM-03.

**Example:**

```cpp
struct ParameterSnapshot
{
    float inputGainDb {};
    float inputThresholdDb {};
    float sizeNorm {};
    float rt60Seconds {};
    float levelNorm {};
    float wetGain {};
    float dryGain {};
    float distnBlend {};
    float outputGainLinear {};
    float sendGain {};
    bool  darkMode {};
    bool  gatePreSoft {};
    bool  sendConnected {};
    bool  bypassed {};

    static ParameterSnapshot capture (const juce::AudioProcessorValueTreeState& apvts)
    {
        ParameterSnapshot s;
        const auto inNorm = apvts.getRawParameterValue (IDs::inputGain)->load();
        s.inputGainDb = ParameterCurves::inputGainDb (inNorm);
        s.sizeNorm = apvts.getRawParameterValue (IDs::size)->load();
        s.rt60Seconds = ParameterCurves::sizeToRT60 (s.sizeNorm);
        const auto lvl = apvts.getRawParameterValue (IDs::level)->load();
        ParameterCurves::levelEqualPower (lvl, s.dryGain, s.wetGain);
        s.distnBlend = ParameterCurves::distnBlend (
            apvts.getRawParameterValue (IDs::distn)->load());
        // ... remaining params
        return s;
    }
};
```

### Pattern 3: Per-Parameter SmoothedValue Bank

**What:** One `SmoothedValue<float>` per continuous parameter; `prepareToPlay` sets ramps; `setTargets(snapshot)` per block; `getNextValue()` per sample in inner loops.

**When to use:** PARM-04; prevents zipper noise during automation.

**Example:**

```cpp
// Source: JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h
void SmoothedParameterBank::prepare (double sampleRate)
{
    for (auto* s : continuousSmoothers)
        s->reset (sampleRate, rampSeconds);
    bypassWetGain.reset (sampleRate, 0.005); // 5 ms — PARM-06
}

void SmoothedParameterBank::setTargets (const ParameterSnapshot& snap)
{
    levelWet.setTargetValue (snap.wetGain);
    // ...
    bypassWetGain.setTargetValue (snap.bypassed ? 0.0f : 1.0f);
}
```

### Pattern 4: Bypass Crossfade (5 ms)

**What:** Keep dry copy of input; process dummy wet path; crossfade per sample using smoothed wet gain (dry = 1 − wet).

**When to use:** PARM-06; also satisfies JUCE guidance to cross-fade wet/dry rather than hard-switch [CITED: AudioProcessor.h processBlockBypassed docs].

**Example:**

```cpp
void BypassCrossfade::processBlock (juce::AudioBuffer<float>& buffer,
                                  juce::AudioBuffer<float>& dryCopy,
                                  juce::SmoothedValue<float>& wetMix)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        auto* out = buffer.getWritePointer (ch);
        const auto* dry = dryCopy.getReadPointer (ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto wet = wetMix.getNextValue();
            const auto dryG = 1.0f - wet;
            out[i] = dry[i] * dryG + out[i] * wet;
        }
    }
}
```

### Pattern 5: Curve Functions (unit-testable)

**What:** Header-only pure functions; no JUCE processor dependency beyond `MathConstants`.

**Example:**

```cpp
// Source: REQUIREMENTS VERB-03, OD-02, RESEARCH_CORPUS R8, juce_DryWetMixer.cpp sin3dB
namespace ParameterCurves
{
    inline float sizeToRT60 (float sizeNorm) noexcept
    {
        return 0.25f + 5.75f * std::pow (sizeNorm, 2.4f);
    }

    inline float distnBlend (float norm) noexcept
    {
        return std::pow (norm, 2.8f);
    }

    inline void levelEqualPower (float norm, float& dry, float& wet) noexcept
    {
        using namespace juce::MathConstants<float>;
        dry = std::sin (halfPi * (1.0f - norm));
        wet = std::sin (halfPi * norm);
    }

    inline float smoothstep (float x) noexcept
    {
        return x * x * (3.0f - 2.0f * x);
    }

    inline float sendGain (float amount, bool firmFeel) noexcept
    {
        const auto s = smoothstep (amount);
        return firmFeel ? std::pow (s, 1.85f) : std::pow (s, 1.2f); // soft exp [ASSUMED]
    }
}
```

### Pattern 6: Dummy DSP Hooks

**What:** Minimal processing proving automation works audibly before Phase 3 chain.

**Recommended audibility strategy (planner discretion within CONTEXT):**
- `input_gain` → apply smoothed dB gain to entire buffer
- `output_gain` → final smoothed trim
- `size` → modulate a simple one-pole lowpass cutoff (audible tone change, not reverb)
- `distn` → blend buffer toward `tanh(buffer * 3)` by smoothed distn blend (wet-path preview)
- `level` → scale dummy wet contribution via smoothed wet gain
- `send_amount` / `send_connected` → multiply dummy wet by smoothed send gain
- `dark_mode` → toggle dummy HF rolloff target (15 ms smoothed)
- **No** logging, heap alloc, locks, or file I/O

### Anti-Patterns to Avoid

- **APVTS loads inside per-sample loops:** Violates PARM-03; causes cache contention and zipper noise. Read once into snapshot.
- **Instant bypass switching:** Causes clicks; violates PARM-06. Always ramp 5 ms.
- **Mutable parameter ID strings:** Use `constexpr` in `ParameterIDs.h`; changing IDs breaks saved sessions.
- **Skipping `getBypassParameter()`:** Hosts expect bypass integration; manual bool without override breaks VST3/AU bypass semantics.
- **PinkGuitarFX-style raw reads in inner loops:** Repo sample loads atomics per sample in `processBlock` — acceptable for demo, **not** for SendBloom (violates PROJECT.md realtime contract).

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Equal-power wet/dry panning | Custom sqrt/sin math from scratch | `ParameterCurves::levelEqualPower` mirroring `dsp::DryWetMixer::sin3dB` | JUCE already validates sin3dB math in unit tests [CITED: juce_DryWetMixer.cpp] |
| Parameter smoothing ramps | Custom exponential smoothing | `juce::SmoothedValue` | Battle-tested; `applyGain()` per-sample helpers |
| Plugin state XML | Custom binary preset format | `apvts.copyState()` / `replaceState()` | Host compatibility, preset round-trip (Phase 9) |
| Bypass click elimination | DC offset compensation hacks | 5 ms linear crossfade | Standard plugin practice [CITED: JUCE forum/docs] |
| RT60 mapping math | Ad-hoc exponential | `0.25 + 5.75×size^2.4` from REQUIREMENTS | Phase 5 reverb depends on identical function |

**Key insight:** ParameterSnapshot + SmoothedValue is the industry-standard JUCE pattern for realtime-safe automation; reinventing smoothing or bypass crossfade introduces clicks and session-breaking ID drift.

## Common Pitfalls

### Pitfall 1: Engineering architecture doc assumed present

**What goes wrong:** Parameter ranges/defaults differ from user expectation; preset compatibility breaks later.

**Why it happens:** PROJECT.md references `SendBloom_engineering_architecture.md` but file is not in repo.

**How to avoid:** Treat Assumptions Log items as plan checkpoints; derive table from REQUIREMENTS; flag `[ASSUMED]` defaults for human verify at phase gate.

**Warning signs:** Planner cannot cite a source for a default value.

### Pitfall 2: Zipper noise on automated knobs

**What goes wrong:** Stepped/blocky automation heard in DAW.

**Why it happens:** Snapshot sets targets but inner loop uses block-constant values; or ramp too short (<10 ms).

**How to avoid:** Per-sample `getNextValue()` for gain-affecting params; 20–50 ms ramps for main knobs.

**Warning signs:** REAPER automation lane produces audible steps at 512-sample buffers.

### Pitfall 3: Bypass double-path confusion

**What goes wrong:** Host calls `processBlockBypassed` while plugin also checks bypass param — silence or full-wet glitch.

**Why it happens:** JUCE 8: when `getBypassParameter()` returns non-null, check param in `processBlock` and do not rely solely on `processBlockBypassed` [CITED: AudioProcessor.h].

**How to avoid:** Single crossfade path in `processBlock`; override `getBypassParameter()` to return bypass bool param.

**Warning signs:** Bypass works in one DAW but clicks or mutes in another.

### Pitfall 4: Heap allocation in processBlock

**What goes wrong:** pluginval / realtime safety failure in Phase 8.

**Why it happens:** Allocating `ParameterSnapshot` on heap, building `std::vector` per block, `String` operations.

**How to avoid:** Stack `ParameterSnapshot`; preallocate dry buffer in `prepareToPlay`; fixed-size `std::array` for smoothers.

**Warning signs:** `malloc` under debugger breakpoints in `processBlock`.

### Pitfall 5: Curve tests testing APVTS instead of math

**What goes wrong:** Tests become flaky host-dependent integration tests.

**Why it happens:** Testing through full processor with uncertain defaults.

**How to avoid:** Test `ParameterCurves` pure functions with `Catch::Approx` at norm = {0, 0.25, 0.5, 0.75, 1.0}.

**Warning signs:** Tests pass/fail when default params change.

## Code Examples

### APVTS State Persistence

```cpp
// Source: repo-samples/twoonemoon-PinkGuitarFX/Source/PluginProcessor.cpp (pattern)
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream stream (destData, false);
    stream.writeFromXml (*apvts.copyState().createXml(), nullptr, "");
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}
```

### createParameterLayout with Skewed Size

```cpp
// Source: RESEARCH_CORPUS R8 + JUCE NormalisableRange
layout.add (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID { IDs::size, 1 }, "Size",
    juce::NormalisableRange<float> { 0.0f, 1.0f, 0.5f, 0.5f }, // skew 0.5 ≈ pow(x,2.4)
    0.5f));
```

### Catch2 Curve Test

```cpp
// Source: BUILD_MICROSTEPS MB-012 / TEST-01
TEST_CASE ("sizeToRT60 matches architecture curve", "[curves][parm]")
{
    REQUIRE (ParameterCurves::sizeToRT60 (0.0f) == Catch::Approx (0.25f));
    REQUIRE (ParameterCurves::sizeToRT60 (1.0f) == Catch::Approx (6.0f));
    REQUIRE (ParameterCurves::sizeToRT60 (0.5f) == Catch::Approx (0.25f + 5.75f * std::pow (0.5f, 2.4f)));
}

TEST_CASE ("level equal-power at 0.5 is -3 dB each", "[curves][parm]")
{
    float dry {}, wet {};
    ParameterCurves::levelEqualPower (0.5f, dry, wet);
    REQUIRE (dry == Catch::Approx (wet).margin (1e-5f));
    REQUIRE (dry == Catch::Approx (std::sin (juce::MathConstants<float>::halfPi * 0.5f)).margin (1e-5f));
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `createAndAddParameter()` in constructor | `ParameterLayout` passed to APVTS ctor | JUCE 5+ | Layout can be static / constexpr-friendly |
| Raw float* parameters | `std::atomic<float>*` via `getRawParameterValue` | JUCE 5+ | Lock-free audio thread reads |
| Block-rate parameter updates | Per-sample `SmoothedValue` | JUCE 4+ | Standard for mix/gain params |
| Instant bypass | Crossfaded bypass (5 ms) | Industry practice | PARM-06 requirement |

**Deprecated/outdated:**
- Reading parameters via `getParameter()` in audio thread — replaced by APVTS atomics.
- Single dual-purpose "In" APVTS param — CROSS_AGENT mandates split gain/threshold IDs.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `SendBloom_engineering_architecture.md` absent; parameter table derived from REQUIREMENTS + corpus | Derived Parameter Table | Missing params or wrong ranges; session/preset rework |
| A2 | Default `input_gain` norm = 0.5 (mid gain) | Derived Parameter Table | Unity gain point wrong at init |
| A3 | Default `input_threshold` norm = 0.35 | Derived Parameter Table | Gate too sensitive/insensitive at init |
| A4 | Main knob smoothing ramps = 20 ms; size/threshold = 50 ms | Pattern 3 | Zipper or sluggish response |
| A5 | Soft send exponent = 1.2 (Firm = 1.85 per BUILD_MICROSTEPS MB-040) | Pattern 5 | Send feel wrong vs hardware |
| A6 | `authentic_color` default = true | Derived Parameter Table | Wrong default timbre in Phase 5 |
| A7 | `send_amount` default = 1.0 (full send when disconnected mode uses 1.0 anyway) | Derived Parameter Table | Silent wet on first load when connected |
| A8 | `output_gain` range −12…+12 dB | Derived Parameter Table | Insufficient trim range |

## Open Questions

1. **Where is the engineering architecture parameter table?**
   - What we know: PROJECT.md cites v0.1 doc as authoritative; not in workspace.
   - What's unclear: Exact defaults, smoothing ms per param, send soft exponent.
   - Recommendation: Human verify Assumptions Log at phase gate; ingest doc if user has it externally.

2. **Single "In" knob vs split automation lanes**
   - What we know: CROSS_AGENT mandates split APVTS IDs; UI-01 shows one In knob.
   - What's unclear: Whether In knob writes both params proportionally in Phase 9.
   - Recommendation: Phase 2 exposes both; link in UI phase only.

3. **Dummy hook audibility level**
   - What we know: CONTEXT requires audible automation verification.
   - What's unclear: How aggressive tanh/size filtering should be.
   - Recommendation: Planner discretion — aim for clearly audible at extremes without clipping.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build | ✓ | 4.3.3 | — |
| Node.js | gsd-tools | ✓ | 22.22.3 | — |
| C++20 compiler | SharedCode | ✓ (Phase 1 built) | — | — |
| Catch2 | Unit tests | ✓ (CPM fetch) | 3.8.1 | — |
| JUCE 8 submodule | APVTS/DSP | ✓ | submodule | — |
| ctest | CI/local test | ✓ | homebrew | `cmake --build Builds --target Tests` |

**Missing dependencies with no fallback:** none

**Missing dependencies with fallback:** none

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (CPM + `catch_discover_tests`) |
| Quick run command | `ctest --test-dir Builds -R "curves|parm" --output-on-failure` |
| Full suite command | `ctest --test-dir Builds --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| PARM-01 | All IDs in `ParameterIDs.h` | unit | `ctest --test-dir Builds -R ParameterIDs` | ❌ Wave 0 |
| PARM-02 | Layout exposes all params | unit | `ctest --test-dir Builds -R ParameterLayout` | ❌ Wave 0 |
| PARM-03 | Snapshot capture no inner-loop atomics | static review + unit | `ctest --test-dir Builds -R ParameterSnapshot` | ❌ Wave 0 |
| PARM-04 | Smoothing ramps configured | unit | `ctest --test-dir Builds -R SmoothedParameter` | ❌ Wave 0 |
| PARM-05 | Curve mappings correct | unit | `ctest --test-dir Builds -R "curves"` | ❌ Wave 0 |
| PARM-06 | 5 ms bypass crossfade | unit | `ctest --test-dir Builds -R BypassCrossfade` | ❌ Wave 0 |

### Sampling Rate

- **Per task commit:** `ctest --test-dir Builds -R "curves|parm" --output-on-failure`
- **Per wave merge:** `ctest --test-dir Builds --output-on-failure`
- **Phase gate:** Full suite green + pluginval 5 (carried from Phase 1) + DAW automation smoke (human)

### Wave 0 Gaps

- [ ] `tests/ParameterCurvesTest.cpp` — covers PARM-05 / TEST-01 curve golden values
- [ ] `tests/ParameterSnapshotTest.cpp` — covers PARM-03 derived fields
- [ ] `tests/BypassCrossfadeTest.cpp` — covers PARM-06 ramp length / no discontinuity
- [ ] `tests/PluginBasics.cpp` — extend with APVTS state round-trip smoke (PARM-02)
- [ ] Source files: `ParameterIDs.h`, `ParameterCurves.h`, `ParameterSnapshot.h`, layout, smoothers

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | N/A — audio plugin |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | yes | `NormalisableRange` clamps all float params; bool/choice enums bounded |
| V6 Cryptography | no | No secrets in Phase 2 |

### Known Threat Patterns for JUCE APVTS

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Out-of-range host parameter values | Tampering | JUCE `NormalisableRange` + `AudioParameterFloat` clamping |
| State XML injection via preset load | Tampering | JUCE `ValueTree::fromXml` parsing; don't eval XML |
| Denormal CPU burn in gain ramps | DoS | `ScopedNoDenormals` in `processBlock` (already Phase 1 pattern) |
| Non-realtime alloc in audio thread | DoS | Snapshot on stack; preallocated buffers |

## Sources

### Primary (HIGH confidence)

- JUCE submodule `juce_AudioProcessorValueTreeState.h` — APVTS layout, `getRawParameterValue`, state copy/replace
- JUCE submodule `juce_SmoothedValue.h` — `reset(sampleRate, rampSeconds)`, `getNextValue()`, `applyGain`
- JUCE submodule `juce_DryWetMixer.cpp` — `sin3dB` equal-power formula
- JUCE submodule `juce_AudioProcessor.h` — bypass param, `processBlockBypassed` crossfade guidance
- `CMakeLists.txt`, `cmake/Tests.cmake` — SharedCode, Catch2 3.8.1

### Secondary (MEDIUM confidence)

- `.planning/RESEARCH_CORPUS.md` R8 — curve formulas, send 25 ms, input dual mapping
- `.planning/BUILD_MICROSTEPS.md` MB-010–015, MB-040 — parameter build gates, send curve
- `.planning/CROSS_AGENT_SYNTHESIS.md` — split input gain/threshold
- `.planning/REQUIREMENTS.md` PARM-01–06 — phase requirements
- JUCE forum / docs on bypass crossfade [CITED: forum.juce.com, docs.juce.com SmoothedValue]

### Tertiary (LOW confidence)

- Default parameter values and soft send exponent — not in corpus; marked `[ASSUMED]`
- `SendBloom_engineering_architecture.md` — referenced but not found in workspace

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — JUCE 8 already in project; no new dependencies
- Architecture: MEDIUM — patterns verified in JUCE source; parameter table synthesized (architecture doc missing)
- Pitfalls: MEDIUM — bypass and snapshot pitfalls documented in JUCE; smoothing times partly assumed

**Research date:** 2026-07-06
**Valid until:** 2026-08-06 (30 days — stable JUCE APVTS domain)
