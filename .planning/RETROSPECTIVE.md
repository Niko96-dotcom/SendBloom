# Project Retrospective

*A living document updated after each milestone. Lessons feed forward into future planning.*

## Milestone: v2.0 — Proper 32k SRC

**Shipped:** 2026-07-09
**Phases:** 8 | **Plans:** 29 | **Tasks:** 78

### What Was Built

- RC1 safety freeze: 32k Color off by default, all presets `authentic_color=0`
- `SchroederTankCore` extraction with `HostRateReverbEngine` wrapper and RT60 parity
- `FixedRateAdapter` r8brain bandlimited hostRate ↔ 32,768 Hz SRC sandwich
- Block-level reverb+SRC integration in `GatedBloomChain` preserving v1 gate/send/OD behavior
- Three-path HF diagnostics harness proving ProperSRC fixes accumulator imaging
- Click-free 35 ms dual-engine crossfade on 32k Color toggle
- ADR-003 measured SRC latency with Policy A conditional host PDC
- 192 Catch2 tests, pluginval 10, ENAB composite acceptance gates

### What Worked

- Surgical eight-phase sequencing kept RC1 safe while building ProperSRC in parallel on feature branch
- Three-path diagnostics gave objective HF imaging proof before user enablement
- Block-level integration with per-sample gate/send/OD split preserved v1 routing parity
- Composite bash gate runner (ENAB-01) caught upstream prerequisite regressions in one command
- r8brain-first choice delivered multi-rate support with MIT license and measurable latency API

### What Was Inefficient

- Nyquist VALIDATION.md not kept current across phases 11–18 — retroactive fill needed
- Phase 16/17 human verification deferred to closeout — runtime toggle behaviors lack Catch2 coverage
- VALIDATION.md for Phase 12 marked non-compliant despite passing verification

### Patterns Established

- `Authentic32Mode` enum (Off / LegacyAccumulator / ProperSRC) for diagnostics without user exposure
- Shared `HfDiagnosticsHelpers` preserving Goertzel step sizes across TEST-11 and DIAG suites
- Two-phase `GatedBloomChain::processBlock` split: per-sample gate/send/OD, block reverb+SRC
- Policy A PDC: zero reported latency when 32k off, measured SRC round-trip when on

### Key Lessons

1. Prove SRC architecturally (three-path HF metrics) before enabling user-facing 32k Color
2. Engine crossfade must be wired before latency/PDC — toggle edges affect reported latency
3. Keep VALIDATION.md current per phase to avoid Nyquist audit gaps at milestone close

### Cost Observations

- v2.0 executed in ~2 days after v1.0 (2026-07-06 → 2026-07-09)
- 29 plans across 8 phases; fastest plans 3–5 min, integration phases 18–22 min
- Closeout override needed for 2 human_needed verification gaps (phases 16, 17)

---

## Milestone: v1.0 — SendBloom

**Shipped:** 2026-07-06
**Phases:** 10 | **Plans:** 36

### What Was Built

- Complete gated dirty ambience AU/VST3 plugin from pamplejuce scaffold
- SchroederTank32 reverb, WetOverdrive, dual gates, pressure send, pedal UI, 8 presets
- 103 Catch2 tests, pluginval 10, legal metadata guardrails

### What Worked

- Routing-first "ugly chain" in Phase 3 proved product behavior before DSP polish
- Vertical phase slicing produced loadable artifacts at every step
- ParameterSnapshot + SmoothedValue bank eliminated zipper noise from day one

### Key Lessons

1. The routing topology is the product moat — prove it audible early
2. Host-rate tank path was clean; accumulator 32k bridge was the real bug (diagnosed post-v1.0)

---

## Cross-Milestone Trends

### Process Evolution

| Milestone | Phases | Plans | Key Change |
|-----------|--------|-------|------------|
| v1.0 | 10 | 36 | Routing-first vertical slices |
| v2.0 | 8 | 29 | Surgical DSP fix on feature branch, RC1 safety preserved |

### Cumulative Quality

| Milestone | Tests | pluginval | Notes |
|-----------|-------|-----------|-------|
| v1.0 | 103 | 10 | TEST-07 multi-DAW deferred |
| v2.0 | 192 | 10 | 2 human_needed verification gaps deferred |

### Top Lessons (Verified Across Milestones)

1. Prove routing topology before polishing individual DSP blocks
2. Diagnose root cause (accumulator SRC) before applying downstream band-aids (anti-image SVF)
3. Keep human verification items explicit in STATE.md deferred table at milestone close
