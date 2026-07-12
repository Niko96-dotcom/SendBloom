# Phase 17: Latency/PDC + ADR-003 - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase)

<domain>
## Phase Boundary

Measure SRC round-trip latency at 44.1/48/88.2/96 kHz. Write ADR-003 documenting PDC policy. Implement reported latency per policy when ProperSRC active; zero latency when 32k off (RC1).

**In scope:** LAT-01, LAT-02, LAT-03, DOC-01, docs/architecture/ADR-003-proper-32k-src.md

**Out of scope:** User enablement (Phase 18), changing default authentic_color on

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
Measure actual latency via r8brain API (not estimates). Document policy choice with rationale in ADR-003.

</decisions>

<code_context>
## Existing Code Insights

- `source/RateConverterPair.h` — `getRoundTripLatencySamples()` at 48 kHz = 5160
- `source/FixedRateAdapter.h` — ProperSRC path
- v1 zero-PDC promise when 32k off

</code_context>
