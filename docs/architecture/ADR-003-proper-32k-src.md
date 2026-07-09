# ADR-003: Proper 32 kHz SRC Architecture and PDC Policy

**Status:** ACCEPTED (Path B / low-latency ProperSRC)  
**Date:** 2026-07-09  
**Supersedes:** VERB-05 accumulator description for the production ProperSRC path; prior Policy A ~100 ms default-r8brain measurement

## Context

SendBloom's FV-1-style reverb character depends on running the Schroeder tank at the hardware reference rate of **32,768 Hz** while the host delivers audio at **44.1–96 kHz**. Milestone v1 promised **zero reported plugin latency** (CHN-04). RC1 ships with `authentic_color` defaulting off.

Two paths exist inside `FixedRateAdapter`:

| Mode | Path | Wet-only SRC delay (host domain) |
|------|------|----------------------------------|
| `LegacyAccumulator` | Hold/decimate without bandlimited SRC | 0 samples (but HF imaging) |
| `ProperSRC` | r8brain upsample → `SchroederTankCore` @ 32 kHz → r8brain downsample | ~181–372 samples (~3.9–4.1 ms) with `kProperSrcQuality` |

## Decision

### 1. SRC architecture — `FixedRateAdapter` sandwich

```
Host-rate block
  → RateConverterPair upsample (host → 32,768 Hz)
  → SchroederTankCore @ 32 kHz (sample-by-sample)
  → RateConverterPair downsample (32,768 Hz → host)
  → Host-rate wet output
```

### 2. Library — r8brain-free-src (MIT)

Pinned in `cmake-local/R8brain.cmake`:

```cmake
GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
```

### 3. SRC quality — `kProperSrcQuality` (Path B / Policy E)

Default r8brain construction (`ReqTransBand=2%`, `ReqAtten≈207 dB`, **linear-phase**) produced **~90–118 ms** priming — rejected as a product tradeoff.

Production ProperSRC uses:

| Knob | Value | Rationale |
|------|------:|-----------|
| `ReqTransBand` | **25%** | Shorter FIR vs default 2%; guitar wet path does not need mastering brickwall |
| `ReqAtten` | **90 dB** | Stopband for SRC-06 imaging + DIAG-03/04 HF gates |
| `ReqPhase` | **`fprLinearPhase`** | Stable multi-rate HF metrics (DIAG-04); still ~25× faster than default r8brain |

Defined in `source/RateConverterPair.h` as `kProperSrcQuality`.

### 4. Host-domain latency accounting

`getInLenBeforeOutPos(0)` on the **upsampler** is already in **host samples**. On the **downsampler** it is in **32,768 Hz samples**. Round-trip host delay is:

```
upHost + downInternal * (hostRate / 32768)
```

Naively summing the two integers (prior ADR) mixed sample domains and inflated the reported figure.

### 5. PDC policy — Path B (always report zero)

| Condition | Reported latency | Rationale |
|-----------|------------------|-----------|
| Any mode | `setLatencySamples(0)` | Preserves CHN-04; wet-only delay ~2.5 ms is musically acceptable for parallel reverb |
| Diagnostics | `getRoundTripLatencySamples()` / `SrcLatencyTable` | LAT-01 still measures real priming |

Policy A (report SRC delay when authentic on) and Policy B (delay dry by SRC amount) remain available if listening tests reject Path B.

## Why accumulator / hold is insufficient

1. **No bandlimited decimation** — HF imaging around **14–15 kHz** (SRC-06). ProperSRC must reduce imaging by **≥70%** vs LegacyAccumulator.
2. **Wrong time base** — hold/step is not true 32 kHz tank timing.
3. Downstream anti-image SVF is cleanup, not a fix.

## Measured latency (`kProperSrcQuality`, maxHostBlock=512, 2026-07-09)

| Host rate (Hz) | Round-trip samples | Wall time (ms) |
|----------------|-------------------:|---------------:|
| 44,100 | **181** | 4.10 |
| 48,000 | **186** | 3.88 |
| 88,200 | **363** | 4.12 |
| 96,000 | **372** | 3.88 |

Canonical constants: `source/SrcLatencyTable.h`.

## Parallel wet/dry caveat

Dry tap is still immediate. ~4 ms wet lag is typically inaudible for gated ambience; revisit Policy B (internal dry delay) only if listening fails.

## Alternatives considered

| Policy | Disposition |
|--------|-------------|
| **A — Conditional host PDC** | Deferred — unnecessary once Path B quality lands |
| **B — Internal dry delay** | Deferred |
| **C — Zero always with default r8brain** | Rejected — lied about ~100 ms wet delay |
| **E — Low-latency SRC preset** | **Accepted** (this ADR) |

## References

- `source/FixedRateAdapter.h`, `source/RateConverterPair.h`, `source/SrcLatencyTable.h`
- `cmake-local/R8brain.cmake`
- r8brain `CDSPResampler` — `ReqTransBand` / `ReqAtten` / `fprMinPhase`
