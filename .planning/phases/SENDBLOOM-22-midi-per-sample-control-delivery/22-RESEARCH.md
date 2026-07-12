# Phase 22: MIDI & Per-Sample Control Delivery - Research

**Researched:** 2026-07-12
**Domain:** JUCE MIDI CC1 realtime modulation (ADR-V1-03), sample-accurate span cuts (ADR-V1-05 + RT-04), per-sample dynamic control arrays (ADR-V1-06)
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

**CRITICAL:** Locked decisions are NON-NEGOTIABLE for planning/execution.

### Locked Decisions
- **D-01:** Remove `sendParam->store` / `setValueNotifyingHost` for CC1; MIDI → `PressureController` realtime target; `max` combine; flip `[midi-apvts]` green
- **D-02:** Sample-accurate CC1; span cuts at next CC1; value 0 releases MIDI pressure; non-CC1 ignored
- **D-03:** MIDI target persists across blocks (remove Phase 20 forced-zero stub)
- **D-04:** Per-sample distn/threshold/send arrays consumed by chain; preserve input/level/output/bypass per-sample
- **D-05:** Update `MidiSendAmountTest` to ADR-V1-03; expand V1Contract MIDI tests

### Deferred (OUT OF SCOPE)
- PostHard / Input / shipping → later phases
- Do not regress pressure-release, oversized-block, true-bypass
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| MIDI-01 | CC1 pressure only when connected | Ignore CC1 when `send_connected` false; disconnected → unity send |
| MIDI-02 | No `setValueNotifyingHost` in processBlock | Source scan + never call from MIDI path |
| MIDI-03 | No APVTS `send_amount` write | Delete `sendParam->store` |
| MIDI-04 | Sample positions respected | Apply CC1 at span start = event samplePosition |
| MIDI-05 | Multiple events ordered | Iterate MidiBuffer in order; last at same sample wins |
| MIDI-06 | CC1 0 releases MIDI pressure | `setMidiPressureTarget(0)` |
| MIDI-07 | Host/UI + MIDI via `max` | Already in PressureController |
| MIDI-08 | MIDI does not dirty saved state | APVTS unchanged → getStateInformation clean |
| MIDI-09 | Non-CC1 ignored | Controller number == 1 only |
| MIDI-10 | Finite/deterministic @ 1–2048 | No alloc; same MIDI→gain path across block sizes |
| RT-04 | Span respects CC1 boundaries | `span = min(..., distanceToNextCc1)` |
| RT-06 | Send/distn/threshold per sample | Scratch arrays + chain array overloads |
| RT-07 | Input/level/output/bypass per sample | Already in processSpan — keep; add contract proof |
</phase_requirements>

## Current State (Exact Bug Loci)

### 1. APVTS mutation (MIDI-02/03) — `PluginProcessor.cpp` ~184–199
```cpp
for (const auto metadata : midiMessages) {
  if (CC1 && connected)
    sendParam->store(norm);  // FORBIDDEN
}
```
Also Phase 20 stub at ~216:
```cpp
pressureController.setMidiPressureTarget(0.0f);  // kills MIDI every block
```

### 2. No sample accuracy (MIDI-04/05, RT-04)
All MIDI applied before audio; span loop ignores event positions (`min(remaining, preparedMaxBlock_, kControlQuantum)` only).

### 3. Distn/threshold span-constant (RT-06) — `processSpan` + `GatedBloomChain`
`getNextDistnBlend()` / threshold advanced per sample but only sample 0 latched into `spanDistn` / `spanThresholdDb`. Chain overloads take scalar distn/threshold. Send gains already array-backed.

### 4. Stale tests
- `V1ContractMidiApvtsPurityTest` — correctly red (expects no store)
- `MidiSendAmountTest` — expects APVTS mutation; must update to ADR-V1-03

## Recommended Implementation Shape

### MIDI cursor
1. Persist `midiTarget_` across blocks.
2. At each span start `offset`, apply all CC1 events with `samplePosition == offset` (connected only).
3. `nextCc1 =` smallest sample position `> offset` among CC1 events (or INT_MAX).
4. `span = jmin(remaining, preparedMaxBlock_, kControlQuantum, nextCc1 - offset)`.

### Chain API
Add overload accepting `const float* distnBlends` and `const float* thresholdDbs` (nullable → constant fallback for existing tests). Keep sendGains array path.

### Scratch
Add `distnScratch_`, `thresholdDbScratch_` sized in `prepareToPlay` like existing scratches.

## Open Questions — RESOLVED

| # | Question | Resolution |
|---|----------|------------|
| Q1 | Clear midiTarget on disconnect? | No — persist; disconnected path returns unity regardless (MIDI-01) |
| Q2 | Apply CC1 when disconnected into midiTarget? | No — ignore CC1 when disconnected (MIDI-01) |
| Q3 | Prove pressure without APVTS? | Wet-energy / buffer RMS contracts + ParameterSnapshot unchanged |
| Q4 | Keep scalar chain overloads? | Yes — for unit tests; production uses arrays |
| Q5 | MidiSendAmountTest fate? | Update to ADR-V1-03 (allowed) |

## Risks

| Risk | Mitigation |
|------|------------|
| Regress pressure-release | Don't touch pad/UI release path; re-run `[pressure-release]` |
| Span of 0 if event at same offset mishandled | Apply events at offset first; distance uses `>` offset so span ≥ 1 |
| Chain API break | Keep scalar overloads; extend private path |
| Zipper from constant distn | Filling arrays every sample removes block-size zipper for Distn |

## Sources

- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` ADR-V1-03/05/06 + §11 Phase 22
- `source/PluginProcessor.cpp`, `PressureController.h`, `GatedBloomChain.h`
- Phase 21 research: MIDI distance deferred to Phase 22

**Confidence:** HIGH — defects and APIs are local and already partially prepared (PressureController + sendGain arrays).
