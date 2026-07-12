# SendBloom / Reverb-X Plugin — Micro-Build Plan

**Status:** Pre-code research phase  
**Rule:** No plugin source until every micro-build has a **reference repo** or **explicit build-from-scratch** decision.

---

## How to use this document

Each **MB-###** is an atomic deliverable with:
- **Goal** — one sentence
- **Depends on** — upstream MB IDs
- **Research lane** — which GitHub investigation covers it
- **Accept** — definition of done (no code until Accept criteria are met in research)

Research lanes map to parallel investigators:
| Lane | ID | Topic |
|------|-----|--------|
| R1 | scaffold | JUCE CMake template, CI, formats |
| R2 | reverb | FDN / algorithmic verb |
| R3 | gate | Envelope + dual gate |
| R4 | od | Wet-only saturation |
| R5 | send | Igor / momentary send UI |
| R6 | product | Gated dirty reverb landscape |
| R7 | test | Catch2, pluginval, RT60 tests |
| R8 | params | APVTS curves, smoothing |

---

## Phase 0 — Repository & legal (no DSP)

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-000 | Choose product name + manufacturer code (4-char) | — | R6 naming | Name cleared; JUCE codes reserved |
| MB-001 | License decision (GPL vs JUCE Indie commercial) | MB-000 | R1 | Written choice + budget if commercial |
| MB-002 | Fork vs greenfield decision | MB-001 | R1 | One base repo selected OR "greenfield + copy patterns from X,Y,Z" |
| MB-003 | `.gitignore`, `LICENSE`, `README` skeleton | MB-002 | R1 | Repo init without plugin binary |
| MB-004 | CMake + JUCE fetch (no DSP) | MB-002 | R1 | Empty plugin loads in DAW as passthrough |
| MB-005 | GitHub Actions: build matrix macOS/Windows | MB-004 | R1, R7 | CI green on passthrough |
| MB-006 | pluginval in CI (strictness 5 initially) | MB-005 | R7 | CI runs pluginval on artifact |

---

## Phase 1 — Parameter system (no audio character yet)

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-010 | `ParameterIDs.h` — all IDs enumerated | MB-004 | R8 | Matches parameter map doc |
| MB-011 | `createParameterLayout()` — floats, bools, enums | MB-010 | R8 | All params visible in host |
| MB-012 | Skew curves: level (equal-power), size (exp), distn (pow) | MB-011 | R8 | Unit test: norm→value at 0, 0.5, 1 |
| MB-013 | Input dual mapping: gain dB + gate threshold dB | MB-011 | R3, R8 | Documented formula + test |
| MB-014 | `SmoothedValue` per continuous param | MB-011 | R8 | Zipper test: automate size in REAPER |
| MB-015 | Bypass with 5 ms crossfade | MB-004 | R1 | No click on bypass toggle |
| MB-016 | Preset XML: 8 factory presets as resources | MB-011 | R1 | Load/save round-trip |

---

## Phase 2 — DSP atoms (offline-tested, not chained)

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-020 | `EnvelopeDetector` peak + optional RMS | — | R3 | Impulse response matches attack/release spec |
| MB-021 | `NoiseGate` soft expander mode | MB-020 | R3 | −90 dB floor; hysteresis 3 dB |
| MB-022 | `NoiseGate` hard gate mode | MB-020 | R3 | Tail to silence <50 ms; **Authentic: no user release** (manual: chop not adjustable) |
| MB-023 | Pre-gate profile (150 ms release) | MB-021 | R3 | Hum burst suppressed in silence |
| MB-024 | Post-gate profile (65 ms release) | MB-022 | R3 | Sustained note passes; staccato chops |
| MB-025 | `WetOverdrive` fixed asymmetric tanh | — | R4 | THD curve documented; 2× OS deferred |
| MB-026 | OD blend via `distn^2.8` | MB-025 | R4 | At 0.5 norm, ~7% wet OD mix |
| MB-027 | `ParallelMixer` equal-power dry/wet | MB-012 | R8 | 0.5 mix = −3 dB perceived, not +3 dB |
| MB-028 | `InputStage` gain + clip flag | MB-013 | R4 | LED triggers at −3 dBFS |
| MB-029 | `OutputStage` makeup gain | MB-011 | R1 | Unity at default |

---

## Phase 3 — Reverb engine

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-030 | Choose reverb core (fork vs write) | — | R2 | Written ADR with repo link |
| MB-031 | `FDNReverb` mono, 8–12 lines | MB-030 | R2 | Impulse decay measurable |
| MB-032 | Size → RT60 mapping `0.25 + 5.75×s^2.4` | MB-031 | R2 | RT60 test ±15% at size 0.25, 0.5, 1.0 |
| MB-033 | Bright mode: immediate, HF damping light | MB-031 | R2 | A/B wav vs spec |
| MB-034 | Dark mode: 55 ms predelay + darker damping | MB-031 | R2 | Predelay visible in impulse |
| MB-035 | Dark/Bright crossfade 15 ms | MB-033,034 | R2 | No click on toggle |
| MB-036 | Modulated delay lines (subtle) | MB-031 | R2 | No static metallic ring |

---

## Phase 4 — Send controller (Igor)

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-040 | `SendController` — send 0–1 curve | MB-011 | R5 | Curve matches `smoothstep^1.85` |
| MB-041 | `igor_connected` — wet multiply vs always-on | MB-040 | R5 | Inserted=0 → dry at rest |
| MB-042 | Release: dry instant, tank keeps decaying | MB-031, MB-040 | R5 | Energy in tail 500 ms after release |
| MB-043 | Firm vs soft sensitivity exponents | MB-040 | R5 | Two curves documented |
| MB-044 | MIDI CC1 → send | MB-040 | R5 | Mod wheel works in host |

---

## Phase 5 — Signal chain integration

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-050 | `ReverbXSignalChain` wire: in→preGate→send→verb | MB-023,031,040 | R6 | Offline impulse test |
| MB-051 | Add wet OD + distn blend | MB-050, MB-026 | R4 | Dry path THD unchanged |
| MB-052 | Add post-gate branch (gate toggle) | MB-050, MB-024 | R3 | Pre vs post A/B documented |
| MB-053 | Parallel dry + level mix + output | MB-050, MB-027, MB-029 | R8 | Full chain matches signal diagram |
| MB-054 | `processBlock` RT-safe audit | MB-053 | R7 | No alloc under stress test |
| MB-055 | Mono in/out contract | MB-053 | R1 | Stereo bus safe (dual-mono v2) |

---

## Phase 6 — UI (after DSP validated offline)

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-060 | `LookAndFeel` — boutique mini-pedal aesthetic | MB-004 | R5 | Style guide screenshot |
| MB-061 | Five knobs + APVTS attachments | MB-011 | R1 | All automate in host |
| MB-062 | Dark + Gate toggles | MB-011 | R1 | Match hardware labels |
| MB-063 | `PressureSendPad` component | MB-040 | R5 | Mouse + touch; visual bloom |
| MB-064 | Clip LED | MB-028 | R1 | Holds 50 ms |
| MB-065 | Preset bar | MB-016 | R1 | Factory + user paths |
| MB-066 | Tooltips: explain pre vs post gate | MB-062 | R6 | One-click education |

---

## Phase 7 — Quality & ship

| ID | Goal | Depends | Research | Accept |
|----|------|---------|----------|--------|
| MB-070 | Catch2: gate tests | MB-024 | R7 | CI green |
| MB-071 | Catch2: RT60 tests | MB-032 | R7 | CI green |
| MB-072 | Catch2: parallel dry clean | MB-051 | R7 | CI green |
| MB-073 | Catch2: send trails | MB-042 | R7 | CI green |
| MB-074 | pluginval strictness 10 | MB-054 | R7 | CI green |
| MB-075 | Hardware A/B protocol (if unit available) | MB-053 | — | Tuning notes doc |
| MB-076 | AU validation (auval) macOS | MB-004 | R7 | Logic scan pass |
| MB-077 | Code signing checklist | MB-074 | — | Release doc |

---

## Phase 8 — Post-MVP (explicitly later)

| ID | Goal |
|----|------|
| MB-080 | CLAP format |
| MB-081 | Stereo I/O (**Extended mode**) |
| MB-082 | 2× OD oversampling (**Extended mode**) |
| MB-083 | MIDI learn UI |
| MB-084 | AAX |
| MB-085 | FV-1 / 32 kHz coloration option (Schroeder tank A/B) |
| MB-086 | Re-amp + ESS hardware capture protocol |
| MB-087 | Marketing-aligned presets (Sparkle Verb, Cut Sample Gate, …) |

---

## Dependency graph (critical path)

```
MB-002 → MB-004 → MB-011 → MB-050 → MB-053 → MB-054 → MB-060 → MB-074
              ↘ MB-020 → MB-023 ─┘
              ↘ MB-030 → MB-031 ─┘
              ↘ MB-025 ──────────┘
              ↘ MB-040 ──────────┘
```

---

## Research output merge checklist

Completed 2026-07-06 — see `RESEARCH_CORPUS.md`, `ADR-001`, `ADR-002`.

- [x] R1 scaffold → **Fork sudara/pamplejuce** (ADR-001)
- [x] R2 reverb → **Custom 8-line FDN, study chowdsp SimpleReverb** (ADR-002)
- [x] R3 gate → **Adapt BYOD Gate.cpp FSM** (dual profiles)
- [x] R4 od → **Greenfield asymmetric tanh; study reverb_trickery**
- [x] R5 send → **Greenfield PressureSendPad** (no OSS Igor)
- [x] R6 product → **No OSS ≥7/10 match; gaps documented**
- [x] R7 test → **Copy pamplejuce Catch2 + pluginval CI**
- [x] R8 params → **APVTS + DryWetMixer + pow/skew curves**

### Cloned reference repos (local)

`.planning/repo-samples/` — shallow clones for code inspection before MB-002.

---

## Files to create (after research merge, not before)

```
.planning/BUILD_MICROSTEPS.md     ← this file
.planning/RESEARCH_R1_SCAFFOLD.md
.planning/RESEARCH_R2_REVERB.md
... (one per lane)
.planning/ADR-001-fork-decision.md
.planning/ADR-002-reverb-engine.md
```

Plugin `Source/` only after **MB-002** and **MB-030** ADRs signed off.
