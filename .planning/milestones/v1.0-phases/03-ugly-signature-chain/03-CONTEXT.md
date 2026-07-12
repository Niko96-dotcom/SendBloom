# Phase 3: Ugly Signature Chain - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous smart discuss — routing-first product proof)

<domain>
## Phase Boundary

Crude full parallel chain proves signature product behavior — routing is audible and feels like the intended pedal. Dry tap stays clean; wet path uses placeholder reverb + dumb feedback delay + fixed tanh dirt; post gate (stub) chops wet from input detector; pressure-send stub preserves tank trails. Tone irrelevant — routing is the product.

</domain>

<decisions>
## Implementation Decisions

### Routing Topology (locked by roadmap)
- Parallel: clean dry tap + gated wet path (pre/post gate stubs → pressure send stub → placeholder reverb → tanh dirt → mix)
- Dry guitar clean at unity; wet-only dirt when distn raised
- Post gate keyed from input detector, not wet tail
- Pressure-send release to zero does not clear reverb tank

### Placeholder DSP
- Placeholder reverb: simple feedback delay or crude tank — not SchroederTank32 (Phase 5)
- Placeholder dirt: fixed tanh on wet path only
- Gate stubs: PreSoft/PostHard behavior approximated crudely for audible chop

### Claude's Discretion
Exact stub algorithms, buffer sizes, ParallelWetMixer implementation, and unit test fixtures at planner discretion within architecture routing pseudocode from planning corpus.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 2: APVTS, ParameterSnapshot, SmoothedParameterBank, BypassCrossfade, DummyDspHooks
- Phase 1: passthrough processor shell, Catch2 harness

### Established Patterns
- `namespace sendbloom`, block-rate snapshot, no heap in processBlock
- Parameter curves for level (sin), distn (pow 2.8), send

### Integration Points
- Replace DummyDspHooks with ugly-chain routing in processBlock
- Wire level/distn/size/send params into wet path stubs

</code_context>

<specifics>
## Specific Ideas

- Study `.planning/repo-samples/` for gate/routing patterns (BYOD, PinkGuitarFX)
- Routing proof unit tests: dry clean, wet-only dirt, input-keyed chop
- Must feel like gated-dirty-ambience object in DAW even with ugly DSP

</specifics>

<deferred>
## Deferred Ideas

- Real InputStage/OutputStage (Phase 4)
- SchroederTank32 reverb (Phase 5)
- Real WetOverdrive (Phase 6)
- Real PressureSend (Phase 7)

</deferred>
