# Phase 21: Realtime Span Engine & True Bypass - Research

**Researched:** 2026-07-12
**Domain:** JUCE realtime `processBlock` span architecture, no-alloc oversized host blocks, ADR-V1-10 true bypass, ADR-V1-07 authentic engine crossfade
**Confidence:** HIGH

<user_constraints>
## User Constraints (from CONTEXT.md)

**CRITICAL:** Locked decisions are NON-NEGOTIABLE for planning/execution.

### Locked Decisions

#### Span Engine (D-01…D-05)
- **D-01:** Remove dry-only oversized-block fallback and any `dryBuffer.setSize` (or equivalent heap) inside `processBlock`
- **D-02:** Process arbitrary host block sizes with no audio-thread allocation; prepare-time capacity covers spans ≤ `preparedMaxBlock_`
- **D-03:** 2048-sample host block prepared at 512 must match smaller-block renders within tolerance; wet remains nonzero for oversized blocks
- **D-04:** Control-rate reverb values update in spans of at most 128 samples (`kControlQuantum`, ADR-V1-05)
- **D-05:** `preparedMaxBlock_ <= 0` handled safely (identity / early return, no UB)

#### True Bypass (D-06…D-08) — ADR-V1-10 / CORE-14…18
- **D-06:** Settled bypass: each input channel at unity within float tolerance; ignore Input/Distn/Gate/Level/Output when bypassed
- **D-07:** Transitions remain click-bounded; engaged mono-first behavior unchanged (`extended_stereo` path preserved)
- **D-08:** Flip Phase 19 `[true-bypass]` and `[oversized-block]` contracts green via production fixes

#### Engine Crossfade / Authentic Mode (D-09…D-10)
- **D-09:** Authentic-mode changes request exactly one engine target transition per parameter change (ADR-V1-07 snapshot edge)
- **D-10:** Reported latency stays zero under ADR-003; crossfade begins in first block after change; converges after rapid toggles; reset only the idle engine; zero allocations through crossfade completion; 10,000-block stress stays finite

### Claude's Discretion
- Exact span buffer/scratch storage strategy (preallocated members vs stack) as long as no `processBlock` heap
- How to structure tests proving 2048 vs chunked equivalence (existing `V1ContractOversizedBlockTest` is sufficient primary proof)

### Deferred Ideas (OUT OF SCOPE)
- MIDI CC1 sample-accurate pressure without APVTS mutation → Phase 22 (RT-04, MIDI-*)
- Per-sample dynamic distn/threshold array reshape (ADR-V1-06 full signature / RT-06/07) → Phase 22
- PostHard de-click, Input anchors → Phase 23
- Predelay/mod/SRC/dirt → Phase 24
- Shipping brand strings / `reverbx` filenames → Phase 25 (do not edit shipping-policy product asserts)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| RT-01 | No alloc tokens in `processBlock` | Remove `dryBuffer.setSize`; extend `IntegrationAllocScanTest` to ban `setSize`; leave MIDI `sendParam->store` for Phase 22 |
| RT-02 | Oversized blocks keep full wet | Delete dry-fallback branch; span-process full wet path |
| RT-03 | 2048@prepare512 matches 4×512 | `V1ContractOversizedBlockTest` flip green |
| RT-05 | Control-rate reverb ≤128 | `kControlQuantum=128` span loop; re-sample RT60/dark/authentic per span |
| RT-08 | One authentic transition per change | ADR-V1-07 `requestedAuthenticColor_` vs snapshot |
| RT-09 | Latency stays 0 | Keep `updateReportedLatency` → `setLatencySamples(0)` |
| RT-10 | Crossfade starts first block | Request at block start on edge |
| RT-11 | Rapid toggles converge | Existing `requestEngineCrossfade` retarget |
| RT-12 | Reset only idle engine | Already in `SchroederTank32` completion path |
| RT-13 | Crossfade completion zero heap | Keep no-alloc mix; static scan |
| RT-14 | 10k-block stress finite | Existing `RealtimeStressTest` + strengthen toggles/bypass/oversized |
| RT-15 | `preparedMaxBlock_ <= 0` safe | Early return before scratch indexing |
| CORE-14 | Channel-preserving bypass | Per-channel dry × (1−mix) + processed × mix |
| CORE-15 | Unity within tolerance | Settled mix=0 → output == input `<1e-6` |
| CORE-16 | Ignore Input/Distn/Gate/Level/Output | Output gain only on engaged path before crossfade |
| CORE-17 | Click-bounded transitions | Keep 5 ms `bypassWetMix` smoother; existing BypassCrossfade tests |
| CORE-18 | Engaged mono-first unchanged | Do not change non-extended engaged dual-mono; only bypass dry side is per-channel |
</phase_requirements>

<architectural_responsibility_map>
## Architectural Responsibility Map

Single-tier native audio plugin — all capabilities reside in the audio processor / DSP tier (AU/VST3 `PluginProcessor` + chain). Editor UI is out of scope for this phase.

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Span loop / no-alloc / oversized wet | Audio processor | — | Host calls `processBlock` with arbitrary sizes |
| True bypass crossfade | Audio processor | — | ADR-V1-10 mix after engaged path |
| Authentic engine request | Audio processor → `GatedBloomChain` / `SchroederTank32` | — | One `requestEngineCrossfade` per APVTS edge |
| Engine crossfade DSP | `EngineCrossfade` / `SchroederTank32` | — | Existing 35 ms equal-power fade |
| Contract proofs | Catch2 tests | — | Flip `[oversized-block]` / `[true-bypass]`; add RT realtime contracts |
</architectural_responsibility_map>

<research_summary>
## Summary

Phase 21 fixes the two remaining realtime honesty defects frozen red in Phase 19: (1) **oversized host blocks** take a dry-only fallback that zeros wet and optionally heap-resizes `dryBuffer` in `processBlock`; (2) **settled bypass** still mono-sums and applies `OutputStage` after the bypass mix, so channels collapse and Output gain leaks through. Milestone ADR-V1-05 already specifies the remedy: a no-allocation **span loop** with `kControlQuantum = 128`, span length `min(remaining, preparedMaxBlock_, 128)` (MIDI distance deferred to Phase 22). ADR-V1-10 requires computing the fully engaged (output-gained) signal first, then crossfading against the **original per-channel dry** copy. ADR-V1-07 replaces the 15 ms authentic smoother 0.5-threshold with a once-per-block snapshot edge request.

Existing engines already support idle-engine reset and zero-heap crossfade mixing (`SchroederTank32`, `EngineCrossfade`). `GatedBloomChain` / `SchroederTank32` early-out when `numSamples > maxBlockSize_` — the span loop must never hand them spans larger than `preparedMaxBlock_`.

**Primary recommendation:** Refactor `PluginProcessor::processBlock` into prepare-sized scratch + `processSpan` (ADR §10.4–10.6), delete the oversized dry branch and in-process `setSize`, fix bypass mix order for true unity, then switch authentic to snapshot-edge requests — flipping `[oversized-block]` and `[true-bypass]` green without touching MIDI purity, PostHard, Input, or shipping brand asserts.
</research_summary>

## Exact Bug Loci (verified)

### 1. Oversized dry fallback + process-time resize — RT-01/02/03 RED

```221:289:source/PluginProcessor.cpp
    if (dryBuffer.getNumChannels() < numOutputChannels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (numOutputChannels, numSamples, false, false, true);
    ...
    if (numSamples > preparedMaxBlock_)
    {
        ...
            constexpr auto wet = 0.0f;
        ...
        return;
    }
```

Also: `monoScratch_` / `wetScratch_` / etc. are sized to `samplesPerBlock` in `prepareToPlay` only — oversized path cannot reuse them without either growing capacity at prepare or spanning ≤ `preparedMaxBlock_`.

Secondary silent fail: `GatedBloomChain` / `SchroederTank32` return early if `numSamples > maxBlockSize_` — one-shot 2048 into chain would no-op wet even without the dry fallback.

`tests/V1ContractOversizedBlockTest.cpp` `[v1][contract][oversized-block][RT-02]` asserts wet energy > 1e-4 and max abs diff vs 4×512 `< 2e-5`.

`tests/V1ContractShippingPolicyTest.cpp` also bans `dryBuffer.setSize` in process body (paired with `sendParam->store` — leave store alone; shipping case stays red on MIDI half).

### 2. True bypass applies Output after mix + mono collapse — CORE-14…16 RED

```352:385:source/PluginProcessor.cpp
        // dryMix/bypassWet crossfade, then:
        buffer[...] = OutputStage::processSample (preOutput, outputGain);
        // non-extended: monoSum dry → same out all channels
```

ADR-V1-10 required order:

```text
engaged[ch] = OutputStage(processedEngagePath, outputGain)   // or mono-first engaged
final[ch]   = originalDry[ch] * (1 - engagedMix) + engaged[ch] * engagedMix
```

When `engagedMix → 0`, `final == originalDry` per channel regardless of Output/Input/Distn/Gate/Level.

`tests/V1ContractTrueBypassTest.cpp` `[v1][contract][true-bypass][CORE-14]` prepares bypass=1, outputGain=+6 dB, settles >5 ms, asserts L/R match distinct inputs `<1e-6`.

### 3. Authentic request via smoothed 0.5 edge — RT-08/10 drift

```315:321:source/PluginProcessor.cpp
        if (! crossfadeEdgeHandled
            && ((prevAuthenticSmoothed <= 0.5f && authenticColorTarget > 0.5f)
                || (prevAuthenticSmoothed > 0.5f && authenticColorTarget <= 0.5f)))
        {
            chain.requestEngineCrossfade (authenticColorTarget > 0.5f);
            updateReportedLatency (authenticColorTarget > 0.5f);
            crossfadeEdgeHandled = true;
        }
```

Plus `SmoothedParameterBank` 15 ms `authenticColorTarget` smoother. ADR-V1-07: compare `snapshot.authenticColor` to `requestedAuthenticColor_` once at block start.

### 4. Control-rate held for full host block — RT-05

```348:350:source/PluginProcessor.cpp
    chain.processBlock (..., blockStartRt60, blockStartDark, blockStartAuthentic, ...);
```

Only sample-0 control-rate values feed the chain for the entire host block. ADR-V1-05 requires re-latch ≤ every 128 samples.

### 5. Already good (preserve)

- `updateReportedLatency` always `setLatencySamples(0)` (RT-09 / ADR-003 Path B)
- `EngineCrossfade` equal-power mix; completion resets only idle engine (RT-12/13)
- `RealtimeStressTest` 10k finite (RT-14 base) — extend coverage for bypass + oversized toggles
- Phase 20 `PressureController` wiring / pressure-release green — do not regress

## Standard Stack

| Library / artifact | Version / path | Purpose |
|--------------------|----------------|---------|
| JUCE | 8.0.12 (repo pin) | `AudioProcessor`, `AudioBuffer`, `SmoothedValue` |
| Catch2 | 3.8.1 via CPM | Contract + stress tests |
| Existing DSP | in-tree | `GatedBloomChain`, `SchroederTank32`, `EngineCrossfade`, `BypassCrossfade`, `OutputStage`, `ParallelWetMixer` |

**No new packages.** Package Legitimacy Audit: N/A (no installs).

## Architecture Patterns

### Target processBlock (ADR §10.4, Phase-21 MIDI stub)

```text
prepareToPlay(sr, N):
  allocate dryBuffer + scratch vectors to N (== preparedMaxBlock_)
  chain.prepare(sr, N)
  requestedAuthenticColor_ = snapshot.authenticColor

processBlock(buffer, midi):
  // KEEP Phase-22 MIDI APVTS mutation as-is (do not fix purity)
  if preparedMaxBlock_ <= 0: clear extras; return   // RT-15

  snapshot = capture; setTargets; pressure wiring
  if snapshot.authenticColor != requestedAuthenticColor_:
      chain.requestEngineCrossfade(snapshot.authenticColor)
      updateReportedLatency(snapshot.authenticColor)
      requestedAuthenticColor_ = snapshot.authenticColor

  offset = 0
  while offset < numSamples:
      span = min(numSamples - offset, preparedMaxBlock_, kControlQuantum)
      // Phase 22: also min with next CC1 sample distance (RT-04)
      processSpan(buffer, offset, span, snapshot)
      offset += span
```

### processSpan contract (ADR §10.5)

1. Copy `buffer[offset:offset+span]` per channel into preallocated `dryBuffer` indices `[0:span]` (capacity ≥ `preparedMaxBlock_`; **never** `setSize` here).
2. Fill per-sample scratch (input/level/send/bypass/output) for `span` samples via existing smoothers / `PressureController`.
3. Latch span-constant RT60 / dark / authentic / gate placement from current smoother heads (or first sample of span).
4. `chain.processBlock` with `span` ≤ `preparedMaxBlock_`.
5. Build engaged output (mono-first dual-mono unless `extendedStereo`; unity dry + wet level; **then** Output gain).
6. Final: `final[ch] = dry[ch]*(1-engagedMix) + engaged[ch]*engagedMix` — write back at `offset`.

### Bypass helper

Prefer extending `BypassCrossfade` with `mixSample(originalDry, processed, engagedMix)` per ADR §10.6; keep 5 ms smoother in `SmoothedParameterBank::bypassWetMix`.

## Common Pitfalls

| Pitfall | Why it bites | Avoidance |
|---------|--------------|-----------|
| Growing scratch in process for 2048 | Violates RT-01; shipping scan | Span ≤ preparedMaxBlock_; never setSize/assign/resize in process |
| Passing full 2048 into chain | Chain early-returns → wet silent | Span loop only |
| Applying Output after bypass mix | CORE-16 fail | Gain on engaged only |
| Fixing mono-first engaged path while fixing bypass | Breaks CORE-18 | Only dry side of bypass is per-channel |
| Removing `sendParam->store` “while here” | Phase 22 / shipping MIDI assert ownership | Leave MIDI purity red |
| Using authentic smoother edge | Misses RT-08 one-request semantics | Snapshot bool edge |
| Chunking 2048 as four separate prepare-sized processBlocks with snap | Diverges smoothers vs continuous span | Continuous offset loop in one call |
| Weakening contract tolerances | Hides incomplete fix | Keep `<2e-5` / `<1e-6` |

## Don't Hand-Roll

- Engine equal-power fade — keep `EngineCrossfade`
- Bypass timing — keep bank 5 ms smoother
- Pressure semantics — keep Phase 20 `PressureController`
- Latency Path B — keep zero report

## Open Questions

### Q1: Scratch strategy for oversized hosts?
**Answer:** Preallocated member scratch / `dryBuffer` sized in `prepareToPlay` to `samplesPerBlock` (`preparedMaxBlock_`). Oversized hosts are handled by iterating spans of `min(remaining, preparedMaxBlock_, kControlQuantum)` and copying only `span` samples into scratch — no audio-thread heap. (Discretion locked.)

### Q2: Include MIDI cursor distance in span `min` now?
**Answer:** **No for Phase 21.** RT-04 is Phase 22. Span `min` uses remaining / `preparedMaxBlock_` / `kControlQuantum` only. Leave `sendParam->store` untouched.

### Q3: Implement full ADR-V1-06 per-sample distn/threshold arrays now?
**Answer:** **No.** RT-06/07 are Phase 22. Phase 21 keeps span-constant distn/threshold into existing `GatedBloomChain::processBlock` overloads; per-sample send/level/bypass/output remain as today.

### Q4: Does removing `dryBuffer.setSize` make `[shipping-policy]` green?
**Answer:** **No.** That case also requires absence of `sendParam->store`. Phase 21 removes setSize (RT-01 / oversized); shipping filter stays intentionally red until Phase 22+25. Do not edit shipping brand asserts.

### Q5: Should IntegrationAllocScan ban `setSize`?
**Answer:** **Yes.** Extend `requireNoAllocTokens` (or equivalent) to reject `.setSize(` inside scanned process bodies for RT-01; existing tokens stay.

### Q6: Where do RT-05 / RT-08…15 live if REQUIREMENTS map cites `V1ContractRealtime*.cpp`?
**Answer:** Add `tests/V1ContractRealtimeSpanTest.cpp` and/or `tests/V1ContractAuthenticTransitionTest.cpp` tagged `[v1][contract][realtime]` plus requirement IDs. Reuse `EngineCrossfadeTest` / `RealtimeStressTest` / `LatencyTest` where already green; strengthen only as needed.

### Q7: SEND-14 Phase 20 oversized caveat?
**Answer:** After Phase 21 oversized green, pressure semantics hold for oversized wet path as a side effect; do not reopen Phase 20 plans. Optional one-line note in executor SUMMARY only.

## Validation Architecture (Nyquist)

| Requirement | Primary automated proof | Expect after Phase 21 |
|-------------|-------------------------|------------------------|
| RT-01 | `IntegrationAllocScanTest` + no `dryBuffer.setSize` in process | green (MIDI store still present — alloc scan must not require its removal) |
| RT-02/03 | `Builds/Tests "[oversized-block]"` | **green** |
| RT-05 | New realtime span contract (control hold ≤128) | green |
| RT-08…13 | New authentic transition contract + existing XFADE/latency | green |
| RT-14 | `Builds/Tests "[realtime][stress]"` | green |
| RT-15 | Unit/contract early-return when unprepared | green |
| CORE-14…16 | `Builds/Tests "[true-bypass]"` | **green** |
| CORE-17 | `BypassCrossfadeTest` + transition click bound | green |
| CORE-18 | True-bypass + engaged non-bypass mono-first regression | green |
| Still red | `[midi-apvts]`, `[posthard]`, `[input-anchors]`, `[shipping-policy]` | **remain red** |
| Must stay green | `[pressure-release]`, `[send]`, `[release]`, `[DryPath]` | green |

## Sources

### Primary (verified in-repo)
- `source/PluginProcessor.cpp` processBlock / prepareToPlay / updateReportedLatency
- `source/GatedBloomChain.h`, `source/SchroederTank32.h`, `source/EngineCrossfade.h`, `source/BypassCrossfade.h`
- `tests/V1ContractOversizedBlockTest.cpp`, `tests/V1ContractTrueBypassTest.cpp`
- `tests/IntegrationAllocScanTest.cpp`, `tests/RealtimeStressTest.cpp`, `tests/EngineCrossfadeTest.cpp`, `tests/LatencyTest.cpp`
- `.planning/phases/SENDBLOOM-19-*/19-RESEARCH.md` defect table
- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` ADR-V1-05/07/10, §10.4–10.6
- `.planning/phases/SENDBLOOM-21-*/21-CONTEXT.md`

### Confidence
**HIGH** — defect loci and ADR remedies are verified in source; no new external APIs required.
