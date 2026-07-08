# ADR-003: Proper 32 kHz SRC Architecture and PDC Policy

**Status:** ACCEPTED (upon Phase 17 completion)  
**Date:** 2026-07-08  
**Supersedes:** VERB-05 accumulator description for the production ProperSRC path

## Context

SendBloom's FV-1-style reverb character depends on running the Schroeder tank at the hardware reference rate of **32,768 Hz** while the host delivers audio at **44.1–96 kHz**. Milestone v1 promised **zero reported plugin latency** when 32 kHz color is off (CHN-04). RC1 ships with `authentic_color` defaulting off, preserving that contract.

Two paths exist inside `FixedRateAdapter`:

| Mode | Path | Reportable SRC delay |
|------|------|----------------------|
| `LegacyAccumulator` | Hold/decimate without bandlimited SRC | 0 samples |
| `ProperSRC` | r8brain upsample → `SchroederTankCore` @ 32 kHz → r8brain downsample | ~5,160–8,915 samples (~90–118 ms) |

The legacy accumulator was adequate for early prototyping but is **not** acceptable for production 32 kHz color. ProperSRC fixes high-frequency imaging at the cost of substantial **filter priming delay** in the r8brain resampler chain. Phase 17 locks architecture and host PDC policy before Phase 18 user enablement.

## Decision

### 1. SRC architecture — `FixedRateAdapter` sandwich

Adopt a three-stage sandwich implemented in `source/FixedRateAdapter.h`:

```
Host-rate block
  → RateConverterPair upsample (host → 32,768 Hz)
  → SchroederTankCore @ 32 kHz (sample-by-sample)
  → RateConverterPair downsample (32,768 Hz → host)
  → Host-rate wet output
```

`RateConverterPair` wraps two `CDSPResampler` instances (up and down). Round-trip priming delay is queried via `getRoundTripLatencySamples()`, which sums `getInLenBeforeOutPos(0)` on each stage — **not** `getLatency()` (r8brain's master class always returns 0).

### 2. Library — r8brain-free-src (MIT)

Use **r8brain-free-src** for all ProperSRC conversion. The dependency is pinned in `cmake/R8brain.cmake`:

```cmake
GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
```

Rationale: MIT license, already integrated in Phase 13, proven imaging reduction (SRC-06), and an authoritative priming-delay API. No custom polyphase SRC or alternate library for RC1.

### 3. PDC policy — Policy A (conditional host PDC)

**Adopt Policy A — Conditional Host PDC** via JUCE `AudioProcessor::setLatencySamples()`:

| Condition | Reported latency | Rationale |
|-----------|------------------|-----------|
| `authentic_color` off (RC1 default) | `setLatencySamples(0)` | Preserves CHN-04 / v1 zero-PDC promise when 32k path inactive |
| ProperSRC active (`authentic_color` on, target path) | `setLatencySamples(roundTripSrc)` | Honest host PDC so DAWs delay other tracks to align with SendBloom wet output |
| Runtime toggle (authentic edge) | Re-call `setLatencySamples` on the same edge as Phase 16 engine crossfade | Host must track mode changes without requiring session restart |
| During 35 ms engine crossfade | Report **target-path** latency (0 fading to host-rate engine; `roundTripSrc` fading to ProperSRC) | Minimizes host PDC reconfiguration churn vs reporting max of both paths |

Latency value at runtime: `FixedRateAdapter::getRoundTripLatencySamples()` → `RateConverterPair::getRoundTripLatencySamples()`, re-queried on every `prepareToPlay` because delay is **rate-dependent**.

Implementation wiring is Phase 17 plan 17-03 (`PluginProcessor.cpp`); this ADR is the policy gate LAT-02.

## Why accumulator / hold is insufficient

The `LegacyAccumulatorPath` holds samples and decimates without bandlimited filtering. That design fails production requirements:

1. **No bandlimited decimation** — energy aliases into the audible band, producing HF imaging around **14–15 kHz** (SRC-06). DIAG/SRC-06 measured **≥70% imaging reduction** with ProperSRC vs LegacyAccumulator at representative mix settings.
2. **Wrong time base** — stepping/hold scales delay lines against host samples, not true 32 kHz FV-1 timing, so tank density and modulation do not match hardware behavior.
3. **False zero latency** — the accumulator adds no reportable r8brain priming delay, but wet HF is corrupted; zero reported latency does not mean zero sonic cost.

ProperSRC trades ~90–118 ms filter priming for correct band limiting and authentic 32 kHz tank timing.

## Measured latency

Values measured **2026-07-08** with `maxHostBlock = 512` and default r8brain quality (`ReqTransBand` / `ReqAtten` as constructed in `RateConverterPair::prepare`). Method: after `prepare(hostRate, 512)`, sum `getInLenBeforeOutPos(0)` on upsampler and downsampler — identical to `RateConverterPair::getRoundTripLatencySamples()`. Cross-checked against legacy `getInLenBeforeOutStart(0)` (delta 0 at all four rates).

Canonical constants live in `source/SrcLatencyTable.h`; this table must stay in sync with that header.

| Host rate (Hz) | Round-trip samples | Wall time (ms) | Upsampler priming | Downsampler priming |
|----------------|-------------------:|---------------:|------------------:|--------------------:|
| 44,100 | **5208** | 118.1 | 3,558 | 1,650 |
| 48,000 | **5160** | 107.5 | 3,510 | 1,650 |
| 88,200 | **8915** | 101.1 | 7,116 | 1,799 |
| 96,000 | **8670** | 90.3 | 7,020 | 1,650 |

Observations:

- Latency is **not** a single constant across host rates; hard-coding 5,160 for all sessions is incorrect (especially 88.2/96 kHz).
- Wall-time milliseconds converge in the ~90–118 ms band because higher rates pack more samples into similar filter lengths.
- Legacy accumulator path remains **0** reportable SRC delay.

## Parallel wet/dry caveat

SendBloom uses a **parallel** topology: `dryBuffer` copies the dry guitar at block start (immediate), while the wet path traverses gating, distortion, and reverb (including SRC when ProperSRC is active). `setLatencySamples()` informs the **host PDC engine** to delay *other tracks* — it does **not** automatically time-align the internal dry tap with the SRC-delayed wet path.

When 32 kHz color is on, users may hear wet **behind** dry at high mix levels unless internal dry delay is added (Policy B, deferred). Live monitoring still hears SRC delay; hosts cannot pre-compensate live input through plugin-reported latency alone.

## Consequences

- **DAW alignment:** Other tracks shift when ProperSRC is active so printed mixes align with SendBloom's delayed wet output.
- **RC1 default:** `authentic_color` off → zero reported latency; marketing and CHN-04 remain truthful.
- **Toggle transients:** Brief PDC adjustment possible during the 35 ms engine crossfade when target-path reporting is used.
- **Phase 18 gate:** User-facing 32k enablement (Phase 18) requires LAT-02 acceptance of this policy.
- **Bypass:** If strict bypass latency matching is required (pluginval level 10), `processBlockBypassed` may need a matching delay line when `getLatencySamples() > 0` — flagged for Phase 18 if TEST-12 requires it.

## Alternatives considered

| Policy | Summary | Disposition |
|--------|---------|-------------|
| **A — Conditional host PDC** | 0 when off; measured round-trip when ProperSRC active | **Accepted** (this ADR) |
| **B — Internal dry delay** | Delay `dryBuffer` by `roundTripSrc` when authentic on | **Deferred (Extended)** — fixes in-plugin parallel phasing but adds buffer, toggle edge cases, and memory; document caveat now, revisit if listening tests show audible dry/wet phasing |
| **C — Zero always** | `setLatencySamples(0)` even when ProperSRC on | **Rejected** — violates honest PDC; wet path is ~5k+ samples late vs host timeline; breaks DAW alignment (Anti-Pattern 5) |
| **D — Defer ProperSRC ship** | Ship RC1 without enabling ProperSRC until latency accepted | **Fallback only** — product escape hatch if ~100 ms is unacceptable; not the engineering default after SRC-06 imaging fix |
| **E — Low-latency SRC preset** | Reduce r8brain quality for fewer priming samples | **Deferred (Extended)** — trades HF quality (SRC-06) for latency; not RC1 |

### Why Policy C was rejected

Reporting zero latency while ProperSRC consumes thousands of input samples before producing aligned output misleads hosts and users. `LatencyTest` passing at 0 with ProperSRC active would be a false negative. Honest reporting is required for mix-bus alignment.

### Why Policy B is deferred, not bundled

Policy A solves the **host** alignment problem with minimal DSP surface change. Policy B requires a delay line on the dry path, crossfade interaction with Phase 16 engine switching, and validation at all four host rates. LAT-02 allows documenting the parallel-mix caveat and deferring B unless listening tests fail assumption A2 in Phase 17 research.

## References

- [ADR-002](../../.planning/ADR-002-reverb-engine.md) — reverb engine strategy and host-rate vs character paths
- Phase 13 — `FixedRateAdapter` / `RateConverterPair` integration research
- `source/FixedRateAdapter.h` — ProperSRC sandwich implementation
- `source/RateConverterPair.h` — `getRoundTripLatencySamples()`
- `source/SrcLatencyTable.h` — measured sample-count constants (LAT-01)
- `cmake/R8brain.cmake` — r8brain CPM pin
- r8brain `CDSPResampler.h` — `getInLenBeforeOutPos`, `getLatency() == 0`
- [JUCE `AudioProcessor::setLatencySamples`](https://docs.juce.com/develop/classjuce_1_1AudioProcessor.html) — host PDC contract
