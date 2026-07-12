# Phase 16: Engine Crossfade - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — smart discuss skipped)

<domain>
## Phase Boundary

Implement safe crossfade when toggling `authentic_color` between host-rate and ProperSRC engines during performance. 20–50 ms fade without clicks, NaN, or buffer overruns. 1000-toggle stress test.

**In scope:** XFADE-01 crossfade implementation in GatedBloomChain/SchroederTank32, XFADE-02 stress test, mid-block toggle handling deferred from Phase 14.

**Out of scope:** PDC/latency (Phase 17), user enablement (Phase 18), new UI controls.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation at Claude's discretion per XFADE-01/02. Reuse existing block-level integration from Phase 14. Crossfade duration 20–50 ms per ROADMAP.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `source/GatedBloomChain.h` — block + per-sample split from Phase 14
- `source/SchroederTank32.h` — dual engine routing (host vs ProperSRC)
- `source/PluginProcessor.cpp` — block-start authenticColor sampling (Phase 14 deviation)
- `tests/RealtimeStressTest.cpp` — stress patterns

</code_context>

<specifics>
## Specific Ideas

No specific requirements — crossfade per ROADMAP Phase 16.

</specifics>

<deferred>
## Deferred Ideas

- PDC policy — Phase 17
- User-facing enablement — Phase 18

</deferred>
