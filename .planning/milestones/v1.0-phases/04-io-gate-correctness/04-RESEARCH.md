# Phase 4: IO + Gate Correctness - Research

**Researched:** 2026-07-06
**Domain:** Real InputStage/OutputStage, EnvelopeDetector, dual NoiseGate profiles replacing Phase 3 stubs (JUCE 8 / C++20)
**Confidence:** HIGH

## Summary

Phase 4 swaps Phase 3 stub IO and gate classes for production DSP atoms **without changing proven routing**. Phase 3 locked the parallel topology: unity dry tap, mono-summed wet feed, input-keyed post gate, `ParallelWetMixer` wet-only level scaling [VERIFIED: `GatedBloomChain.h`, `PluginProcessor.cpp`, 36 passing tests].

Current processor applies `inputGain` and `outputGain` inline in `processBlock`; CONTEXT locks moving these into `InputStage` (gain + soft clip + 50 ms clip-hold flag) and `OutputStage` (output trim). `StubInputEnvelope` and `StubNoiseGate` become `EnvelopeDetector` and `NoiseGate` with constexpr PreSoft/PostHard profiles. Post gate remains keyed from **input** envelope, not wet tail (CHN-03 preserved).

BYOD `Gate.cpp` FSM (Attack/Hold/Release + level detector) is the study reference [VERIFIED: `.planning/repo-samples/Chowdhury-DSP-BYOD/src/processors/other/Gate.cpp`]. RESEARCH_CORPUS R3 decision: greenfield `NoiseGate` with BYOD-informed FSM, dual profiles via constexpr, 3 dB hysteresis from LiveDSP pattern [CITED: RESEARCH_CORPUS.md R3]. No GPL code fork — clean-room implementation.

**Primary recommendation:** Add four header-only DSP classes (`InputStage`, `OutputStage`, `EnvelopeDetector`, `NoiseGate`), swap into `GatedBloomChain` and `PluginProcessor`, extend Catch2 with IO/gate unit tests plus Phase 3 routing regression, then phase-gate with pluginval 5 and DAW smoke.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### IO Stages
- InputStage: input gain dB, soft clip limiter, 50 ms clip-hold flag for UI
- OutputStage: output gain dB trim
- Mono bus contract per IO-03

#### Gates
- EnvelopeDetector: peak follower with configurable attack/release
- PreSoft: 150 ms release, hum silencer on wet input only
- PostHard: ≤7 ms Authentic release, brutal wet chop keyed from input detector
- Gate Pre/Post toggle repositions gate on wet path only; dry never gated
- input_threshold_db with 3 dB hysteresis

### Claude's Discretion
Swap stubs in GatedBloomChain incrementally; exact class file layout and test fixtures at planner discretion. Preserve Phase 3 routing proofs as regression tests.

### Deferred Ideas (OUT OF SCOPE)
- SchroederTank32 (Phase 5), real PressureSend (Phase 7), UI clip LED (Phase 9)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| IO-01 | InputStage gain + soft clip + 50 ms clip-hold | Tanh knee at −3 dBFS [CITED: BUILD_MICROSTEPS MB-028]; hold counter in samples |
| IO-02 | OutputStage output gain dB trim | `ParameterCurves::outputGainLinear`; move inline multiply from processor |
| IO-03 | Mono bus: stereo summed in, dual-mono out unless Extended | Existing mono sum in processor; dual-mono write loop unchanged; `extendedStereo` deferred Phase 8 |
| GATE-01 | EnvelopeDetector peak follower, configurable attack/release | Replace `StubInputEnvelope`; default 5/10 ms sidechain [VERIFIED: stub defaults] |
| GATE-02 | PreSoft 150 ms release, hum silencer wet-only | Soft floor ~−80 dB [CITED: RESEARCH_CORPUS R3]; gate on wet path only |
| GATE-03 | PostHard ≤7 ms release, input-keyed chop | Hard floor 0; envelope from input detector only [VERIFIED: CHN-03 tests] |
| GATE-04 | Pre/Post toggle wet-path only; dry never gated | Existing `gatePreSoft` routing in `GatedBloomChain::processSample` |
| GATE-05 | input_threshold_db with 3 dB hysteresis | Open/close thresholds offset 3 dB [CITED: RESEARCH_CORPUS R3, MB-021] |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| InputStage / OutputStage | API / Backend (audio thread) | — | Per-sample in `processBlock`; no heap |
| EnvelopeDetector | API / Backend (audio thread) | — | Sidechain on gained mono input |
| NoiseGate (dual profile) | API / Backend (audio thread) | — | Wet-path multiply only |
| GatedBloomChain orchestration | API / Backend (audio thread) | — | Stub swap; topology unchanged |
| Clip-hold flag | API / Backend (audio thread) | UI (Phase 9) | Flag computed RT-safe; LED deferred |
| IO/gate unit tests | Build / CI | — | Catch2 offline proofs |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce_audio_basics` | 8.x | `Decibels`, `SmoothedValue` | Phase 1–3 foundation |
| Project `ParameterCurves` | — | Gain/threshold mappings | Phase 2 complete |
| Catch2 | 3.8.1 | IO/gate + routing regression | Phase 1 harness |

### Study References (not forked)

| Source | Role |
|--------|------|
| BYOD `Gate.cpp` | Attack/Hold/Release FSM pattern |
| RESEARCH_CORPUS R3 | PreSoft −80 dB floor, PostHard hard gate, 3 dB hysteresis |
| Phase 3 `StubNoiseGate` | Behavioral baseline for regression |

**No new packages.**

## Architecture Patterns

### Signal Flow (post Phase 4)

```
input buffer ──┬──► dryBuffer copy (unity dry tap, never gated)
               │
               └──► mono sum ──► InputStage(gain, soft clip) ──► gained mono
                                        │
                                        ├──► EnvelopeDetector.follow(|gained|)
                                        │
                                        ▼
                              GatedBloomChain wet path
                              [PreSoft gate if pre] → send → reverb → dirt
                              [PostHard gate if post, keyed from input env]
                                        │
                                        ▼
                              ParallelWetMixer(dryTap, wet, levelWet)
                                        │
                                        ▼
                              BypassCrossfade + OutputStage(trim)
                                        │
                                        ▼
                              dual-mono write (IO-03)
```

### Pattern 1: InputStage

**What:** Applies smoothed linear gain, tanh soft clip at −3 dBFS knee, maintains 50 ms clip-hold flag.

**API:**
```cpp
void prepare(double sampleRate) noexcept;
float processSample(float monoIn, float gainLinear) noexcept;
bool isClipHoldActive() const noexcept;
```

### Pattern 2: OutputStage

**What:** Single-sample output trim.

```cpp
static float processSample(float sample, float gainLinear) noexcept;
```

### Pattern 3: EnvelopeDetector

**What:** Peak follower `|x|` with exponential attack/release coeffs.

```cpp
void prepare(double sr, float attackMs, float releaseMs) noexcept;
float process(float input) noexcept;
```

### Pattern 4: NoiseGate with profiles

**What:** Hysteresis threshold comparator + smoothed gain toward open (1.0) or profile floor.

```cpp
enum class GateProfile { PreSoft, PostHard };
void prepare(double sr, GateProfile profile) noexcept;
float process(float inputEnvelope, float thresholdDb) noexcept;
```

| Profile | Release | Floor | Attack |
|---------|---------|-------|--------|
| PreSoft | 150 ms | ~−80 dB (~1e-4 linear) | 2 ms |
| PostHard | 7 ms | 0 (hard) | 0.5 ms |

Hysteresis: open when `env > threshGain`; close when `env < threshGain * 10^(-3/20)`.

### Pattern 5: Incremental stub swap in GatedBloomChain

Replace `StubInputEnvelope` → `EnvelopeDetector`, `StubNoiseGate` → `NoiseGate` with same `processSample` signature. Keep `getEnvelope()` accessor for tests. Delete stub headers after swap.

## Recommended Project Structure

```
source/
├── InputStage.h
├── OutputStage.h
├── EnvelopeDetector.h
├── NoiseGate.h
├── GatedBloomChain.h          # uses real classes
├── PluginProcessor.cpp      # InputStage/OutputStage wiring
└── (delete StubInputEnvelope.h, StubNoiseGate.h)

tests/
├── InputStageTest.cpp
├── OutputStageTest.cpp
├── EnvelopeDetectorTest.cpp
├── NoiseGateTest.cpp
├── GatedBloomChainTest.cpp  # update + dry-never-gated
└── MonoBusTest.cpp          # IO-03 stereo sum contract
```

## Testing Strategy

| Test | Requirement | Method |
|------|-------------|--------|
| Input gain + clip flag | IO-01 | High gain sine → clip hold true; decays after 50 ms silence |
| Output trim | IO-02 | −6 dB gain halves amplitude |
| Stereo sum | IO-03 | L=0.5 R=0.3 → mono chain sees 0.4 |
| Envelope attack/release | GATE-01 | Step response timing |
| PreSoft hum floor | GATE-02 | Low envelope → gain > 0 but << 1 |
| PostHard chop | GATE-03 | Burst→silence RMS < 0.01 (existing test) |
| Dry never gated | GATE-04 | Gate closed wet=0; dry mix unchanged |
| Hysteresis | GATE-05 | Chatter band between open/close thresholds |
| Phase 3 regression | CHN-01–03 | All existing GatedBloomChain tests GREEN |

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Stub swap breaks routing | Run full Phase 3 test suite unchanged |
| Hysteresis too wide/narrow | Unit test open/close at ±3 dB |
| Clip hold timing | Assert hold ≥ 45 ms at 48 kHz |
| Input gain on dry path | Dry tap from pre-gain buffer copy only |

## Planner Notes (MVP)

**4 plans recommended:**
1. IO stages + unit tests (IO-01, IO-02, IO-03)
2. EnvelopeDetector + NoiseGate + unit tests (GATE-01, GATE-02, GATE-03, GATE-05)
3. Chain/processor integration + dry-never-gated + regression (GATE-04, CHN regression)
4. Phase gate: full suite, pluginval 5, DAW smoke, VERIFICATION.md
