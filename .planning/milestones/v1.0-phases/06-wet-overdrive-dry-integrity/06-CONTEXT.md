# Phase 6: Wet Overdrive + Dry Integrity - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Replace PlaceholderDirtStub with real WetOverdrive asymmetric tanh circuit on wet reverb output only. distn blends clean wet ↔ fully driven via pow(distn, 2.8). Dry tap must remain pristine at distn=1, level=1.

</domain>

<decisions>
## Implementation Decisions

### WetOverdrive
- Asymmetric tanh grind on wet path only (after reverb, before post gate in chain order per architecture)
- distn=0: clean wet; distn=1: fully driven wet via ParameterCurves::distortionBlend (pow 2.8)
- Fixed drive circuit — no oversampling in Authentic mode (Extended OS deferred)

### Dry Integrity
- Dry tap taken before wet chain; never passes through WetOverdrive
- Unit test: dry-path THD unchanged at distn=1, level=1

### Claude's Discretion
Exact tanh asymmetry coefficients, blend implementation, THD test fixture at planner discretion.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- PlaceholderDirtStub in GatedBloomChain to replace
- ParameterCurves::distortionBlend from Phase 2
- ParallelWetMixer dry tap at unity

### Integration Points
- Swap stub in GatedBloomChain wet path
- distn from ParameterSnapshot

</code_context>

<specifics>
## Specific Ideas

- OD-03 dry-path integrity is the critical regression gate

</specifics>

<deferred>
## Deferred Ideas

- OD oversampling in Extended mode (post-v1)

</deferred>
