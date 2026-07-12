# Phase 22: MIDI & Per-Sample Control Delivery - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-accepted smart-discuss recommendations (full autonomous run)

<domain>
## Phase Boundary

MIDI CC1 is sample-position-aware realtime pressure modulation (NOT APVTS mutation), and fast dynamic controls are consumed per sample. Covers MIDI-01…10, RT-04, RT-06, RT-07. ADRs: ADR-V1-03, ADR-V1-06.

Does NOT fix PostHard de-click, Input anchors, or shipping-policy (Phases 23/25). Does NOT regress pressure-release, oversized-block, or true-bypass greens from Phases 20–21.

</domain>

<decisions>
## Implementation Decisions (LOCKED — auto-accepted)

### D-01 — MIDI purity (ADR-V1-03)
- Remove `sendParam->store` / any `setValueNotifyingHost` for CC1 from `processBlock` when pressure mode is connected
- MIDI pressure is a realtime-only target on `PressureController` (`setMidiPressureTarget`)
- Host/UI pressure and MIDI combine via `max` (already in `PressureController::processSample`)
- Flip `[midi-apvts]` green; preserved Phase 20/21 greens must stay green

### D-02 — Sample-accurate CC1 (MIDI-04…06, RT-04)
- CC1 event sample positions are respected; multiple events in one block apply in order (last at same sample wins)
- Span length = `min(remaining, preparedMaxBlock_, kControlQuantum, distanceToNextCc1)`
- At each span start offset, apply every CC1 whose `samplePosition == offset` to the MIDI target only
- CC1 value 0 releases MIDI pressure (`midiTarget = 0`); does not write APVTS
- Non-CC1 messages do not change pressure
- When disconnected, CC1 does not affect sound (ignore / no audible effect)

### D-03 — MIDI target persistence
- Do **not** force `setMidiPressureTarget(0.0f)` at every block start (Phase 20 stub removed)
- MIDI target persists across blocks until a new CC1 event updates it

### D-04 — Per-sample dynamic controls (ADR-V1-06 / RT-06 / RT-07)
- Fill preallocated scratch arrays every sample: send gain, distn blend, threshold dB (plus existing wet/output/bypass/input)
- `GatedBloomChain::processBlock` consumes per-sample distn and threshold arrays (not span constants alone)
- Input gain, level, output gain, and bypass remain per-sample (already in processSpan mix path — preserve)

### D-05 — Contract updates
- Prefer new/green V1Contract MIDI tests; expand beyond purity source-scan
- `MidiSendAmountTest` may still document old APVTS behavior — update it to match ADR-V1-03 (allowed contract update for this phase)

### Claude's Discretion
- Exact MIDI event cursor / scratch indexing as long as sample positions and no-alloc hold
- How to prove audible pressure without APVTS (wet energy / buffer energy preferred over exposing internals)

</decisions>

<code_context>
## Existing Code Insights

### From Phase 20–21
- `PressureController` already implements `max(host, midi)` + asymmetric 3 ms / 25 ms + Firm/Soft curve
- Phase 20 forced `setMidiPressureTarget(0.0f)` every block — must remove for Phase 22
- Span engine with `kControlQuantum = 128` exists; MIDI distance excluded until this phase
- `sendGainScratch_` already filled per sample; chain already accepts `sendGains` array
- `distnBlend` / `thresholdDb` still latched as span constants into chain — RT-06 gap
- Failing contract: `[v1][contract][midi-apvts]` (expects no `sendParam->store`)

### Integration Points
- `source/PluginProcessor.cpp` processBlock MIDI loop + span loop + processSpan
- `source/PressureController.h` midi target API
- `source/GatedBloomChain.h` processBlock overloads
- `tests/V1ContractMidiApvtsPurityTest.cpp`, `tests/MidiSendAmountTest.cpp`

</code_context>

<specifics>
## Specific Ideas

Autonomous run — accept recommended defaults; pause only on blockers.
Preserve `[pressure-release]`, `[oversized-block]`, `[true-bypass]`.

</specifics>

<deferred>
## Deferred Ideas

- PostHard 0.75 ms de-click, Input −9/0/+9 → Phase 23
- Predelay/mod/SRC/dirt → Phase 24
- Shipping brand strings → Phase 25

</deferred>
