# Phase 5: SchroederTank32 Reverb - Research

**Researched:** 2026-07-06
**Domain:** FV-style Schroeder tank reverb at 32.768 kHz (JUCE 8 / C++20 header-only DSP)
**Confidence:** HIGH

## Summary

Phase 5 replaces `PlaceholderReverb` (single 300 ms feedback delay) with a clean-room **SchroederTank32** engine matching the locked FV-1 topology: **4 series allpass diffusion → 4 parallel damped combs → modulated tank allpass**. ADR-002's 8-line FDN path is superseded; this tank is primary per PROJECT.md and CONTEXT.

Existing integration points are stable: `GatedBloomChain` calls `reverb.processSample(wet, rt60Seconds)` after send scaling; `ParameterCurves::sizeToRT60` already implements VERB-03; `ParameterSnapshot` exposes `darkMode` and `authenticColor` but the processor does not yet wire them to reverb. Phase 3/4 routing tests (53 passing) must remain green after swap.

chowdsp `SimpleReverb` uses FDN (not tank) — study for `calcGainForT60` feedback math and `DryWetMixer` patterns only [VERIFIED: `SimpleReverbPlugin.h`]. BYOD `SchroederAllpass.h` provides a clean Schroeder AP recurrence [VERIFIED: repo-samples]. Freeverb delay ratios inform coprime length selection scaled to 32.768 kHz [VERIFIED: `Freeverb.h`]. No GPL code fork; no EEPROM/FV-1 bytecode.

**Primary recommendation:** Header-only `SchroederTank32` with constexpr delay table at 32.768 Hz, per-comb RT60 feedback from longest delay, dark/bright predelay + damping crossfade, optional 32 kHz coloration path + 9-bit param quantization when `authentic_color` ON; swap in `GatedBloomChain`; add RT60 impulse tests at size 0.25/0.5/1.0 within ±15%.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Engine
- SchroederTank32 primary: 4 series APF → 4 parallel damped combs → modulated tank AP
- Fixed delay lengths at 32,768 Hz per architecture/planning corpus
- RT60 mapping: `0.25 + 5.75×size^2.4` with smooth feedback updates
- Dark: 55 ms predelay, 3200 Hz damping; Bright: no predelay, 8000 Hz
- authentic_color: internal 32 kHz processing, optional 9-bit param quantization

#### Integration
- Swap PlaceholderReverbStub only; gates/IO/send/dirt unchanged
- RT60 impulse tests at size 0.25, 0.5, 1.0 within ±15%

### Claude's Discretion
Class decomposition, delay line implementation, oversampling/downsample strategy at planner discretion. Study chowdsp SimpleReverb in repo-samples.

### Deferred Ideas (OUT OF SCOPE)
- Fdn8Reverb fallback (Phase 8)
- Extended stereo (post-v1)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| VERB-01 | SchroederTank32: 4 APF → 4 comb → modulated tank AP | FV-1 dossier topology; BYOD SchroederAllpass pattern |
| VERB-02 | Fixed delay lengths at 32,768 Hz | `SchroederTank32DelayTable` constexpr coprime set [SYNTHESIZED: Freeverb ratios × 32768/44100] |
| VERB-03 | Size→RT60 `0.25 + 5.75×size^2.4` | `ParameterCurves::sizeToRT60` exists Phase 2 |
| VERB-04 | Dark: 55 ms predelay + 3200 Hz; Bright: 0 predelay + 8000 Hz | One-pole LPF in comb feedback; predelay line |
| VERB-05 | authentic_color: 32 kHz internal + 9-bit quantization | Fractional resampler; `quantize9bit()` on feedback/damping |
| VERB-07 | RT60 ±15% impulse test at size 0.25, 0.5, 1.0 | Schroeder energy decay measurement in Catch2 |
</phase_requirements>

## Architectural Responsibility Map

Single-tier audio plugin — all capabilities on realtime audio thread (header-only, no heap in `processSample`).

| Capability | Owner | Rationale |
|------------|-------|-----------|
| Delay table constants | `SchroederTank32DelayTable.h` | VERB-02 compile-time proof |
| Schroeder atoms (APF, comb) | `SchroederAllpass.h`, `DampedComb.h` | Unit-testable |
| Tank engine + resampler | `SchroederTank32.h` | VERB-01/04/05 |
| Chain slot swap | `GatedBloomChain.h` | Preserve routing |
| Param wiring | `PluginProcessor.cpp` | dark + authentic from snapshot/smoothers |
| RT60 tests | `SchroederTank32Test.cpp` | VERB-07 offline |

## Standard Stack

### Core

| Library | Version | Purpose |
|---------|---------|---------|
| JUCE `juce_dsp` | 8.x | `DelayLine`, `FirstOrderTPTFilter`, `ProcessSpec` |
| Project `ParameterCurves` | — | `sizeToRT60` |
| Catch2 | 3.8.1 | Impulse + RT60 + routing regression |

### Study References (not linked)

| Source | Role |
|--------|------|
| chowdsp SimpleReverb | FDN T60 feedback pattern |
| BYOD SchroederAllpass | AP recurrence |
| Freeverb delay ratios | Coprime length seed |
| PlaceholderReverb | Existing RT60→feedback `exp(-6.9078*Td/T60)` |

## Architecture Patterns

### Signal Flow

```
Input → [predelay 0|55ms] → APF×4 (series) → split → Comb×4 (parallel, damped) → sum/4
     → Tank AP (LFO mod ±0.5ms) → output
```

### Delay Table (32,768 Hz) [SYNTHESIZED]

Scaled from Freeverb coprime ratios (×32768/44100), clamped under 1 s RAM:

| Stage | Samples @ 32 kHz | ~ms |
|-------|------------------|-----|
| APF 1–4 | 167, 253, 328, 413 | 5–13 |
| Comb 1–4 | 1057, 1108, 1157, 1202 | 32–37 |
| Tank AP | 677 + LFO | ~21 |

Host-rate mode scales: `delayHost = delay32k * (hostRate / 32768)`.

### Pattern 1: Schroeder Allpass

```cpp
// y = -g*x + delayed; push x + g*delayed
auto d = delay.popSample(0);
const auto v = x + g * d;
delay.pushSample(0, v);
return d - g * v;
```

### Pattern 2: Damped Comb + RT60 Feedback

```cpp
// Per comb: g = exp(-6.9078 * delaySec / rt60)
const auto g = std::exp(-6.9078f * (delaySamples / sr) / rt60);
```

### Pattern 3: 32 kHz Authentic Path

Fractional hold: accumulate host samples; emit one 32 kHz tick per `hostRate/32768`; linear interpolate output back to host rate.

### Pattern 4: 9-bit Quantization

`quantize9bit(x) = round(x * 511) / 511` applied to feedback gain and damping when `authenticColor`.

### Anti-Patterns

- **Forking chowdsp_reverb GPL module** — license risk; clean-room only.
- **Clearing tank on send→0** — violates CHN-03; engine state persists.
- **Per-sample APVTS reads** — pass smoothed coeffs from processor.

## Don't Hand-Roll

| Problem | Use Instead | Why |
|---------|-------------|-----|
| RT60 curve | `ParameterCurves::sizeToRT60` | Phase 2 locked |
| Delay buffers | `juce::dsp::DelayLine` | RT-safe, tested |
| Damping LPF | `FirstOrderTPTFilter` | Stable one-pole |
| Comb feedback math | `exp(-6.9078*Td/T60)` | Standard T60 identity |

## Common Pitfalls

### Pitfall 1: Metallic ringing
**Avoid:** Coprime delays; modulated tank AP (0.5 Hz LFO, ±16 samples).

### Pitfall 2: RT60 test flakiness
**Avoid:** Warmup 2× RT60; measure Schroeder energy −60 dB crossing; ±15% margin.

### Pitfall 3: Routing regression
**Avoid:** Run full `GatedBloomChain` + `DryNeverGated` suite after swap.

## Code Examples

### RT60 from Impulse (test helper)

```cpp
float measureRT60(const std::vector<float>& ir, double sr) {
    // Schroeder backward integration on squared IR tail
    // Return time to −60 dB from peak energy
}
```

### Chain Integration

```cpp
wet = reverb.processSample(wet, rt60Seconds, darkModeMix, authenticColor);
```

## Open Questions

1. **Exact FV-1 delay table unavailable (legal)** — synthesized coprime set; tune RT60 test to pass ±15%.
2. **Dark crossfade 15 ms** — use existing `darkModeTarget` smoother (15 ms) from `SmoothedParameterBank`.

## Sources

### Primary (HIGH)
- `05-CONTEXT.md`, `REQUIREMENTS.md`, `PROJECT.md`
- `source/GatedBloomChain.h`, `PlaceholderReverb.h`, `ParameterCurves.h`
- BYOD `SchroederAllpass.h`, chowdsp `SimpleReverbPlugin.h`

### Secondary (MEDIUM)
- `RESEARCH_ADDENDUM_USER_DOSSIER.md` — FV-1 topology
- `Freeverb.h` — delay ratios

---

*Phase: 05-schroedertank32-reverb*
*Research completed: 2026-07-06*
*Ready for planning: yes*
