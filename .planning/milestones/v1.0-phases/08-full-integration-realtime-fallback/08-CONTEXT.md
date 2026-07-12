# Phase 8: Full Integration / Realtime / Fallback - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Production chain hardening: zero reported latency, realtime safety stress verified, Fdn8Reverb fallback behind same reverb interface. pluginval strictness raised to 7. Full gated-dirty-ambience stable under extended load.

</domain>

<decisions>
## Implementation Decisions

### Latency & Realtime
- Zero reported plugin latency; no lookahead in processBlock
- Realtime safety: 10,000 blocks varying sizes; no heap alloc, locks, logging, file I/O in audio thread after prepare()
- Preallocate all buffers in prepareToPlay

### Fdn8Reverb Fallback
- Same IReverbEngine (or equivalent) interface as SchroederTank32
- Available for tests and Extended reference; Authentic mode keeps SchroederTank32 primary
- ADR-002: Fdn8 is fallback only

### CI
- Raise pluginval to strictness 7 for this phase gate

### Claude's Discretion
Fdn8 implementation depth (minimal vs full), reverb interface abstraction shape, stress test harness at planner discretion.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Complete GatedBloomChain with all real modules (Phases 3–7)
- SchroederTank32 embedded directly in chain today — needs interface extraction for fallback

### Integration Points
- PluginProcessor::getLatencySamples() must return 0
- CI workflow STRICTNESS_LEVEL bump to 7

</code_context>

<specifics>
## Specific Ideas

- Integration regression: all Phase 3–7 routing tests must still pass
- Study chowdsp FDN patterns in repo-samples if useful for Fdn8

</specifics>

<deferred>
## Deferred Ideas

- Extended stereo decorrelator (post-v1)

</deferred>
