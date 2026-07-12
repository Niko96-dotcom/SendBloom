# Feature Research

**Domain:** Guitar FX plugin — proper fixed-rate 32.768 kHz reverb coloration via SRC bridge (SendBloom v2.0)
**Researched:** 2026-07-08
**Confidence:** MEDIUM (HIGH on codebase-grounded requirements; LOW on external SRC/FV-1 ecosystem claims — cross-checked against existing implementation and tests)

## Feature Landscape

### Table Stakes (Users Expect These)

Features users assume exist when enabling **32k Color**. Missing these = the toggle feels broken or dishonest.

| Feature | Why Expected | Complexity | Notes |
|---------|--------------|------------|-------|
| **Bandlimited hostRate ↔ 32,768 Hz SRC bridge** | Current accumulator/hold path produces stable 14–15 kHz imaging at 48 kHz host rate; users enabling 32k Color expect *coloration*, not a whistle | HIGH | Replace `processAuthentic()` accumulator + linear hold with `FixedRateAdapter` using r8brain (or equivalent windowed-sinc SRC). Must handle non-integer ratios (44.1, 48, 88.2, 96 kHz). |
| **True fixed-rate tank at 32.768 kHz** | "32k Color" implies the reverb delay network runs at the FV-1-class rate with unscaled delay table lengths | MEDIUM | Extract `SchroederTankCore` with no `hostRate` / `useAuthenticPath` branches inside. Fixed delays from `SchroederTank32DelayTable` (167/253/328/413 APFs, 1057–1202 combs, 677 tank AP). |
| **Audibly distinct from host-rate path** | Toggle must do something meaningful; existing `ReleaseTruthTest` requires distinct impulse response | LOW | Already tested (`maxDiff > 1e-4`). Proper SRC should preserve or strengthen the difference via authentic damping/quantization, not collapse toward host-rate. |
| **HF imaging suppressed (no glass whistle)** | v1 diagnosis: 14–15 kHz narrowband artifact is the #1 user-facing bug | HIGH | Must pass `HighFrequencyRingingDiagnosticsTest` acceptance gates: imaging band RMS < 0.0022 at 14825 Hz, narrowband dominance < 10:1, authentic 10k+ RMS within 1.4× of host-rate on guitar pluck. |
| **Click-free 32k Color toggle** | Guitarists toggle mid-performance; pops destroy trust | MEDIUM | 20–50 ms crossfade between host-rate engine and proper-SRC engine when `authentic_color` changes. Depends on dual-engine or crossfading wrapper, not in-place delay rescale. |
| **Preserves authentic color DSP traits** | Users who enable 32k Color expect the darker, grittier FV-1-class character already documented | LOW | Keep: `kAuthenticBrightDampingHz` (6500 vs 8000), 9-bit RT60/damping quantization, fixed delay table, per-comb RT60 feedback at 32k reference, tank LFO at 32k rate. |
| **Realtime-safe processing** | SendBloom is a live guitar pedal plugin | MEDIUM | Zero heap alloc, locks, logging, or file I/O in `processBlock()` after `prepare()`. SRC state pre-allocated; pull-based resampler loop bounded by block size. |
| **Works across common host sample rates** | DAWs run 44.1 / 48 / 88.2 / 96 kHz | MEDIUM | Arbitrary-ratio SRC required; integer-ratio-only shortcuts fail at 44.1 kHz. Existing tests use 48 kHz; extend acceptance matrix. |
| **32k Color off by default (RC1 safety)** | Broken legacy path must not ship as default | LOW | All presets `authentic_color=0`; host-rate tank is primary until ProperSRC passes acceptance gates. Already in PROJECT.md Active requirements. |
| **Honest product documentation** | Legal/clean-room boundary: software model, not firmware clone | LOW | Update VERB-05 truth docs when ProperSRC lands: describe bandlimited SRC, not accumulator stepping. No EEPROM/bytecode/hardware-clone claims. |

### Differentiators (Competitive Advantage)

Features that set proper 32k Color apart. Not required for a reverb toggle, but valuable for SendBloom's "edited sample" positioning.

| Feature | Value Proposition | Complexity | Notes |
|---------|-------------------|------------|-------|
| **SRC-correct coloration without anti-image band-aid** | Host-rate path is already HF-clean; authentic mode should add *musical* darkness (FV-1 ~15 kHz bandwidth), not an SVF mop-up at 12 kHz | MEDIUM | After ProperSRC passes HF tests, `kAuthenticAntiImageLpHz` SVF becomes optional or removable. Differentiator vs v1 accumulator hack. |
| **Three-path diagnostics harness** | Engineering confidence + future regression safety; proves the fix is architectural | MEDIUM | Internal `Authentic32Mode`: Off / LegacyAccumulator / ProperSRC. Automated HF imaging metrics (Goertzel tail scans) comparing all three paths. Not user-facing. |
| **Block-level `IReverbEngine::processBlock()`** | Efficient SRC + tank processing; cleaner than per-sample resampler calls in hot loop | MEDIUM | New interface method on `IReverbEngine`; host-rate wrapper keeps `processSample()` for compatibility. Fixed-rate path processes internal blocks at 32.768 kHz. |
| **`SchroederTankCore` single-rate extraction** | Clean architecture enables A/B engine swap, crossfade, and test isolation | MEDIUM | Core owns tank DSP only. `FixedRateAdapter` owns SRC up/down. `SchroederTank32` becomes thin host-rate wrapper + authentic adapter selector. |
| **Measured latency with documented PDC policy (ADR-003)** | Transparency for DAW users; distinguishes SendBloom from plugins that silently add delay | MEDIUM | Measure r8brain filter latency via `getLatency()` / `getLatencyFrac()`. Decide: report via `setLatencySamples()`, internal dry compensation only, or defer PDC to v2.1. v1 promised zero latency — policy decision is a feature, not an implementation detail. |
| **Authentic brightness as natural HF limit** | FV-1-class 15 kHz bandwidth is the *intended* color, not a bug | LOW | Spin Semi documents 32768 Hz → ~15 kHz passband. Proper SRC lets `kAuthenticBrightDampingHz` and tank dynamics provide the rolloff organically instead of a post-hoc 12 kHz LP. |
| **Seamless integration with gated-dirty-ambience chain** | 32k Color colors the reverb *before* wet OD and post-gate — the product's signature routing | LOW | `GatedBloomChain` order preserved: gate → send → reverb → overdrive → post-gate. SRC only wraps reverb tank input/output, not the full wet chain. |

### Anti-Features (Commonly Requested, Often Problematic)

Features that seem good but create problems for proper 32k SRC.

| Feature | Why Requested | Why Problematic | Alternative |
|---------|---------------|-----------------|-------------|
| **Accumulator/hold upsample (status quo)** | Simple, zero latency, already implemented | Creates stable 14–15 kHz imaging; linear hold is not bandlimited reconstruction | Proper SRC bridge — the entire v2.0 milestone exists to remove this |
| **Anti-image SVF as primary fix** | Already shipped; suppresses whistle audibly | Masks root cause; darkens authentic path beyond FV-1 intent; wrong cutoff (12 kHz) for musical rolloff | Fix SRC first; demote SVF to optional safety net or remove after acceptance |
| **Linear interpolation for rate conversion** | Low CPU, easy to code | Same failure mode as hold — introduces aliasing/imaging | Windowed-sinc / polyphase FIR (r8brain, JUCE half-band for integer stages) |
| **User-facing SRC quality / latency controls** | "Pro" plugins expose oversampling quality | Musicians don't understand SRC; adds UI clutter and test matrix explosion | Fixed internal quality tier chosen in ADR-003; document latency policy instead |
| **Oversampling the entire wet chain** | "Just oversample everything" | Gates, send, and wet OD don't need 32k; wastes CPU; OD aliasing is a separate Extended-mode concern | SRC bridge only around fixed-rate tank core |
| **Firmware/EEPROM matching or "hardware A/B" claims** | Marketing authenticity | Legal boundary; out of scope per PROJECT.md | Clean-room software model with documented behavioral targets |
| **Real-time engine swap without crossfade** | Instant toggle feels responsive | Delay table lengths, SRC state, and filter histories differ — causes clicks and level jumps | 20–50 ms equal-power or linear crossfade between engines |
| **Fake zero-latency reporting with hidden SRC delay** | Preserves v1 latency promise | DAW PDC breaks; parallel wet/dry alignment drifts; bypass pre-echo | ADR-003: either report true SRC latency or compensate internally and document tradeoff |
| **Extended stereo decorrelator in authentic path** | Stereo width | v1 ships mono-first; stereo is Extended-mode deferred | Mono tank → dual-mono out (existing `ReleaseTruthTest` behavior) |
| **Host-rate delay scaling inside authentic mode** | Reuse one tank for both paths | Defeats purpose of fixed 32k table; blurs authentic vs host-rate distinction | Separate cores: `SchroederTankCore` (fixed 32k) vs host-rate scaled tank |
| **SRC inside wet overdrive** | "More authentic distortion" | OD nonlinearity aliasing is orthogonal; FV-1 runs OD in analog domain before ADC | Keep `WetOverdrive` at host rate; only reverb tank gets 32k treatment |
| **Graphs / confidence meters / algorithm language in UI** | Transparency | PROJECT.md explicitly out of scope; confuses guitarists | Simple "32k Color" toggle in advanced drawer (existing UI) |

## Feature Dependencies

```
RC1 safety freeze (32k off default, presets=0)
    └──requires──> No user-facing change until acceptance gates pass

SchroederTankCore extraction
    └──requires──> constexpr delay table + atoms (already shipped)
    └──requires──> DampedComb/SchroederAllpass single-rate operation
    └──enables──> FixedRateAdapter, engine crossfade, three-path harness

FixedRateAdapter (r8brain hostRate ↔ 32,768 Hz)
    └──requires──> SchroederTankCore
    └──requires──> Block-level processBlock interface
    └──enables──> HF imaging fix, anti-image SVF removal

Block-level IReverbEngine::processBlock()
    └──requires──> IReverbEngine interface extension
    └──enables──> Efficient SRC pull loop, GatedBloomChain block adaptation

Three-path diagnostics harness
    └──requires──> SchroederTankCore + LegacyAccumulator path retained internally
    └──requires──> ProperSRC path via FixedRateAdapter
    └──enables──> Automated HF regression proofs, ADR-003 evidence

Safe engine crossfade (20–50 ms)
    └──requires──> Dual engine instances OR crossfading wrapper with shared params
    └──requires──> SmoothedParameterBank / APVTS authentic_color smoothing
    └──conflicts──> In-place delay rescale on toggle (current behavior in processSample)

HF acceptance tests (HighFrequencyRingingDiagnosticsTest)
    └──requires──> ProperSRC path
    └──blocks──> Enabling 32k Color for end users

Latency measurement + PDC policy (ADR-003)
    └──requires──> FixedRateAdapter prepared at known host rate
    └──conflicts──> v1 zero-latency promise (must decide before ship)

32k Color user enablement
    └──requires──> HF acceptance tests pass
    └──requires──> ADR-003 latency decision
    └──requires──> VERB-05 docs updated
    └──requires──> Preset migration (currently all auth=1 in BinaryData; RC1 needs auth=0)
```

### Dependency Notes

- **SchroederTankCore requires existing atoms:** Delay table, comb/APF primitives, RT60 mapping, and 9-bit quantization logic already exist in `SchroederTank32.h`. Extraction is refactor, not greenfield DSP.
- **FixedRateAdapter requires block processing:** r8brain `CDSPResampler` is async/pull-based; per-sample `process()` calls in the current `GatedBloomChain::processSample` loop would be inefficient and complicate state management. Block interface is a prerequisite, not a nice-to-have.
- **Engine crossfade conflicts with in-place toggle:** Current code flips `useAuthenticPath` and rescales delays mid-stream (`SchroederTank32::processSample` lines 67–72). Proper SRC needs parallel engines or a crossfading adapter — cannot bolt SRC onto the existing toggle mechanism.
- **HF tests block user enablement:** `HighFrequencyRingingDiagnosticsTest` defines the acceptance contract. Proper SRC is not done until these pass — not merely "sounds better."
- **Latency policy conflicts with v1 promise:** README states "zero reported latency." SRC filters add delay (r8brain `getLatency()` typically tens of samples). ADR-003 must resolve before marketing 32k Color as production-ready.
- **GatedBloomChain is the integration seam:** Reverb is one stage in the wet path. SRC adapter wraps only `IReverbEngine`, leaving gates, `PressureSend`, and `WetOverdrive` untouched at host rate.
- **Presets depend on RC1 freeze:** Factory presets currently ship `authentic_color=1` in BinaryData builds; RC1 requires all `authentic_color=0` before proper SRC is validated.

## MVP Definition

### Launch With (v2.0 — Proper SRC milestone)

Minimum to replace the broken accumulator path and pass acceptance gates.

- [ ] **SchroederTankCore** — Single-rate tank, no hostRate branches; fixed 32.768 kHz delay table
- [ ] **FixedRateAdapter** — Bandlimited down/up SRC (r8brain prototype) around core
- [ ] **Block-level `processBlock()`** — Fixed-rate path processes internal blocks; host-rate wrapper retained
- [ ] **HF imaging regression pass** — All `[hf][ringing][regression]` tests green at 48 kHz (extend to 44.1 kHz before user enable)
- [ ] **20–50 ms engine crossfade** — Click-free `authentic_color` toggle
- [ ] **Three-path diagnostics** — Off / LegacyAccumulator / ProperSRC with HF metrics output
- [ ] **ADR-003 latency decision** — Measured SRC latency + PDC policy documented
- [ ] **RC1 safety** — 32k Color off by default; presets `authentic_color=0`
- [ ] **VERB-05 doc update** — Describe ProperSRC, not accumulator

### Add After Validation (v2.0.x)

Features to add once ProperSRC passes acceptance and 32k Color is user-enabled.

- [ ] **Remove or bypass anti-image SVF** — After HF tests pass without it; A/B to confirm no regression
- [ ] **Multi-host-rate acceptance matrix** — 44.1, 48, 88.2, 96 kHz in CI HF tests
- [ ] **Preset refresh with 32k Color on** — Factory presets can re-enable `authentic_color=1` once validated
- [ ] **PDC implementation** — If ADR-003 chooses `setLatencySamples()`, implement in `PluginProcessor::prepareToPlay`

### Future Consideration (v3+ / Extended mode)

Defer until proper SRC is stable and product-market fit for 32k Color is confirmed.

- [ ] **SRC quality tier selector** — Only if CPU/latency tradeoffs demand user choice (Extended drawer)
- [ ] **Lower-latency SRC alternative** — If ADR-003 latency is unacceptable; e.g., relaxed `ReqTransBand` or polyphase IIR
- [ ] **Wet OD oversampling** — Separate from 32k reverb color; Extended-mode concern
- [ ] **Extended stereo decorrelator** — Orthogonal to authentic mono-first path
- [ ] **CLAP/AAX format support** — Post-v1 platform expansion

## Feature Prioritization Matrix

| Feature | User Value | Implementation Cost | Priority |
|---------|------------|---------------------|----------|
| Bandlimited SRC bridge (FixedRateAdapter) | HIGH | HIGH | P1 |
| SchroederTankCore extraction | HIGH | MEDIUM | P1 |
| HF imaging regression pass | HIGH | LOW (tests exist) | P1 |
| Block-level processBlock | MEDIUM | MEDIUM | P1 |
| Engine crossfade on toggle | HIGH | MEDIUM | P1 |
| RC1 safety (32k off default) | HIGH | LOW | P1 |
| Three-path diagnostics harness | LOW (user) / HIGH (eng) | MEDIUM | P1 |
| ADR-003 latency/PDC policy | MEDIUM | LOW | P1 |
| Remove anti-image SVF band-aid | MEDIUM | LOW | P2 |
| Multi-host-rate CI matrix | MEDIUM | LOW | P2 |
| Preset refresh (auth=1) | MEDIUM | LOW | P2 |
| PDC via setLatencySamples | MEDIUM | MEDIUM | P2 |
| User-facing SRC quality control | LOW | HIGH | P3 |
| Wet OD oversampling | LOW | HIGH | P3 |

**Priority key:**
- P1: Must have for v2.0 Proper SRC milestone
- P2: Should have, add when P1 passes acceptance
- P3: Nice to have, future consideration

## Competitor Feature Analysis

| Feature | Host-rate Schroeder (typical plugin) | FV-1 hardware (reference) | SendBloom v1 (accumulator) | SendBloom v2 (Proper SRC target) |
|---------|--------------------------------------|----------------------------|---------------------------|----------------------------------|
| Internal reverb rate | Host rate (44.1–96 kHz) | Fixed 32.768 kHz | Pseudo-32k via accumulator | True 32.768 kHz via SRC bridge |
| HF behavior | Full host Nyquist | ~15 kHz analog bandwidth | 14–15 kHz imaging whistle | ~15 kHz musical rolloff, no imaging |
| Rate conversion | N/A (runs at host rate) | Fixed crystal | Linear hold + 12 kHz SVF | Bandlimited sinc (r8brain) |
| Delay table | Scaled to host rate | Fixed sample counts | Fixed counts (correct) | Fixed counts (correct) |
| Parameter quantization | Continuous | ~9-bit pot resolution | 9-bit in authentic path | 9-bit preserved |
| Latency | Minimal | ADC/DAC filter delay | Zero reported | TBD (ADR-003) |
| Gated dirty ambience routing | Rare / none | Analog pedal topology | Full chain (moat) | Unchanged — SRC is reverb-only |

No OSS competitor ships gated-dirty-ambience + proper 32k SRC. Northern Valley DV-1 and claytonyen/gated-reverb-distortion are behavioral references, not SRC architecture sources. SendBloom's differentiator remains routing topology; proper SRC makes the 32k Color toggle trustworthy.

## Sources

- SendBloom codebase: `source/SchroederTank32.h`, `source/SchroederTank32DelayTable.h`, `source/GatedBloomChain.h`, `tests/HighFrequencyRingingDiagnosticsTest.cpp`, `tests/ReleaseTruthTest.cpp`, `docs/RELEASE_CHECKLIST.md` (VERB-05) — **HIGH confidence**
- `.planning/PROJECT.md` v2.0 milestone requirements and v1.0 diagnosis — **HIGH confidence**
- Spin Semiconductor FV-1 knowledge base (32768 Hz crystal, 15 kHz bandwidth) — **LOW confidence** (web)
- r8brain-free-src documentation (CDSPResampler, latency API, realtime pull pattern) — **LOW confidence** (web)
- JUCE AudioProcessor `setLatencySamples()` documentation — **LOW confidence** (web)
- JUCE `dsp::Oversampling` (integer-ratio pattern; not primary choice for 32768↔host) — **LOW confidence** (web)

---
*Feature research for: SendBloom v2.0 Proper 32k SRC*
*Researched: 2026-07-08*
