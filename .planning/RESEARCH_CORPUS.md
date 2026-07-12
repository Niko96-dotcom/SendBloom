# Research Corpus — Pre-Code Investigation

**Date:** 2026-07-06  
**Method:** 8 parallel research lanes + shallow clones in `.planning/repo-samples/`  
**Rule:** No `Source/` plugin code until ADRs signed off.

---

## Executive fork decisions (read this first)

| Layer | Decision | Primary reference |
|-------|----------|-------------------|
| **Scaffold + CI** | Fork **`sudara/pamplejuce`** (not anthonyalfimov alone) | Catch2 + pluginval + notarization patterns |
| **CMake baseline** | Borrow **`anthonyalfimov/JUCE-CMake-Plugin-Template`** FetchContent JUCE 8.0.12 pin | Clean minimal CMake |
| **Reverb DSP** | **Study** `chowdsp_utils` SimpleReverb + **study** RSAlgorithmicVerb FDNs; **implement custom** 8-line FDN | Don't fork whole RSAlgorithmicVerb (GPL) |
| **Gate** | **Adapt pattern** from `BYOD/src/processors/other/Gate.cpp` | Attack/Hold/Release state machine |
| **Wet OD** | **Write from scratch** (~20 lines asymmetric tanh); study `reverb_trickery` Faust `cubicnl` | No full fork |
| **Params** | `chowdsp::ParamHolder` pattern OR plain APVTS; use `juce::dsp::DryWetMixer` for level | SimpleReverb example |
| **Send UI** | **Write custom** `PressureSendPad`; no OSS Igor equivalent found | — |
| **Product landscape** | No OSS matches Reverb-X routing ≥7/10 | Build routing ourselves |

---

## R1 — Scaffold & CI

### Repos evaluated

| Repo | Stars | License | Updated | Solves | Fork score |
|------|-------|---------|---------|--------|------------|
| [sudara/pamplejuce](https://github.com/sudara/pamplejuce) | ~1k+ | MIT | 2026-07 | JUCE 8, Catch2, pluginval CI, packaging, notarization | **5/5** |
| [anthonyalfimov/JUCE-CMake-Plugin-Template](https://github.com/anthonyalfimov/JUCE-CMake-Plugin-Template) | 73 | MIT | 2026-06 | FetchContent JUCE 8.0.12, AU+VST3, Validation.yml | **4/5** |
| [Chowdhury-DSP/JUCEPluginTemplate](https://github.com/Chowdhury-DSP/JUCEPluginTemplate) | 22 | BSD-3 | 2026-05 | ChowDSP setup.sh naming; BSD | **3/5** |
| [Tracktion/pluginval](https://github.com/Tracktion/pluginval) | — | — | 2026-07 | Validation binary (dependency, not fork) | dep |
| [Xav83/pluginval_cmake_integration](https://github.com/Xav83/pluginval_cmake_integration) | low | — | 2021 | CMake pluginval target | **2/5** stale |

### Code choices to copy

**From `anthonyalfimov-JUCE-CMake-Plugin-Template/CMakeLists.txt`:**
- `FetchContent` JUCE with pinned `LIB_JUCE_TAG "8.0.12"`
- `FETCHCONTENT_BASE_DIR` → `Libs/` (survives clean builds)
- `juce_add_plugin` with `FORMATS AU VST3`, manufacturer/plugin codes
- `target_compile_features(... cxx_std_17)` → bump to **20** for us

**From `anthonyalfimov/.github/workflows/Validation.yml`:**
- Download pluginval release zip per OS
- Run strictness against built `.vst3` / `.component`

**From `sudara-pamplejuce/.github/workflows/build_and_test.yml`:**
- Matrix: Linux + macOS universal + Windows
- Ninja + clang everywhere
- sccache, concurrency cancel
- Catch2 test target wired in CMake
- pluginval in same pipeline

### Anti-patterns to avoid
- Projucer-only projects (RSAlgorithmicVerb, shredverb) — migrate cost
- ChowDSP template `setup.sh` if we want zero vendor lock-in to ChowDSP modules
- GPL templates as **base** if commercial closed-source (pamplejuce MIT is safe)

### MB mapping
`MB-002` → fork **pamplejuce**, strip demo DSP, inject our modules  
`MB-004`–`MB-006` → copy CMake + CI wholesale

---

## R2 — Reverb engine

### Repos evaluated

| Repo | License | Algorithm | RT60 / Size | Mono | Fork score |
|------|---------|-----------|-------------|------|------------|
| [Chowdhury-DSP/chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) `examples/SimpleReverb` | BSD/GPL mix | 8-line FDN + ConvolutionDiffuser | `fdnT60Low/High` ms params | ✓ | **5/5 study** |
| [reillypascal/RSAlgorithmicVerb](https://github.com/reillypascal/RSAlgorithmicVerb) | **GPL-3** | Multiple FDN orders, Dattorro, Gardner | `ReverbProcessorParameters` | ✓ | **3/5 study only** |
| [nvssynthesis/shredverb](https://github.com/nvssynthesis/shredverb) | check repo | FDN + FM distortion in feedback | Decay knob | ✓ | **2/5** wrong aesthetic |
| [unicornsasfuel/reverb_trickery](https://github.com/unicornsasfuel/reverb_trickery) | BSD-3 | Faust stereo verb + effect chain | `reverb_decay` 0–100% | stereo | **3/5** routing ideas |
| Surge reverb (in surge repo) | GPL | Many verbs | — | — | study only |

### SimpleReverb — key files (cloned)
```
chowdsp_utils/examples/SimpleReverb/SimpleReverbPlugin.h
chowdsp_utils/modules/dsp/chowdsp_reverb/   # FDN template
```

**Steal:**
- `chowdsp::Reverb::FDN<DefaultFDNConfig<float, 8>>` architecture
- `Multiplicative` smoothers on T60 params
- `juce::dsp::DryWetMixer<float>` for wet/dry (adapt to equal-power)

**Rewrite:**
- Collapse to **mono** single channel
- Map single `size` knob → coupled T60 low/high (dark = darker damping + predelay)
- Remove diffusion time user control (fixed internally)

### RSAlgorithmicVerb — key files
```
Source/FDNs.h          # GeneralizedFDN 8th order, feedback matrix
Source/ProcessorBase.h # ReverbProcessorParameters interface
```

**Steal:** Feedback matrix layout, delay time vectors, LFO modulation pattern  
**Don't fork:** GPL license; too many algorithms (scope creep)

### ADR-002 recommendation
**Greenfield `ReverbEngine` class**, initial implementation by **porting concepts** from chowdsp FDN (BSD `chowdsp_reverb` module) with our RT60 curve. Submodule `chowdsp_utils` as dependency **only if license reviewed** (GPL modules exist in repo — use BSD modules only: `chowdsp_reverb` check license file).

---

## R3 — Gate & envelope

### Repos evaluated

| Repo | License | Detector | Hysteresis | Dual position | Fork score |
|------|---------|----------|------------|---------------|------------|
| [Chowdhury-DSP/BYOD](https://github.com/Chowdhury-DSP/BYOD) `Gate.cpp` | GPL-3 | Peak + LevelDetector | Attack/Hold/Release FSM | single | **5/5 adapt** |
| [martinpenberthy/JUCENoiseGateBasic](https://github.com/martinpenberthy/JUCENoiseGateBasic) | — | JUCE dsp::NoiseGate wrapper | basic | no | **2/5** |
| [bologna121121/NoiseGate](https://github.com/bologna121121/NoiseGate) | — | simple | — | no | **2/5** |
| [twoonemoon/PinkGuitarFX](https://github.com/twoonemoon/PinkGuitarFX) | — | **Naive** abs threshold sample kill | none | no | **1/5** anti-pattern |
| [Aizhee/CleanVerb](https://github.com/Aizhee/CleanVerb) | — | HISE; **ducking** wet by peak | inverse of our gate | no | **3/5** concept only |

### BYOD Gate — extracted pattern (cloned)
File: `BYOD/src/processors/other/Gate.cpp`

```
LevelDetector detectorEnv  → envelope of input
LevelDetector detector01   → attack/release smoothing
States: Attack → Hold → Release
threshGain = decibelsToGain(threshDB)
```

**Our adaptation:**
- **Pre-gate instance:** longer release (150 ms), soft floor (−80 dB)
- **Post-gate instance:** short release (65 ms), hard floor (−120 dB)
- Same `threshDB` from `input` knob mapping
- No Hold needed for post-gate (simplify) OR keep Hold=0

**Parameter ranges from BYOD defaults** (inspect `createParameterLayout` in Gate.cpp):
- Threshold: typical −60 to −20 dB range
- Attack: 1–50 ms
- Release: 50–500 ms

### MB mapping: `MB-020`–`MB-024` — implement `NoiseGate.h` adapted from BYOD FSM, two profiles as constexpr.

### R3 subagent addendum (2026-07-06)
No repo implements dual pre/post in one module. **Architecture refs:**
| Rank | Repo | Role |
|------|------|------|
| 1 | **LSP dsp-units** (LGPL) | `Expander` pre + `Gate` post — Hermite curves, dual hysteresis, sidechain HPF |
| 2 | **airwindows PointyGuitar** (MIT) | Detect pre-amp, gate post-amp — closest positional precedent |
| 3 | **Tukan ExpGate 2** (MIT) | `operation` toggle: expander floor vs hard gate |
| 4 | **GateXpander** (GPL) | Guitar soft expander +12 dB hysteresis — pre profile only |
| 5 | **LiveDSP** `NoiseGate.h` | Clean JUCE header, 3 dB hysteresis — post profile candidate |

**Decision unchanged:** greenfield `NoiseGate` with BYOD Attack/Hold/Release FSM; tune pre/post via constexpr profiles informed by LSP + LiveDSP ranges.

---

## R4 — Wet-only distortion

### Repos evaluated

| Repo | Routing | Saturation | OS | Fork score |
|------|---------|------------|-----|------------|
| [unicornsasfuel/reverb_trickery](https://github.com/unicornsasfuel/reverb_trickery) | `effectize = octave : distort : band : gate` **serial on wet** | `ef.cubicnl(drive,0)` | no | **4/5** routing |
| [Chowdhury-DSP/BYOD](https://github.com/Chowdhury-DSP/BYOD) | serial distortion modules | WDF / multiple | yes | **2/5** overkill |
| [nvssynthesis/shredverb](https://github.com/nvssynthesis/shredverb) | distortion via FDN FM | unique | no | **1/5** |
| Northern Valley DV-1 | closed | wet-only + gate | — | N/A |

### reverb_trickery Faust chain (cloned `reverbtrickery.dsp`)
```
process = dry * dryWet + wet * (octave → distort → band → gate → reverb)
```
**Difference from Reverb-X:** trickery gates **before** reverb; Reverb-X gates **before and after** reverb+OD. Distortion **before** verb in trickery; Reverb-X is **verb → OD**.

### Our routing (correct):
```
wet → preGate → reverb → odBlend → postGate
dry → parallel sum
```

### Saturation recommendation
```cpp
// ~15 lines, no fork needed
float asymmetricTanh(float x, float drive) {
    x *= drive;
    if (x > 0) x *= 1.12f;
    return std::tanh(x) / std::tanh(drive);
}
```
Study `cubicnl` in Faust for alternative grit.

### MB mapping: `MB-025`–`MB-026` — greenfield, cite trickery for wet-path serial FX concept only.

---

## R5 — Send / expression UI

### Repos evaluated

| Repo | Send model | UI | Trails on release | Fork score |
|------|------------|-----|-----------------|------------|
| **None found** | Igor-equivalent | — | — | — |
| [Chowdhury-DSP/chowdsp_utils](https://github.com/Chowdhury-DSP/chowdsp_utils) GUI modules | — | fancy widgets | — | **2/5** |
| BYOD | momentary switches | custom | — | **2/5** |
| melatonin_inspector | debug only | — | — | dev tool |

### Implementation spec (greenfield)
- `PressureSendPad : juce::Component`
- `mouseDown` → `igorSend` 0→1 by `mouseDrag` Y or pressure
- `mouseUp` → `igorSend` → 0 with 25 ms smoother; **do not clear** reverb buffers
- `igor_connected` bool: when true, wet path *= send; when false, send ignored (=1)
- MIDI CC1 → same parameter

### MB mapping: `MB-040`–`MB-044`, `MB-063` — all greenfield.

---

## R6 — Competitive open-source landscape

| Project | Reverb-X similarity (0–10) | License | Notes |
|---------|------------------------------|---------|-------|
| **Reverb-X hardware** | 10 | — | reference |
| [reverb_trickery](https://github.com/unicornsasfuel/reverb_trickery) | 5 | BSD-3 | gated+distorter+verb; wrong order |
| [CleanVerb](https://github.com/Aizhee/CleanVerb) | 4 | — | ducking wet, not chop |
| [shredverb](https://github.com/nvssynthesis/shredverb) | 3 | — | FDN grit, no gate toggle |
| [RSAlgorithmicVerb](https://github.com/reillypascal/RSAlgorithmicVerb) | 2 | GPL | hi-fi verbs |
| [PinkGuitarFX](https://github.com/twoonemoon/PinkGuitarFX) | 2 | — | naive gate |
| DV-1 / Dirty Dog | 6–7 | closed | commercial reference |

### Gaps (must build ourselves)
1. Wet-only post-verb OD with dry always clean
2. Dual gate pre/post toggle
3. Momentary send with decaying trails
4. Two-mode bright/dark simple verb

---

## R7 — Testing & CI

### Repos evaluated

| Repo | Catch2 | pluginval | DSP offline tests | Fork score |
|------|--------|-----------|-------------------|------------|
| [sudara/pamplejuce](https://github.com/sudara/pamplejuce) | ✓ | ✓ strict | basic instance tests | **5/5** |
| [anthonyalfimov/JUCE-CMake-Plugin-Template](https://github.com/anthonyalfimov/JUCE-CMake-Plugin-Template) | ✗ | ✓ Validation.yml | no | **3/5** |
| [Chowdhury-DSP/chowdsp-ergo](https://github.com/Chowdhury-DSP/chowdsp-ergo) | — | — | regression WAV tests | **4/5** study |

### Test patterns to implement

| Test | Method |
|------|--------|
| RT60 | Impulse → process 3s silence → measure −60 dB time |
| Pre-gate | Hum tone below threshold → output silent |
| Post-gate | Burst → silence → wet < −60 dB in 50 ms |
| Dry clean | Sine through distn=1 → dry THD unchanged |
| Send trails | send 1→0 → wet buffer energy decays |
| No alloc | `processBlock` × 10000 under malloc debug |

Copy CMake test target from **pamplejuce** `tests/CMakeLists.txt` pattern.

---

## R8 — Parameters & smoothing

### Repos evaluated

| Repo | Param system | Smoothing | Curves | Fork score |
|------|--------------|-----------|--------|------------|
| [chowdsp_utils SimpleReverb](https://github.com/Chowdhury-DSP/chowdsp_utils) | `chowdsp::ParamHolder` + typed params | Multiplicative smoothers | `createNormalisableRange` skew | **5/5** |
| [BYOD ParameterHelpers](https://github.com/Chowdhury-DSP/BYOD) | `createPercentParameter` macros | per-block | — | **4/5** |
| JUCE `dsp::DryWetMixer` | built-in | — | equal-power option | **5/5 use** |

### Curve implementation

| Param | Implementation |
|-------|----------------|
| level | `DryWetMixer` or `sin/cos` equal-power |
| size | `NormalisableRange` with `skew = 0.5f` approximating `pow(x, 2.4)` |
| distn | `std::pow(norm, 2.8f)` at DSP read |
| input | Split: `lerp(+9,-3,smoothstep)` gain + `lerp(-52,-18,pow(x,1.6))` thresh |

### MB mapping: `MB-010`–`MB-014`

---

## Cloned samples (local)

```
.planning/repo-samples/
  sudara-pamplejuce/              ← FORK BASE
  anthonyalfimov-JUCE-CMake-Plugin-Template/
  Chowdhury-DSP-chowdsp_utils/
  Chowdhury-DSP-BYOD/
  reillypascal-RSAlgorithmicVerb/
  unicornsasfuel-reverb_trickery/
  twoonemoon-PinkGuitarFX/
```

---

## Micro-build research gate status

| Lane | Status | MB unblocked |
|------|--------|--------------|
| R1 Scaffold | ✅ | MB-002, MB-004–006 |
| R2 Reverb | ✅ | MB-030–036 |
| R3 Gate | ✅ | MB-020–024 |
| R4 OD | ✅ | MB-025–026 |
| R5 Send | ✅ (greenfield) | MB-040–044, MB-063 |
| R6 Product | ✅ | MB-000 naming |
| R7 Test | ✅ | MB-070–074 |
| R8 Params | ✅ | MB-010–014 |

**Next human/agent action:** Sign ADR-001 (fork pamplejuce), ADR-002 (chowdsp FDN-inspired engine), then execute MB-003.
