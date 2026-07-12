# Phase 7: Pressure Send & MIDI - Research

**Researched:** 2026-07-06
**Domain:** Momentary wet-feed send gate + MIDI CC1 mod-wheel control (JUCE 8 / C++20 header-only DSP)
**Confidence:** HIGH

## Summary

Phase 7 replaces `StubPressureSend` with **`PressureSend`**: a named wet-feed multiplier before `SchroederTank32`. SEND-01/02 curve semantics already live in `ParameterSnapshot::capture` and `ParameterCurves::sendGain` (smoothstep + Firm 1.85 / Soft 1.2 exponents). `SmoothedParameterBank` smooths the computed `sendGain` at 25 ms — PressureSend applies the smoothed scalar per sample without touching tank state.

SEND-03 requires that releasing send to zero only gates new wet input; `SchroederTank32` delay lines must never be cleared. Existing `GatedBloomChainTest` "send release preserves tank energy" proves non-zero tail but only ~5 ms post-release — extend to ≥500 ms (24 000 samples @ 48 kHz).

SEND-04 wires MIDI CC1 in `PluginProcessor::processBlock` (currently `juce::ignoreUnused(midiMessages)`). When `send_connected` is on, CC1 value 0–127 maps to `send_amount` 0–1 via `setValueNotifyingHost`. Enable `NEEDS_MIDI_INPUT TRUE` on the plugin target.

**Primary recommendation:** Header-only `PressureSend.h`; swap in `GatedBloomChain`; extend tank-trail test; add `PressureSendTest.cpp` + `MidiSendAmountTest.cpp`; enable MIDI input in CMake; human DAW momentary-send smoke in VERIFICATION.md.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### PressureSend
- send_connected off → gain 1.0 (always-on wet feed)
- send_connected on → send_amount scales wet feed via smoothstep + Firm/Soft exponents
- Release to zero: multiply wet feed only; never reset SchroederTank32 buffers

#### MIDI
- CC1 (mod wheel) controls send_amount when send_connected on
- send_amount automatable; pad UI deferred to Phase 9

### Claude's Discretion
Exact smoothstep implementation, MIDI routing in processBlock, trail unit test fixture at planner discretion.

### Deferred Ideas (OUT OF SCOPE)
- PressureSendPad UI (Phase 9)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SEND-01 | PressureSend returns 1.0 when send_connected false | `ParameterSnapshot` forces 1.0; `PressureSend::computeGain` unit test |
| SEND-02 | Connected: send_amount via smoothstep + Firm/Soft curves | `ParameterCurves::sendGain` already implemented; PressureSend applies gain |
| SEND-03 | Release to zero does not clear tank; natural decay ≥500 ms | Chain multiplies pre-reverb only; extend tail test to 24k samples |
| SEND-04 | MIDI CC1 controls send_amount when connected | processBlock MIDI scan + NEEDS_MIDI_INPUT |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Owner | Rationale |
|------------|-------|-----------|
| Send curve math | `ParameterCurves::sendGain` | SEND-02 locked Phase 2 |
| Connected bypass | `ParameterSnapshot::capture` | SEND-01 at read time |
| Gain smoothing | `SmoothedParameterBank` | 25 ms ramp on sendGain |
| Wet multiply | `PressureSend.h` | Momentary gate before reverb |
| Chain slot swap | `GatedBloomChain.h` | Preserve bloom-then-chop order |
| Tank persistence | `SchroederTank32` (no reset on send) | SEND-03 architectural |
| MIDI CC1 | `PluginProcessor.cpp` | SEND-04 host control |
| Trail test | `GatedBloomChainTest.cpp` | SEND-03 offline gate |
| MIDI test | `MidiSendAmountTest.cpp` | SEND-04 offline gate |

## Signal Flow (unchanged topology)

```
monoIn → [preGate?] → PressureSend → SchroederTank32 → WetOverdrive → [postGate?] → wet
dryBuffer (raw input) ──────────────────────────────────────────────→ ParallelWetMixer → out
```

`StubPressureSend::process` → `PressureSend::process` at line 41 of `GatedBloomChain.h`.

## Standard Stack

| Library | Version | Purpose |
|---------|---------|---------|
| `ParameterCurves` | Phase 2 | smoothstep + Firm/Soft sendGain |
| JUCE MidiBuffer | 8.x | CC1 controller messages |
| Catch2 | 3.8.1 | Unit + trail + MIDI tests |

## Architecture Patterns

### Pattern 1: PressureSend gain (SEND-01/02)

```cpp
static float computeGain (float amountNorm, bool sendConnected, bool firmFeel) noexcept
{
    if (! sendConnected)
        return 1.0f;
    return ParameterCurves::sendGain (amountNorm, firmFeel);
}

static float process (float input, float sendGain) noexcept
{
    return input * sendGain;  // sendGain pre-smoothed upstream
}
```

`computeGain` exposed for unit tests; chain uses smoothed `sendGain` from bank.

### Pattern 2: Tank trail preservation (SEND-03)

Send release sets `sendGain → 0` which zeros **new** input to tank. Tank delay lines retain energy; tail decays per RT60. Anti-pattern: calling `SchroederTank32::reset()` or clearing comb/APF buffers on send release.

### Pattern 3: MIDI CC1 routing (SEND-04)

```cpp
for (const auto metadata : midiMessages)
{
    const auto msg = metadata.getMessage();
    if (msg.isController() && msg.getControllerNumber() == 1)
    {
        if (apvts.getRawParameterValue(sendConnected)->load() > 0.5f)
            if (auto* p = apvts.getParameter(sendAmount))
                p->setValueNotifyingHost (msg.getControllerValue() / 127.0f);
    }
}
```

Process MIDI before `ParameterSnapshot::capture` each block.

### Pattern 4: Extended trail test fixture

1. Warm up chain with sendGain=1.0, input burst
2. Release sendGain=0.0, silence input
3. At sample offset 24 000 (500 ms), tail RMS must exceed noise floor

### Anti-Patterns

- **Clearing tank on send release** — destroys momentary pedal feel
- **Applying send curve inside chain without smoothing** — zipper noise
- **MIDI CC1 when send_connected off** — violates SEND-04 guard
- **Ignoring midiMessages** — current PluginProcessor state

## Existing Code References

| File | Relevance |
|------|-----------|
| `StubPressureSend.h` | Trivial multiply stub to replace |
| `ParameterSnapshot.h:64-66` | SEND-01 connected bypass |
| `ParameterCurves.h:48-52` | SEND-02 smoothstep + exponents |
| `GatedBloomChain.h:41` | Integration point |
| `PluginProcessor.cpp:138` | MIDI currently ignored |
| `GatedBloomChainTest.cpp:119-136` | Trail test (needs 500 ms extension) |

## Open Questions (resolved)

| Question | Resolution |
|----------|------------|
| Where does curve math live? | ParameterCurves + Snapshot; PressureSend applies gain |
| Does plugin accept MIDI? | Add NEEDS_MIDI_INPUT TRUE |
| Trail test duration? | ≥500 ms = 24 000 samples @ 48 kHz |

## Planner Guidance

**MVP plan split (3 plans):**
1. `07-01`: PressureSend.h + unit tests (SEND-01, SEND-02)
2. `07-02`: Chain swap + 500 ms trail test (SEND-03)
3. `07-03`: MIDI CC1 + verification gate (SEND-04)

**Test tags:** `[send][PressureSend]`, `[send][tank]`, `[midi][send]`
