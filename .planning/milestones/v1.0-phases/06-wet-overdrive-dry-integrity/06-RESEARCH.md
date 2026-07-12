# Phase 6: Wet Overdrive + Dry Integrity - Research

**Researched:** 2026-07-06
**Domain:** Wet-only asymmetric tanh overdrive on reverb output (JUCE 8 / C++20 header-only DSP)
**Confidence:** HIGH

## Summary

Phase 6 replaces `PlaceholderWetDirt` (symmetric `tanh(wet*3)` blend stub) with **`WetOverdrive`**: a fixed asymmetric tanh circuit on the wet path only, after `SchroederTank32` and before the post gate. The `distn` knob blend curve (`pow(norm, 2.8)`) is already implemented in `ParameterCurves::distnBlend` and smoothed in `SmoothedParameterBank` — WetOverdrive receives the curved `distnBlend` scalar per sample.

Dry integrity is architectural: `PluginProcessor` copies input to `dryBuffer` before `InputStage` and wet chain; `ParallelWetMixer::mix(dryTap, wet, wetGain)` keeps dry at unity. OD-03 requires a **dry-tap THD regression test** at `distn=1`, `level=1` proving the extracted dry component is unchanged when distortion maxes.

No oversampling in Authentic mode (Extended OS deferred per CONTEXT). Phase 5 routing tests (62 passing) must remain green after swap.

**Primary recommendation:** Header-only `WetOverdrive.h` with RESEARCH_CORPUS R4 asymmetric tanh; swap include in `GatedBloomChain`; add `WetOverdriveTest.cpp` + `DryPathIntegrityTest.cpp` with Goertzel THD helper; human DAW grind smoke in VERIFICATION.md.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### WetOverdrive
- Asymmetric tanh grind on wet path only (after reverb, before post gate)
- distn=0: clean wet; distn=1: fully driven wet via `ParameterCurves::distortionBlend` (pow 2.8)
- Fixed drive circuit — no oversampling in Authentic mode

#### Dry Integrity
- Dry tap taken before wet chain; never passes through WetOverdrive
- Unit test: dry-path THD unchanged at distn=1, level=1

### Claude's Discretion
Exact tanh asymmetry coefficients, blend implementation, THD test fixture at planner discretion.

### Deferred Ideas (OUT OF SCOPE)
- OD oversampling in Extended mode (post-v1)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| OD-01 | WetOverdrive on reverb output only; fixed asymmetric tanh | RESEARCH_CORPUS R4 `asymmetricTanh`; chain slot after `reverb.processSample` |
| OD-02 | distn blends clean wet ↔ driven via pow(distn, 2.8) | `ParameterCurves::distnBlend` + `SmoothedParameterBank` already wired |
| OD-03 | Dry path THD unchanged at distn=1, level=1 | Goertzel THD on `output − wet`; dry tap never in WetOverdrive |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Owner | Rationale |
|------------|-------|-----------|
| Asymmetric tanh + blend | `WetOverdrive.h` | OD-01 fixed circuit |
| Blend curve | `ParameterCurves::distnBlend` | OD-02 locked Phase 2 |
| Chain slot swap | `GatedBloomChain.h` | Preserve bloom-then-chop order |
| Dry unity mix | `ParallelWetMixer.h` | CHN-02 unchanged |
| distn smoothing | `SmoothedParameterBank` | 20 ms already set |
| Dry THD test | `DryPathIntegrityTest.cpp` | OD-03 offline gate |
| Wet-only unit tests | `WetOverdriveTest.cpp` | OD-01/02 isolation |

## Signal Flow (unchanged topology)

```
monoIn → [preGate?] → send → SchroederTank32 → WetOverdrive → [postGate?] → wet
dryBuffer (raw input) ──────────────────────────────────────→ ParallelWetMixer → out
```

`PlaceholderWetDirt::process` → `WetOverdrive::process` at line 43 of `GatedBloomChain.h`.

## Standard Stack

| Library | Version | Purpose |
|---------|---------|---------|
| `<cmath>` std::tanh | C++20 | Asymmetric waveshaper |
| `ParameterCurves` | Phase 2 | `distnBlend` pow 2.8 |
| Catch2 | 3.8.1 | Unit + THD regression |

## Architecture Patterns

### Pattern 1: Asymmetric tanh (RESEARCH_CORPUS R4)

```cpp
static float asymmetricTanh (float x) noexcept
{
    constexpr auto drive = 3.0f;
    constexpr auto asymPos = 1.12f;
    auto scaled = x * drive;
    if (scaled > 0.0f)
        scaled *= asymPos;
    return std::tanh (scaled) / std::tanh (drive);
}
```

Positive half-wave driven harder → even-harmonic grit vs symmetric stub.

### Pattern 2: Wet-only blend (same as PlaceholderWetDirt)

```cpp
static float process (float wet, float distnBlend) noexcept
{
    const auto driven = asymmetricTanh (wet);
    return wet + distnBlend * (driven - wet);
}
```

`distnBlend` is pre-curved; WetOverdrive does not re-apply pow.

### Pattern 3: Dry-path THD test (OD-03)

1. Render plugin at `distn=0`, `level=1` → `out0`
2. Render at `distn=1`, `level=1` → `out1`
3. Render wet-only via `GatedBloomChain` with matching params → `wet0`, `wet1`
4. `dry0 = out0 − wet0`, `dry1 = out1 − wet1` (both ≈ raw dry tap)
5. `measureThd(dry0) ≈ measureThd(dry1)` within tolerance

Goertzel at fundamental + harmonics 2–5; steady-state sine after warmup.

### Anti-Patterns

- **Processing dry through WetOverdrive** — violates product core value
- **Re-applying pow(distn, 2.8) inside WetOverdrive** — double curve
- **Per-sample APVTS reads** — use smoothed `distnBlend` from bank

## Don't Hand-Roll

| Problem | Use Instead | Why |
|---------|-------------|-----|
| distn curve | `ParameterCurves::distnBlend` | Phase 2 locked |
| Dry/wet sum | `ParallelWetMixer::mix` | CHN-02 |
| Blend idiom | PlaceholderWetDirt pattern | Proven Phase 3 |

## Common Pitfalls

### Pitfall 1: Confusing mix THD with dry THD
At level=1, full output THD rises with distn because wet adds harmonics. Test **extracted dry** (`output − wet`), not raw output.

### Pitfall 2: Routing regression
Run full `GatedBloomChain`, `DryNeverGated`, `PluginBasics` routing suite after swap.

### Pitfall 3: Symmetric tanh only
Placeholder used symmetric tanh; OD-01 requires measurable asymmetry (positive vs negative drive delta).

## Code Examples

### Chain Integration

```cpp
wet = reverb.processSample (wet, rt60Seconds, darkModeMix, authenticColor);
wet = WetOverdrive::process (wet, distnBlend);
```

### THD Helper (Goertzel)

```cpp
float measureThd (const std::vector<float>& samples, double sr, float fundHz);
```

## Open Questions

1. **Exact asymmetry coefficient** — 1.12f from RESEARCH_CORPUS; tune if DAW smoke feels weak (planner discretion).
2. **THD tolerance** — ±0.5% absolute or relative match between distn=0/1 dry extracts `[ASSUMED]`.

## Sources

### Primary (HIGH)
- `06-CONTEXT.md`, `REQUIREMENTS.md` OD-01–OD-03
- `source/GatedBloomChain.h`, `PlaceholderWetDirt.h`, `ParameterCurves.h`, `ParallelWetMixer.h`
- `source/PluginProcessor.cpp` dryBuffer copy before chain

### Secondary (MEDIUM)
- `RESEARCH_CORPUS.md` R4 asymmetric tanh
- `03-RESEARCH.md` Pattern 5 PlaceholderWetDirt

---

*Phase: 06-wet-overdrive-dry-integrity*
*Research completed: 2026-07-06*
*Ready for planning: yes*
