# Phase 5: SchroederTank32 Reverb - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Replace placeholder feedback-delay reverb in GatedBloomChain with authentic FV-style SchroederTank32 at 32.768 kHz. Size→RT60 mapping, Dark/Bright modes, authentic_color quantization. Preserve Phase 3/4 routing (bloom-then-chop intact).

</domain>

<decisions>
## Implementation Decisions

### Engine
- SchroederTank32 primary: 4 series APF → 4 parallel damped combs → modulated tank AP
- Fixed delay lengths at 32,768 Hz per architecture/planning corpus
- RT60 mapping: `0.25 + 5.75×size^2.4` with smooth feedback updates
- Dark: 55 ms predelay, 3200 Hz damping; Bright: no predelay, 8000 Hz
- authentic_color: internal 32 kHz processing, optional 9-bit param quantization

### Integration
- Swap PlaceholderReverbStub only; gates/IO/send/dirt unchanged
- RT60 impulse tests at size 0.25, 0.5, 1.0 within ±15%

### Claude's Discretion
Class decomposition, delay line implementation, oversampling/downsample strategy at planner discretion. Study chowdsp SimpleReverb in repo-samples.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- GatedBloomChain with PlaceholderReverbStub to replace
- ParameterCurves size→RT60 already in Phase 2
- InputStage, gates, ParallelWetMixer from Phases 3–4

### Integration Points
- Reverb interface behind same slot as placeholder
- Size, dark, authentic_color params from APVTS snapshot

</code_context>

<specifics>
## Specific Ideas

- ADR-002 superseded: SchroederTank32 primary (not Fdn8)
- `.planning/repo-samples/` chowdsp SimpleReverb for reference patterns

</specifics>

<deferred>
## Deferred Ideas

- Fdn8Reverb fallback (Phase 8)
- Extended stereo (post-v1)

</deferred>
