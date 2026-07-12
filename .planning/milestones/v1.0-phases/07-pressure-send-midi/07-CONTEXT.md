# Phase 7: Pressure Send & MIDI - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Replace PressureSendStub with real PressureSend: momentary wet feed control, smoothstep + Firm/Soft exponent curves, send_connected toggle. Tank trails persist when send releases to zero. MIDI CC1 maps to send_amount when connected.

</domain>

<decisions>
## Implementation Decisions

### PressureSend
- send_connected off → return 1.0 (always-on wet feed)
- send_connected on → send_amount scales wet feed via smoothstep + Firm (1.85) / Soft (1.2) exponents per ParameterCurves
- Release to zero: multiply wet feed only; never reset SchroederTank32 buffers

### MIDI
- CC1 (mod wheel) controls send_amount when send_connected on
- Expose send_amount as automatable param; pad UI deferred to Phase 9

### Claude's Discretion
Exact smoothstep implementation, MIDI routing in processBlock, trail unit test fixture at planner discretion.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- PressureSendStub in GatedBloomChain
- ParameterCurves send curves from Phase 2
- SchroederTank32 must not be cleared on send release

### Integration Points
- Swap stub before reverb in wet path
- MIDI buffer in processBlock (already ignored in passthrough era — now wire CC1)

</code_context>

<specifics>
## Specific Ideas

- SEND-03 tank trail test: ≥500 ms decay after send→0
- Momentary feel is product-critical

</specifics>

<deferred>
## Deferred Ideas

- PressureSendPad UI (Phase 9)

</deferred>
