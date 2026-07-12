# Project Research Summary

**Project:** SendBloom v2.0 — Proper 32k SRC
**Domain:** Realtime bandlimited sample-rate conversion (host ↔ 32,768 Hz) for a JUCE 8 guitar FX plugin
**Researched:** 2026-07-08
**Confidence:** HIGH

## Executive Summary

SendBloom v2.0 is a surgical DSP milestone, not a greenfield product. The plugin already ships a gated-dirty-ambience wet chain with a "32k Color" toggle that routes through a broken accumulator/hold path — a non-bandlimited pseudo-SRC that produces a stable 14–15 kHz imaging whistle at 48 kHz host rate. Experts build this class of problem by separating rate-agnostic DSP (the Schroeder tank) from a bandlimited SRC bridge, then integrating both into a realtime block-processing path with explicit latency accounting. The recommended approach is r8brain-free-src 7.1 (MIT, arbitrary-ratio, linear-phase) wrapped in a `FixedRateAdapter` around an extracted `SchroederTankCore` running strictly at 32,768 Hz, with dual-engine crossfade on toggle and conditional PDC when ProperSRC is active.

The architecture is well-defined: extract the monolithic `SchroederTank32` into core + host-rate wrapper + fixed-rate adapter; extend `IReverbEngine` with `processBlock()`; move reverb+SRC to block processing inside `GatedBloomChain` while keeping gates/send/OD per-sample; and gate user-facing enablement behind existing HF acceptance tests (`HighFrequencyRingingDiagnosticsTest`). RC1 safety requires 32k Color off by default (`authentic_color=0` on all presets) until ProperSRC passes acceptance gates — the broken accumulator path must never ship as production bridge.

Key risks are integration-shaped, not library-shaped. The top hazards are: (1) mistaking the accumulator for real SRC, (2) breaking v1's zero-latency contract without an ADR-003 PDC policy, (3) heap allocation or per-sample r8brain calls on the audio thread, (4) instant engine switching causing clicks, and (5) buffer sizing mismatches across variable host block sizes. Mitigation is strict phase ordering — core extraction before SRC glue, block integration before crossfade, diagnostics before enablement — plus the existing three-path harness (Off / LegacyAccumulator / ProperSRC) to prove the fix architecturally.

## Key Findings

### Recommended Stack

r8brain-free-src 7.1 is the clear winner for the host ↔ 32,768 Hz bridge. It handles non-integer ratios (44.1/48/88.2/96 kHz), exposes `getLatency()` for PDC, ships MIT-clean, and is already the project's ADR-track choice. JUCE `dsp::Oversampling` is explicitly wrong here (integer powers of 2 only). libsamplerate and SpeexDSP are test-only fallbacks if r8brain fails acceptance after tuning. Integration is header-only via CPM with a thin INTERFACE target; resamplers construct in `prepare()` only, using `double` scratch buffers.

**Core technologies:**
- **r8brain-free-src 7.1** (pin `e71c31bf`): arbitrary-ratio bandlimited SRC — handles 32768↔host non-integer ratios with explicit latency API
- **JUCE 8 `juce_dsp`**: existing delay/comb/APF core and `ProcessSpec` — no new framework; tank stays fixed-rate after extraction
- **C++20 + CMake/CPM 0.42.0**: matches repo standard; vendor r8brain as INTERFACE dependency per pamplejuce pattern
- **Catch2 3.8.1**: three-path HF imaging harness and latency regression — tests already define acceptance contract

### Expected Features

v2.0 replaces a broken DSP path with a trustworthy one. Users enabling 32k Color expect FV-1-class coloration (darker damping, 9-bit quantization, fixed delay table), not a glass whistle. The toggle must be audibly distinct from host-rate, click-free, and HF-clean. Engineering differentiators — block `processBlock()`, three-path diagnostics, conditional PDC — support validation and regression safety, not user-facing complexity.

**Must have (table stakes):**
- Bandlimited hostRate ↔ 32,768 Hz SRC bridge (`FixedRateAdapter` + r8brain) — fixes 14–15 kHz imaging
- True fixed-rate `SchroederTankCore` at 32,768 Hz — unscaled delay table, no host-rate branches inside core
- HF imaging regression pass — existing `[hf][ringing][regression]` tests green at 48 kHz (extend to 44.1 kHz before user enable)
- 20–50 ms engine crossfade on `authentic_color` toggle — prevents clicks from incompatible engine states
- RC1 safety — 32k Color off by default; all presets `authentic_color=0` until acceptance gates pass
- Realtime-safe processing — zero heap/locks/logging in `processBlock()` after `prepare()`

**Should have (competitive):**
- Three-path diagnostics harness (Off / LegacyAccumulator / ProperSRC) — proves architectural fix, not band-aid
- Block-level `IReverbEngine::processBlock()` — efficient r8brain pull loop, clean adapter boundary
- ADR-003 latency/PDC policy — measured SRC latency with conditional `setLatencySamples()` when ProperSRC active
- Remove/demote anti-image SVF band-aid — after ProperSRC passes HF tests; authentic brightness provides natural rolloff

**Defer (v2+):**
- User-facing SRC quality/latency controls — musicians don't need SRC knobs; fixed internal tier
- Wet OD oversampling — orthogonal Extended-mode concern; OD stays at host rate
- Extended stereo decorrelator — v1 ships mono-first; dual-mono out preserved
- Preset refresh with `authentic_color=1` — only after HF acceptance and ADR-003 resolved

### Architecture Approach

Split `SchroederTank32` into rate-agnostic core and bandlimited adapter. Keep per-sample wet chain shell (gates, send, OD) but move reverb+SRC to block processing. Use dual-engine crossfade when `authentic_color` toggles. Report SRC latency conditionally via `setLatencySamples()` only when ProperSRC is active — RC1 ships with 32k off, so latency stays 0 for release candidate.

**Major components:**
1. **`SchroederTankCore`** — single-rate Schroeder tank at 32,768 Hz; fixed delay table; no `hostRate`/`useAuthenticPath` branches
2. **`FixedRateAdapter` + `RateConverterPair`** — r8brain upsample → core → downsampler sandwich; owns buffer sizing and latency query
3. **`HostRateReverbEngine`** — thin wrapper scaling delays for host-rate path (RC1 primary)
4. **`SchroederTank32` (facade)** — routes host vs authentic; retains `LegacyAccumulator` behind `Authentic32Mode` for diagnostics only
5. **`EngineCrossfade`** — 20–50 ms wet output mix during engine swap; dual-run only during fade window
6. **`GatedBloomChain::processBlock`** — block wet reverb stage with scratch buffers; per-sample gates/OD preserved

### Critical Pitfalls

1. **Accumulator/hold mistaken for SRC** — structural 14–15 kHz imaging; anti-image SVF masks but doesn't fix. Keep accumulator as `LegacyAccumulator` diagnostic only; ship ProperSRC or keep 32k off.
2. **Zero-latency contract break without PDC policy** — r8brain adds group delay; reporting 0 while SRC active misaligns parallel wet/dry. Measure at 44.1/48/96 kHz; decide ADR-003 before user enablement.
3. **Heap allocation on audio thread** — construct `CDSPResampler` in `prepare()` only; pre-size buffers via `getMaxOutLen()`; fix existing `dryBuffer.setSize` hazard in same milestone.
4. **Per-sample r8brain calls** — async pull model requires block `processBlock()`; per-sample wrapper causes underrun, CPU blowup, wrong tank stepping.
5. **Instant engine switch without crossfade** — incompatible delay states cause clicks; run dual engines in parallel during 20–50 ms fade, reset idle engine after completion.
6. **Buffer sizing mismatch** — non-integer 32768/host ratio means internal buffers may exceed host block; size at prepare with margin and maintain leftover FIFOs between blocks.

## Implications for Roadmap

Based on research, suggested phase structure:

### Phase 0: RC1 Safety Freeze
**Rationale:** Prevents user exposure to accumulator HF bug while v2.0 work proceeds; zero dependency on SRC implementation.
**Delivers:** All factory presets `authentic_color=0`; production default `Authentic32Mode::Off`; host-rate path remains primary.
**Addresses:** RC1 safety, honest product documentation (32k off until validated)
**Avoids:** Enabling 32k Color before ProperSRC passes gates (UX pitfall); preset regression

### Phase 1: SchroederTankCore Extraction
**Rationale:** Must precede all SRC work — entangled host/authentic branches in monolithic class cause double-scaling and untestable boundaries (Pitfall 7).
**Delivers:** `SchroederTankCore` with fixed 32,768 Hz delay table; `HostRateReverbEngine` wrapper; unit tests proving RT60/impulse parity at same rate.
**Addresses:** True fixed-rate tank, `SchroederTankCore` single-rate extraction (differentiator)
**Avoids:** Host/fixed rate entanglement; subtle delay/RT60 errors in ProperSRC path

### Phase 2: FixedRateAdapter + r8brain Integration
**Rationale:** Core exists; now wrap with bandlimited SRC. r8brain CMake + `RateConverterPair` are prerequisites for adapter.
**Delivers:** CPM vendored r8brain 7.1; `RateConverterPair` with prepare-time allocation; `FixedRateAdapter` SRC sandwich (no crossfade yet); offline round-trip + HF imaging < legacy accumulator at 48 kHz.
**Uses:** r8brain `CDSPResampler` up/down; `double` scratch buffers; `getLatency()` query
**Implements:** `FixedRateAdapter`, `RateConverterPair` architecture components
**Avoids:** Accumulator mistaken for SRC (Pitfall 1); heap alloc on audio thread (Pitfall 3); buffer underrun/overrun (Pitfall 6)

### Phase 3: Block-Level Integration
**Rationale:** r8brain requires block pull processing; cannot bolt onto per-sample `GatedBloomChain` loop (Pitfall 4).
**Delivers:** `IReverbEngine::processBlock()` extension; `Fdn8Reverb` stub (loop `processSample`); `GatedBloomChain::processBlock` with wet scratch buffers; `PluginProcessor` block chain call; full ctest suite green.
**Addresses:** Block-level `processBlock()`, realtime-safe processing
**Avoids:** Per-sample SRC shim; sample-by-sample r8brain calls

### Phase 4: Three-Path Diagnostics Harness
**Rationale:** Proves ProperSRC architecturally fixes imaging before any user enablement; provides regression evidence for ADR-003.
**Delivers:** `Authentic32Mode` enum (Off / LegacyAccumulator / ProperSRC); `AuthenticPathDiagnosticsTest` with HF metrics CSV; legacy fails imaging gates, proper passes.
**Addresses:** Three-path diagnostics harness (engineering differentiator), HF imaging regression pass
**Avoids:** "Looks done but isn't" — steady-state HF pass without verifying upsample bandlimiting

### Phase 5: Engine Crossfade
**Rationale:** ProperSRC wired but toggle still clicks without dual-engine fade (Pitfall 5); depends on both engines existing from Phases 1–3.
**Delivers:** `EngineCrossfade` with 20–50 ms equal-power ramp; dual-run only during fade; toggle stress test (no click, max delta threshold).
**Addresses:** Click-free 32k Color toggle
**Avoids:** Instant delay-length switch; mode-switch clicks during performance

### Phase 6: Latency/PDC + ADR-003
**Rationale:** SRC adds measurable group delay; policy must be decided before marketing 32k Color production-ready (Pitfall 2, 8).
**Delivers:** Impulse/cross-correlation measurement at 44.1/48/88.2/96 kHz; ADR-003 documenting conditional PDC policy; updated `LatencyTest` (mode-aware); `setLatencySamples(roundTripSrc)` when ProperSRC active.
**Addresses:** Measured latency with documented PDC policy
**Avoids:** Fake zero-latency with hidden SRC delay; gate/send timing desync in parallel mix

### Phase 7: Enablement + Documentation
**Rationale:** All acceptance gates green; safe to expose 32k Color to users and update truth docs.
**Delivers:** HF tests pass at 44.1 + 48 kHz; VERB-05/RELEASE_CHECKLIST updated (ProperSRC, not accumulator); optional anti-image SVF removal; multi-host-rate CI matrix; preset refresh with `authentic_color=1`.
**Addresses:** 32k Color user enablement, honest product documentation, audibly distinct authentic path
**Avoids:** RC1 preset regression; shipping dull authentic path post-LPF band-aid

### Phase Ordering Rationale

- **RC1 freeze first** — no user-facing risk while architecture work proceeds in parallel
- **Core extraction before SRC** — eliminates entangled host/authentic branches that cause subtle double-scaling bugs impossible to debug after SRC glue is added
- **SRC adapter before block integration** — adapter can be unit-tested offline; block refactor is the largest integration diff and needs a working adapter to validate
- **Diagnostics before crossfade** — proves ProperSRC fixes imaging independently of toggle UX polish
- **Crossfade before PDC** — both engines must run correctly before measuring end-to-end latency through the full chain
- **PDC before enablement** — ADR-003 is a hard gate for user-facing 32k Color; marketing cannot proceed without latency policy
- **Enablement last** — only after HF acceptance, PDC policy, and docs are complete

### Research Flags

Phases likely needing deeper research during planning:
- **Phase 2:** r8brain `ReqTransBand` latency/quality tradeoffs — measure actual latency at target ratios before ADR-003; may need `/gsd-plan-phase --research-phase 2` for filter knob tuning
- **Phase 6:** JUCE `setLatencySamples` behavior across VST3/AU hosts on sample-rate change — community guidance suggests `AsyncUpdater` quirks; validate in target DAWs

Phases with standard patterns (skip research-phase):
- **Phase 1:** Tank extraction follows established v1 decomposition precedent (`05-RESEARCH.md`); codebase-grounded
- **Phase 3:** `IReverbEngine` extension pattern already used by `Fdn8Reverb` swap tests
- **Phase 5:** Crossfade reuses existing `BypassCrossfade` / `SmoothedValue` patterns in codebase

## Confidence Assessment

| Area | Confidence | Notes |
|------|------------|-------|
| Stack | HIGH | r8brain + JUCE constraints verified against official docs; project already chose r8brain-first; latency numbers MEDIUM until measured on target ratios |
| Features | HIGH | Codebase-grounded requirements with existing acceptance tests; external FV-1 ecosystem claims LOW but not blocking |
| Architecture | HIGH | Detailed build order with component boundaries; matches v1 decomposition precedent; integration points mapped to existing files |
| Pitfalls | HIGH | Codebase-verified integration risks; external SRC/PDC guidance authoritative but not build-verified |

**Overall confidence:** HIGH

### Gaps to Address

- **Actual r8brain latency at 44.1/48/96 kHz:** Measure in Phase 2 prototype before ADR-003; `getLatency()` may return 0 for internal consumption — use impulse peak alignment as cross-check
- **CPU budget with dual-engine crossfade:** Profile 48 kHz / 512 block during Phase 5; enable `R8B_PFFFT_DOUBLE` only if HF tests pass and CPU tight
- **Gate/send timing with PDC:** Pluck-stop chop test in Phase 6–7; gate is input-keyed so mainly affects parallel mix bloom lag, not recorded alignment
- **Preset BinaryData migration:** Verify all factory presets `authentic_color=0` in RC1 freeze; currently some ship `auth=1`

## Sources

### Primary (HIGH confidence)
- SendBloom codebase — `SchroederTank32.h`, `GatedBloomChain.h`, `HighFrequencyRingingDiagnosticsTest.cpp`, `LatencyTest.cpp`, `RealtimeStressTest.cpp`
- `.planning/PROJECT.md` — v1 diagnosis, v2.0 Option C scope, r8brain-first ADR track
- [r8brain-free-src README](https://github.com/avaneev/r8brain-free-src/blob/master/README.md) — MIT, v7.1 header-only, realtime pull model, latency API
- [r8brain CDSPResampler reference](https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html) — `getLatency()`, `process()`, constructor params
- [JUCE dsp::Oversampling](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) — integer-ratio only; confirms unsuitability

### Secondary (MEDIUM confidence)
- [r8brain realtime latency discussion (issue #6)](https://github.com/avaneev/r8brain-free-src/issues/6) — `ReqTransBand` latency tradeoffs
- [libsamplerate API](https://libsndfile.github.io/libsamplerate/api_full.html) — test-only A/B reference
- [SpeexDSP resampler](https://github.com/xiph/speexdsp/blob/master/include/speex/speex_resampler.h) — fallback evaluation only
- `.planning/milestones/v1.0-phases/05-schroedertank32-reverb/05-RESEARCH.md` — tank decomposition precedent
- JUCE forum — `setLatencySamples` in `prepareToPlay`, sample-rate change quirks

### Tertiary (LOW confidence)
- Spin Semiconductor FV-1 knowledge base — 32768 Hz crystal, ~15 kHz bandwidth (behavioral reference, not implementation source)
- [CCRMA resampling survey](https://ccrma.stanford.edu/~jos/resample/Free_Resampling_Software.html) — ecosystem context

---
*Research completed: 2026-07-08*
*Ready for roadmap: yes*
