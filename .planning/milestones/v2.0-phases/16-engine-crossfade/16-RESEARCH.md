# Phase 16: Engine Crossfade - Research

**Researched:** 2026-07-08
**Domain:** Realtime DSP — dual reverb engine crossfade on `authentic_color` toggle
**Confidence:** HIGH

## Summary

Phase 16 closes the gap left by Phase 14's block-start `authenticColor` branching. Today, `PluginProcessor` latches `blockStartAuthentic` from smoothed sample 0 and `GatedBloomChain`/`SchroederTank32` hard-switch between `HostRateReverbEngine` and `FixedRateAdapter` (ProperSRC) with no output blend. That instant swap across incompatible delay-line states is the root cause of clicks, tail restarts, and potential non-finite spikes under rapid toggling.

The correct architecture is **dual-engine parallel crossfade** (not sequential switch-then-fade). Both engines are already prepared in `SchroederTank32::prepare()` (`hostEngine` + `fixedRate_`). On a toggle edge, run both engines on the same wet-send block buffer, blend outputs with a dedicated **20–50 ms** ramp (recommend **35 ms** default), then return to single-engine processing. Reset only the **idle** engine after the fade completes so the fading-out tail decays naturally and the idle engine does not leak stale state on the next toggle [CITED: `.planning/research/ARCHITECTURE.md` Pattern 3] [CITED: JUCE forum — parallel convolution engines during IR crossfade].

Click detection should reuse the repo's established **max adjacent sample delta** pattern from `BypassCrossfadeTest` and `PluginBasics`, scaled for wet reverb amplitude — not a hand-rolled Essentia/MAD detector. XFADE-02 extends `RealtimeStressTest` to **1000 explicit toggles** with finiteness, peak bound, alloc-scan coverage, and click metric gates.

**Primary recommendation:** Add `EngineCrossfade` state to `SchroederTank32`, drive it from per-sample `authenticColorTarget` edge detection in `PluginProcessor`, use block-level dual `processBlock` during the fade window, and gate with unit + integration tests patterned on existing bypass crossfade tests.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

_(None — smart discuss skipped; no user-locked implementation choices.)_

### Claude's Discretion

All implementation at Claude's discretion per XFADE-01/02. Reuse existing block-level integration from Phase 14. Crossfade duration 20–50 ms per ROADMAP.

### Deferred Ideas (OUT OF SCOPE)

- PDC policy — Phase 17
- User-facing enablement — Phase 18
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| XFADE-01 | Toggling `authentic_color` crossfades between engines over 20–50 ms without click above threshold | Dual-engine parallel architecture in `SchroederTank32`; dedicated `juce::SmoothedValue` ramp (35 ms default); equal-power wet blend; click metric via max adjacent delta (see Validation Architecture) |
| XFADE-02 | 1000-toggle stress test passes: no NaN/Inf, no heap alloc, no delay buffer overrun | Extend `RealtimeStressTest` pattern; 1000 toggles with `kBlockSizes` rotation; finiteness + peak `< 4.0f`; static alloc scan on new `processBlock` bodies; guard `numSamples > maxBlockSize_` paths |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Toggle edge detection | API / Backend (`PluginProcessor`) | — | Per-sample smoothed `authenticColorTarget` already advanced in processor first loop; edge belongs next to parameter smoothing, not UI |
| Dual-engine wet render | API / Backend (`SchroederTank32`) | `GatedBloomChain` routing | Both engines live in tank; crossfade is a reverb-output mix problem |
| Wet-send block assembly | API / Backend (`GatedBloomChain`) | — | Gates/pressure send stay host-rate per Phase 14; must feed block buffer to reverb during fade |
| Crossfade gain ramp | API / Backend (`SchroederTank32` or `EngineCrossfade.h`) | — | Independent of 15 ms param smoother; 20–50 ms engine morph |
| Click/stress validation | Test tier (`tests/`) | — | Catch2 unit + integration; no runtime click detector in product |
| PDC / latency reporting | — (Phase 17) | — | Explicitly deferred; crossfade must not call `setLatencySamples` |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce::SmoothedValue<float>` | 8 (bundled) | Engine crossfade gain ramp | Already used for bypass (`BypassCrossfade.h`, `SmoothedParameterBank.h`) [VERIFIED: codebase grep] |
| `HostRateReverbEngine` | in-tree | Host-rate wet path | Production path when `authentic_color` off [VERIFIED: `source/HostRateReverbEngine.h`] |
| `FixedRateAdapter` | in-tree | ProperSRC 32,768 Hz path | Production path when `authentic_color` on [VERIFIED: `source/FixedRateAdapter.h`] |
| Catch2 | 3.8.1 | Unit + stress tests | Project test runner [VERIFIED: `cmake/Tests.cmake`] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `BypassCrossfade` pattern | in-tree | Per-sample wet mix helper | Reference for `SmoothedValue` + block loop structure — adapt for dual wet paths |
| `IntegrationAllocScanTest` static scan | in-tree | Heap-allocation regression | After adding crossfade `processBlock` bodies (XFADE-02) |
| `kBlockSizes` stress array | in-tree | Variable block coverage | `{32,64,128,256,512,1024,256,128}` from `RealtimeStressTest.cpp` |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Dual-engine parallel crossfade | Sequential switch then fade | **Rejected** — incompatible delay states; instant switch causes click before fade can help |
| Dual-engine parallel crossfade | In-place delay rescale / coefficient morph | **Rejected** — host vs 32k fixed table lengths and SRC FIFO state are not morphable [VERIFIED: `.planning/research/PITFALLS.md` Pitfall 5] |
| 35 ms linear wet blend | Equal-power sin/cos blend | Equal-power preferred when engine levels may differ; linear acceptable if levels matched — use equal-power for XFADE-01 |
| Per-sample dual `processSample` during fade | Block dual `processBlock` + per-sample gain | Block dual preferred for ProperSRC FIFO efficiency; ~35 ms window keeps CPU spike bounded |

**Installation:** None — no new external packages.

**Version verification:** N/A (in-tree + JUCE 8 already vendored).

## Package Legitimacy Audit

> No external packages are installed in this phase.

| Package | Registry | Verdict | Disposition |
|---------|----------|---------|-------------|
| _(none)_ | — | — | N/A |

**Packages removed due to SLOP verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
APVTS authentic_color toggle
        │
        ▼
ParameterSnapshot (per processBlock)
        │
        ▼
SmoothedParameterBank::authenticColorTarget (15 ms UI smooth)
        │
        ▼
PluginProcessor first loop ──► detect 0.5 threshold crossing
        │                      (mid-block edge OK in Phase 16)
        ▼
GatedBloomChain::processBlock
   │ per-sample: gates, envelope, pressure send
   │ block: wetSendScratch_ ─────────────────────┐
   ▼                                              │
SchroederTank32::processBlock ◄───────────────────┘
   │
   ├─ [idle] single engine ──► hostEngine OR fixedRate_ (ProperSRC)
   │
   └─ [crossfading] parallel:
         hostEngine block out ──┐
                               ├── EngineCrossfade mix (20–50 ms)
         fixedRate_ block out ─┘
                │
                ▼
         reverbScratch_ ──► WetOverdrive ──► postGate ──► wetOut
```

### Recommended Project Structure

```
source/
├── EngineCrossfade.h          # NEW — equal-power wet mix + SmoothedValue state
├── SchroederTank32.h          # MODIFY — dual-engine block path, fade state machine
├── GatedBloomChain.h          # MODIFY — route block path when crossfading OR authentic on
└── PluginProcessor.cpp        # MODIFY — edge detect; stop block-start-only authentic latch

tests/
├── EngineCrossfadeTest.cpp    # NEW — unit fade + click metric (XFADE-01)
└── RealtimeStressTest.cpp     # MODIFY — 1000-toggle XFADE-02 case
```

### Pattern 1: Dual-Engine Parallel Crossfade (Required)

**What:** On `authentic_color` edge, process the same wet-send buffer through both `hostEngine` and `fixedRate_` (ProperSRC), blend outputs with a ramping mix gain over 20–50 ms, then run only the target engine.

**When to use:** Any swap between engines with non-interchangeable internal state (Schroeder delay lines + r8brain FIFOs).

**Why not sequential:** A fade after an instant switch still contains the switch discontinuity in the first sample.

**Example:**

```cpp
// Source: .planning/research/ARCHITECTURE.md Pattern 3; adapted for SendBloom
// Equal-power wet blend (g: 0 = host, 1 = fixed/ProperSRC)
const auto g = crossfadeSmoother.getNextValue();
const auto hostGain = std::cos (g * juce::MathConstants<float>::halfPi);
const auto fixedGain = std::sin (g * juce::MathConstants<float>::halfPi);
output[i] = hostScratch[i] * hostGain + fixedScratch[i] * fixedGain;
```

### Pattern 2: Dedicated Engine Crossfade Smoother (Not Param Smoother)

**What:** `authenticColorTarget` in `SmoothedParameterBank` is 15 ms — too fast for reverb state swap [VERIFIED: `source/SmoothedParameterBank.h` line 23]. Trigger a **separate** `juce::SmoothedValue<float>` (recommend `reset(sampleRate, 0.035)`) on boolean edge from smoothed target crossing 0.5.

**When to use:** Always for XFADE-01. Do not repurpose `authenticColorTarget` as the engine mix control.

### Pattern 3: GatedBloomChain Block-Path Gating

**What:** Phase 14 uses block reverb path only when `authenticColor == true` [VERIFIED: `source/GatedBloomChain.h` lines 83–112]. Extend condition to `authenticColor || reverb->isCrossfading()`.

**When to use:** Whenever crossfade is active so both engines receive the same `wetSendScratch_` block.

### Pattern 4: Idle-Engine Reset After Fade

**What:** When crossfade completes, call `reset()` on the **inactive** engine only (`fixedRate_.reset()` or host core reset) so the next activation starts clean without killing the audible tail on the active engine [CITED: `.planning/research/PITFALLS.md` Pitfall 5].

### Anti-Patterns to Avoid

- **Instant `authenticColor` branch swap:** Current `SchroederTank32::processBlock` hard branch — must not run when crossfade active.
- **Block-start-only authentic latch:** `blockStartAuthentic` in `PluginProcessor.cpp` lines 266–308 — replace with edge-driven crossfade for mid-block toggles (deferred from Phase 14).
- **Clearing both engines on toggle:** Causes tail restart; only idle engine resets post-fade.
- **Per-sample `FixedRateAdapter` with `n=1` during entire fade:** Destroys SRC FIFO efficiency; use block `processBlock` + per-sample gain apply.
- **Hand-rolled Essentia/MAD click detector in product:** Overkill; use test-time max-delta like existing bypass tests.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Equal-power stereo wet morph | Custom polynomial approximations | `sin`/`cos` half-Pi ramp (see Pattern 1) | Numerically stable; 35 ms × 48 kHz is cheap |
| Gain ramp | Manual linear increment | `juce::SmoothedValue<float>` | Matches bypass pattern; handles sample rate changes in `prepare()` |
| Click test metric | Essentia ClickDetector / MAD pipeline | Max adjacent `\|y[n]-y[n-1]\|` in fade window | Repo precedent in `BypassCrossfadeTest.cpp`, `PluginBasics.cpp` |
| Heap alloc detection | Runtime malloc hooks only | `IntegrationAllocScanTest` token scan + stress | Phase 13–14 pattern already gates TEST-09 |
| Mid-block sub-block splitter (initially) | Complex arbitrary split logic everywhere | Whole-block dual run while `isCrossfading()` | Simpler; CPU bounded to ~35 ms windows; optimize later only if profiled |

**Key insight:** Phase 16 is a **state-machine + mix** layer on top of Phase 14 wiring — do not reopen SRC, tank math, or gate/send topology.

## Common Pitfalls

### Pitfall 1: Using Param Smoother as Engine Crossfade

**What goes wrong:** 15 ms `authenticColorTarget` ramp switches engine routing while still crossfading too quickly — zip or pop on tail.

**Why it happens:** Smoother was designed for UI/automation zipper prevention, not incompatible DSP state swap.

**How to avoid:** Separate 20–50 ms `engineCrossfadeMix` smoother; trigger on threshold crossing.

**Warning signs:** HF regression passes steady-state but fails when toggling during held chord.

### Pitfall 2: Sequential Switch Before Crossfade

**What goes wrong:** First sample after toggle still clicks; fade only masks steady-state level difference.

**Why it happens:** Delay/SRC states jump discontinuously before blend starts.

**How to avoid:** Parallel dual `processBlock` before any single-engine path resumes.

**Warning signs:** Oscilloscope spike at toggle sample even with fade enabled.

### Pitfall 3: Scratch Buffer Underrun During Dual Run

**What goes wrong:** OOB write or silent output when `numSamples > maxBlockSize_`.

**Why it happens:** Dual path adds `hostScratch` + `fixedScratch` sized in `prepare()`.

**How to avoid:** Mirror existing guards in `FixedRateAdapter` and `GatedBloomChain`; size scratch to `maxBlockSize` from `prepare()`.

**Warning signs:** `RealtimeStressTest` "larger-than-prepared block" case fails with `authentic_color=1` during crossfade.

### Pitfall 4: Crossfade Active but GatedBloomChain Uses Per-Sample Path

**What goes wrong:** Host per-sample path bypasses block SRC; crossfade never engages ProperSRC block FIFO correctly.

**Why it happens:** `if (! authenticColor)` early return in `GatedBloomChain::processBlock`.

**How to avoid:** Block path when `isCrossfading()` even if target is host.

**Warning signs:** Authentic-off toggle produces no ProperSRC participation during fade-from-fixed.

### Pitfall 5: False XFADE-02 Pass (Adapter-Only Coverage)

**What goes wrong:** `FixedRateAdapter` alloc-free but new `SchroederTank32` crossfade branch allocates.

**How to avoid:** Extend `IntegrationAllocScanTest` to scan `SchroederTank32.h` and `EngineCrossfade.h`.

**Warning signs:** Stress test passes but CI alloc scan fails after merge.

## Code Examples

### Existing Bypass Crossfade Pattern (Reference)

```cpp
// Source: source/BypassCrossfade.h
for (int sample = 0; sample < numSamples; ++sample)
{
    const auto wet = wetMix.getNextValue();
    const auto dry = 1.0f - wet;
    wetSample = drySample * dry + wetSample * wet;
}
```

### Click Metric (Repo Precedent)

```cpp
// Source: tests/BypassCrossfadeTest.cpp — adapt threshold for engine crossfade
float maxAdjacentDelta = 0.0f;
for (int i = 1; i < numSamples; ++i)
    maxAdjacentDelta = std::max (maxAdjacentDelta,
                                 std::abs (out[i] - out[i - 1]));
REQUIRE (maxAdjacentDelta < kClickThreshold);
```

### Recommended Click Thresholds (XFADE-01)

| Test layer | Signal | Threshold | Rationale |
|------------|--------|-----------|-----------|
| Unit (`EngineCrossfadeTest`) | 1 kHz sine, wet level 0.5, single toggle mid-render | `maxAdjacentDelta < 0.25f` | Stricter than bypass; wet-only buffer |
| Unit (normalized) | Same, divide delta by `max(peak, 1e-6)` | normalized delta `< 0.5` | Scale-invariant; matches bypass relative intent |
| Integration (`RealtimeStressTest` XFADE-02) | Noise input, full plugin, 1000 toggles | max delta any block `< 1.0f` | Matches `PluginBasics` bypass toggle gate [VERIFIED: `tests/PluginBasics.cpp` line 177] |
| Integration finiteness | All samples | `std::isfinite` | Existing stress pattern [VERIFIED: `tests/RealtimeStressTest.cpp`] |
| Integration peak | All samples | `peak < 4.0f` | Matches tank limiter headroom [VERIFIED: `HostRateReverbEngine.h` `jlimit(-4,4)`] |

**Duration samples (verify in `prepare()`):**

| Rate | 20 ms | 35 ms (default) | 50 ms |
|------|-------|-----------------|-------|
| 44.1 kHz | 882 | 1544 | 2205 |
| 48 kHz | 960 | 1680 | 2400 |
| 96 kHz | 1920 | 3360 | 4800 |

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Block-start `authenticColor` bool | Per-block edge + crossfade state machine | Phase 16 | Mid-block toggles safe |
| Instant engine branch in `SchroederTank32` | Parallel dual-engine 20–50 ms blend | Phase 16 | XFADE-01 |
| 2000-block / 50-block toggle stress | 1000 explicit toggle counter | Phase 16 | XFADE-02 |
| 15 ms param smooth only | + 35 ms engine crossfade smoother | Phase 16 | Audible tail preserved |

**Deprecated/outdated:**
- `blockStartAuthentic` latch as sole routing control — replace with crossfade-aware routing.
- Expecting `RealtimeStressTest` "authentic color toggling" (2000 blocks, toggle every 50) to satisfy XFADE-02 — insufficient toggle count and no click metric.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | **35 ms** default fade (midpoint of 20–50 ms) is inaudibly smooth on guitar reverb tails | Pattern 2 | User may prefer 50 ms — still within ROADMAP bounds |
| A2 | Equal-power sin/cos blend is sufficient without latency alignment between engines | Pattern 1 | Phase 17 PDC may expose sub-sample misalignment; acceptable until LAT-* |
| A3 | Whole-block dual processing during crossfade is CPU-acceptable | Pattern 3 | May need sub-block optimization at 96 kHz / small buffers — profile if needed |
| A4 | Click threshold `0.25f` unit / `1.0f` integration is tight enough for "audible click above threshold" | Code Examples | May need tuning after listening — thresholds are test constants not user-facing |
| A5 | Edge trigger on smoothed `authenticColorTarget` crossing 0.5 matches user toggle timing | Pattern 2 | Raw APVTS edge would crossfade faster than UI smooth — confirm if automation tests fail |

## Open Questions

1. **Should `HostRateReverbEngine` gain a `processBlock` override wrapping `SchroederTankCore` loop?**
   - What we know: Dual-run during fade currently falls through `IReverbEngine` default per-sample loop for host path [VERIFIED: `source/IReverbEngine.h`].
   - What's unclear: Whether per-sample host loop during 35 ms × 512 block is CPU-safe at 96 kHz.
   - Recommendation: Implement block loop in fade path only; add override if profiler shows hot spot.

2. **Reset API on `FixedRateAdapter` vs full `prepare()` for idle engine**
   - What we know: `FixedRateAdapter::reset()` exists [VERIFIED: `source/FixedRateAdapter.h` line 70].
   - What's unclear: Whether host engine needs symmetric `reset()` on `SchroederTankCore`.
   - Recommendation: Mirror `fixedRate_.reset()` with core reset helper on host engine post-fade.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build tests | ✓ | 4.3.3 | — |
| CTest | XFADE-02 CI gate | ✓ | (bundled) | Run `./build/Tests` directly |
| Catch2 | All tests | ✓ | 3.8.1 | Fetched by CPM at configure |
| JUCE 8 | DSP / plugin | ✓ | vendored | — |

**Missing dependencies with no fallback:** none

**Missing dependencies with fallback:** none

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (Catch discovery) |
| Quick run command | `cd build && ctest -R "EngineCrossfade|XFADE|authentic color toggling" --output-on-failure` |
| Full suite command | `cd build && ctest --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| XFADE-01 | 20–50 ms crossfade without click above threshold | unit | `cd build && ./Tests "EngineCrossfade*" -r compact` | ❌ Wave 0 |
| XFADE-01 | Toggle produces bounded adjacent deltas | unit | same | ❌ Wave 0 |
| XFADE-01 | Full plugin single toggle click gate | integration | `cd build && ./Tests "authentic color crossfade" -r compact` | ❌ Wave 0 |
| XFADE-02 | 1000 toggles, finite output | integration | `cd build && ./Tests "1000-toggle" -r compact` | ❌ Wave 0 |
| XFADE-02 | No heap in crossfade processBlock | static scan | `cd build && ./Tests "integrated processBlock bodies have no heap allocation tokens"` | ✅ extend scan paths |
| XFADE-02 | No buffer overrun on variable blocks | integration | `cd build && ./Tests "RealtimeStressTest" -r compact` | ✅ extend case |

### Sampling Rate

- **Per task commit:** `ctest -R "EngineCrossfade|authentic color"` (when added)
- **Per wave merge:** `ctest --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `tests/EngineCrossfadeTest.cpp` — XFADE-01 unit fade curve + click threshold
- [ ] `source/EngineCrossfade.h` — testable mix helper
- [ ] `tests/RealtimeStressTest.cpp` — new `TEST_CASE` `"processBlock survives 1000 authentic_color toggles [XFADE-02]"` with 1000 toggles (not 2000 blocks), click metric, `kBlockSizes` rotation
- [ ] `tests/IntegrationAllocScanTest.cpp` — add `SchroederTank32.h`, `EngineCrossfade.h` to scan list
- [ ] `tests/SchroederTank32BlockTest.cpp` — optional dual-engine crossfade finite output case

### XFADE-02 Stress Test Design (Prescriptive)

```cpp
// Pattern source: tests/RealtimeStressTest.cpp — extend, do not replace TEST-09 cases
TEST_CASE ("processBlock survives 1000 authentic_color toggles",
           "[realtime][integration][stress][XFADE-02]")
{
    // prepare 48 kHz, maxBlock 1024 (match existing stress)
    // authenticColor toggles every block (worst case) for exactly 1000 toggles
    // rotate kBlockSizes each block
    // per sample: REQUIRE isfinite; track peak < 4.0f
    // per block: track maxAdjacentDelta; REQUIRE < 1.0f (integration click gate)
    // optional: REQUIRE_NOTHROW on every processBlock
}
```

**Differences from existing `"processBlock stress with authentic color toggling"`:**
- Existing: 2000 **blocks**, toggle every **50** blocks → 40 toggles only [VERIFIED: `tests/RealtimeStressTest.cpp` lines 156–185]
- XFADE-02 requires: **1000 toggles**, variable block sizes, click + finiteness + peak gates

## Security Domain

> DSP-only phase; no new network, auth, or user input surfaces. `security_enforcement` noted for completeness.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | no | APVTS bool already typed; no new parameters |
| V6 Cryptography | no | — |

### Known Threat Patterns

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Buffer overrun via `numSamples > maxBlockSize_` | Tampering | Early return guards (existing pattern) |
| Denormal CPU spike | Denial of Service | `ScopedNoDenormals` already in processor |

## Sources

### Primary (HIGH confidence)

- `source/SchroederTank32.h`, `source/GatedBloomChain.h`, `source/PluginProcessor.cpp` — current routing and Phase 14 deviation [VERIFIED: codebase read]
- `source/SmoothedParameterBank.h`, `source/BypassCrossfade.h` — smoother + crossfade patterns [VERIFIED: codebase read]
- `tests/RealtimeStressTest.cpp`, `tests/BypassCrossfadeTest.cpp`, `tests/PluginBasics.cpp` — stress and click metrics [VERIFIED: codebase read]
- `.planning/REQUIREMENTS.md` XFADE-01/02 — acceptance criteria [VERIFIED: file read]
- `.planning/research/ARCHITECTURE.md` Pattern 3 — dual-engine crossfade [VERIFIED: file read]

### Secondary (MEDIUM confidence)

- [JUCE forum — Avoiding clicks while switching impulse responses](https://forum.juce.com/t/avoiding-clicks-while-switching-impulse-responses/37443) — parallel engines during crossfade [CITED]
- [Dusk Audio DuskVerb](https://duskaudio.com/plugins/duskverb/) — 50 ms equal-power preset crossfade, dual pre-allocated engines [CITED]

### Tertiary (LOW confidence)

- Essentia ClickDetector / MAD differentiator literature — informative only; **not** recommended for SendBloom tests [ASSUMED]

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — entirely in-tree; patterns verified in source
- Architecture: HIGH — dual parallel crossfade matches project research + industry practice
- Pitfalls: HIGH — Phase 14 documented the exact gap; codebase confirms block-start latch

**Research date:** 2026-07-08
**Valid until:** 2026-08-07 (stable DSP patterns; 30 days)
