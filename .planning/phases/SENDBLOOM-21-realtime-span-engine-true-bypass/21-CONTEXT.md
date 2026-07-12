# Phase 21: Realtime Span Engine & True Bypass - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-accepted smart-discuss recommendations (full autonomous run)

<domain>
## Phase Boundary

Realtime span processing and true bypass: host blocks of any size keep full wet processing with zero audio-thread heap allocation; settled bypass is channel-preserving unity. Covers RT-01…03, RT-05, RT-08…15, CORE-14…18. Does NOT implement MIDI APVTS purity / per-sample MIDI delivery (Phase 22) or Pressure Mode semantics already fixed in Phase 20.

</domain>

<decisions>
## Implementation Decisions

### Span Engine
- Remove dry-only oversized-block fallback and any `dryBuffer.setSize` (or equivalent heap) inside `processBlock`
- Process arbitrary host block sizes with no audio-thread allocation; prepare max block covers host size
- 2048-sample host block prepared at 512 must match smaller-block renders within tolerance; wet remains nonzero for oversized blocks
- Control-rate reverb values update in spans of at most 128 samples (ADR quantum)
- `preparedMaxBlock_ <= 0` handled safely

### True Bypass (ADR-V1-10 / CORE-14…18)
- Settled bypass: each input channel at unity within float tolerance; ignore Input/Distn/Gate/Level/Output when bypassed
- Transitions remain click-bounded; engaged mono-first behavior unchanged
- Flip Phase 19 `[true-bypass]` and `[oversized-block]` contracts green via production fixes

### Engine Crossfade / Authentic Mode
- Authentic-mode changes request exactly one engine target transition per parameter change
- Reported latency stays zero under ADR-003; crossfade begins in first block after change; converges after rapid toggles
- Reset only the idle engine; zero allocations through crossfade completion
- 10,000-block stress stays finite

### Claude's Discretion
- Exact span buffer/scratch storage strategy (preallocated members vs stack) as long as no processBlock heap
- How to structure tests proving 2048 vs chunked equivalence

</decisions>

<code_context>
## Existing Code Insights

### From Phase 19–20
- Failing contracts: `[oversized-block]`, `[true-bypass]` (must turn green)
- PressureController wired; do not regress Phase 20 pressure-release greens
- `IntegrationAllocScanTest`, `RealtimeStressTest`, `BypassCrossfadeTest` exist

### Integration Points
- `source/PluginProcessor.cpp` processBlock / prepareToPlay
- Bypass crossfade helpers; engine crossfade / authentic_color path

</code_context>

<specifics>
## Specific Ideas

Autonomous run — accept recommended defaults; pause only on blockers.

</specifics>

<deferred>
## Deferred Ideas

- MIDI CC1 sample-accurate pressure without APVTS mutation → Phase 22
- PostHard de-click, Input anchors → Phase 23
- Predelay/mod/SRC/dirt → Phase 24

</deferred>
