---
phase: 21-realtime-span-engine-true-bypass
verified: 2026-07-12T16:30:06Z
status: passed
score: 5/5 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification: false
---

# Phase 21: Realtime Span Engine & True Bypass Verification Report

**Phase Goal:** Host blocks of any size retain full wet processing without audio-thread allocation, and settled bypass is channel-preserving unity.

**Verified:** 2026-07-12T16:30:06Z  
**Status:** passed  
**Re-verification:** No — initial verification

**Success model (phase-specific):** `[oversized-block]` + `[true-bypass]` green; no-alloc span path; authentic RT-08…14 green; `[pressure-release]` remains green (Phase 20 regression).

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | ------- | ---------- | -------------- |
| 1 | `processBlock` performs zero heap allocations (including oversized blocks and engine-crossfade completion); dry-only oversized fallback and `dryBuffer.setSize` in process are gone | ✓ VERIFIED | Span loop + `processSpan` in `PluginProcessor.cpp`; `setSize` only in `prepareToPlay`; static extract of process bodies has no `.setSize` / `make_unique` / `resize` / `push_back` / `assign`; `[realtime][static][integration]` alloc-scan PASS (28 asserts); dry-only `numSamples > preparedMaxBlock_` branch absent |
| 2 | A 2048-sample host block prepared at 512 matches equivalent smaller-block renders within tolerance; wet remains nonzero for oversized blocks | ✓ VERIFIED | `[v1][contract][oversized-block][RT-02]` PASS (2 asserts) — wet continuity + chunked parity within contract tolerance |
| 3 | Control-rate reverb values update in spans of at most 128 samples; `preparedMaxBlock_ <= 0` is handled safely | ✓ VERIFIED | `kControlQuantum = 128`; span = `jmin(remaining, preparedMaxBlock_, kControlQuantum)`; `[RT-05]` + `[RT-15]` PASS in `V1ContractRealtimeSpanTest.cpp` |
| 4 | Settled bypass preserves each input channel at unity within floating tolerance and ignores Input, Distn, Gate, Level, and Output; transitions remain click-bounded; engaged mono-first behavior unchanged | ✓ VERIFIED | ADR-V1-10 order: `OutputStage` then `BypassCrossfade::mixSample(dryTap, engaged, engagedMix)` per channel; `[true-bypass]` PASS (4 asserts); `[parm][bypass]` click/mixSample/5 ms/mid-stream PASS (8 asserts / 5 cases) |
| 5 | Authentic-mode changes request exactly one engine target transition per parameter change; reported latency stays zero under ADR-003; crossfade begins in the first block after change, converges after rapid toggles, and resets only the idle engine with zero allocations; 10,000-block stress stays finite | ✓ VERIFIED | Block-start `snap.authenticColor != requestedAuthenticColor_` → single `requestEngineCrossfade`; `updateReportedLatency` → `setLatencySamples(0)`; idle reset in `SchroederTank32` via `targetIsFixedEngine()`; `[v1][contract][authentic]` RT-08/09/10/11 PASS (53 asserts / 3 cases); `[RT-14]` 10k authentic+bypass+oversized stress PASS (~10.9M asserts) |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | ----------- | ------- | ------- |
| `source/PluginProcessor.cpp` | span loop + processSpan; ADR-V1-10 mix; authentic snapshot edge | ✓ VERIFIED | `kControlQuantum` span loop; `processSpan` → `chain.processBlock`; Output then mixSample; requestedAuthenticColor_ edge |
| `source/PluginProcessor.h` | processSpan / preparedMaxBlock_ / requestedAuthenticColor_ / kControlQuantum | ✓ VERIFIED | `kControlQuantum = 128`; members + decls present |
| `source/BypassCrossfade.h` | mixSample(originalDry, processed, engagedMix) | ✓ VERIFIED | ADR-V1-10 formula; buffer helper calls mixSample |
| `tests/IntegrationAllocScanTest.cpp` | setSize banned in process bodies | ✓ VERIFIED | `requireNoAllocTokens` rejects `.setSize`; tag green |
| `tests/V1ContractRealtimeSpanTest.cpp` | RT-05 + RT-15 | ✓ VERIFIED | quantum == 128; unprepared safe |
| `tests/V1ContractOversizedBlockTest.cpp` | RT-02/03 oversized wet parity | ✓ VERIFIED | `[oversized-block]` green |
| `tests/V1ContractTrueBypassTest.cpp` | CORE-14…16 true bypass | ✓ VERIFIED | `[true-bypass]` green |
| `tests/BypassCrossfadeTest.cpp` | CORE-17 click bounds | ✓ VERIFIED | `[parm][bypass]` green |
| `tests/V1ContractAuthenticTransitionTest.cpp` | RT-08/09/10/11 | ✓ VERIFIED | `[v1][contract][authentic]` green |
| `tests/RealtimeStressTest.cpp` | RT-14 strengthened stress | ✓ VERIFIED | `[RT-14]` green |
| `tests/V1ContractPressureReleaseTest.cpp` | Phase 20 regression | ✓ VERIFIED | `[pressure-release]` still green (8 asserts) |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | --- | --- | ------ | ------- |
| `processBlock` span loop | `processSpan` | `offset += span` with `jmin(remaining, preparedMaxBlock_, kControlQuantum)` | ✓ WIRED | lines ~230–236 |
| `processSpan` | `GatedBloomChain::processBlock` | span ≤ preparedMaxBlock_ | ✓ WIRED | `chain.processBlock(...)` after latch sample 0 |
| `prepareToPlay` | scratch / dryBuffer capacity | sized to samplesPerBlock only | ✓ WIRED | `dryBuffer.setSize` only in prepare; `preparedMaxBlock_ = samplesPerBlock` |
| engaged path | `OutputStage::processSample` | gain before bypass crossfade only | ✓ WIRED | engaged built then `mixSample`; no Output after mix |
| `BypassCrossfade::mixSample` | buffer writeback | dry*(1-mix)+engaged*mix per channel | ✓ WIRED | extended + mono-first branches |
| `snapshot.authenticColor` | `chain.requestEngineCrossfade` | inequality vs `requestedAuthenticColor_` at block start | ✓ WIRED | one request then latch |
| `SchroederTank32` crossfade completion | idle `hostEngine.reset` / `fixedRate_.reset` | `targetIsFixedEngine()` | ✓ WIRED | resets only idle side |
| `updateReportedLatency` | `setLatencySamples(0)` | always Path B | ✓ WIRED | ignores target; always 0 |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| -------- | ------------- | ------ | ------------------ | ------ |
| `processSpan` wet path | `wetScratch_` | `chain.processBlock` from mono/envelope scratches | Yes — engine output, not static empty | ✓ FLOWING |
| bypass writeback | `dryTap` / `engagedMix` | per-channel `dryBuffer` + `bypassWetScratch_` from smoother | Yes — host input + APVTS smoother | ✓ FLOWING |
| authentic request | `snap.authenticColor` | `ParameterSnapshot::capture(apvts)` | Yes — live APVTS | ✓ FLOWING |
| pressure regression | send amount/connected | Phase 20 controller path unchanged in process | Yes — `[pressure-release]` still asserts | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| Oversized wet parity | `Builds/Tests "[oversized-block]"` | 1 case / 2 asserts PASS | ✓ PASS |
| True bypass unity | `Builds/Tests "[true-bypass]"` | 1 case / 4 asserts PASS | ✓ PASS |
| Pressure-release regression | `Builds/Tests "[pressure-release]"` | 1 case / 8 asserts PASS | ✓ PASS |
| Authentic RT-08…11 | `Builds/Tests "[v1][contract][authentic]"` | 3 cases / 53 asserts PASS | ✓ PASS |
| No-alloc static scan | `Builds/Tests "[realtime][static][integration]"` | 1 case / 28 asserts PASS | ✓ PASS |
| RT-14 stress | `Builds/Tests "[RT-14]"` | 1 case / ~10.9M asserts PASS | ✓ PASS |
| Bypass click / mixSample | `Builds/Tests "[parm][bypass]"` | 5 cases / 8 asserts PASS | ✓ PASS |
| Combined Phase 21 filter | oversized+true-bypass+pressure+authentic+span+stress | 17 cases / ~23.6M asserts PASS | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| ----- | ------- | ------ | ------ |
| — | — | No phase-declared `scripts/*/tests/probe-*.sh` | SKIPPED |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ---------- | ----------- | ------ | -------- |
| RT-01 | 21-01 | No audio-thread heap in process | ✓ SATISFIED | alloc-scan + process body static check |
| RT-02 / RT-03 | 21-01 | Oversized wet continuity / chunk parity | ✓ SATISFIED | `[oversized-block]` |
| RT-05 | 21-01 | ≤128 control quantum | ✓ SATISFIED | `[RT-05]` + `kControlQuantum` |
| RT-15 | 21-01 | Unprepared safety | ✓ SATISFIED | `[RT-15]` early return |
| CORE-14…16 | 21-02 | Settled true bypass unity / ignore Output | ✓ SATISFIED | `[true-bypass]` |
| CORE-17 | 21-02 | Click-bounded transitions | ✓ SATISFIED | `[parm][bypass]` |
| CORE-18 | 21-02 | Engaged mono-first unchanged | ✓ SATISFIED | mono-first dual-mono branch retained |
| RT-08…11 | 21-03 | Snapshot-edge authentic + latency + converge | ✓ SATISFIED | `[v1][contract][authentic]` |
| RT-12 / RT-13 | 21-03 | Idle-only reset; no heap on completion | ✓ SATISFIED | `SchroederTank32` idle reset + alloc-scan / stress |
| RT-14 | 21-03 | 10k stress finite under churn | ✓ SATISFIED | `[RT-14]` |

No orphaned Phase 21 requirements: all RT/CORE IDs mapped to this phase appear in plan `requirements:` fields.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| — | — | No TBD/FIXME/XXX in Phase 21 processor/bypass sources | — | None |
| `PluginProcessor.cpp` | ~277–278 | `(void) getNextAuthenticColorTarget()` | ℹ️ Info | Smoother still advanced but no longer drives requests (ADR-V1-07 intentional) |

### Human Verification Required

None. All roadmap success criteria are exercised by named Catch2 contract/stress tests that passed in this verification run. No `<human-check>` blocks in Phase 21 plans.

### Gaps Summary

None. Phase goal achieved: span/no-alloc oversized wet path, true-bypass unity, authentic RT-08…14, and pressure-release regression all green.

---

_Verified: 2026-07-12T16:30:06Z_  
_Verifier: Claude (gsd-verifier)_
