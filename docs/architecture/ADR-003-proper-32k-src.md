# ADR-003: Proper 32 kHz SRC Architecture and PDC Policy

**Status:** ACCEPTED — amended 2026-08-10 for truthful normal host PDC
**Date:** 2026-07-13
**Supersedes:** VERB-05 accumulator description for the production ProperSRC path; prior Policy A ~100 ms default-r8brain measurement

## Context

SendBloom runs its Schroeder tank at a fixed **32,768 Hz** while the host delivers audio at **44.1–96 kHz**. This is the project's strongest current engineering approximation for host-rate-independent digital-pedal behavior; it is not a published or verified hardware specification. A prior development policy reported zero latency even though the production ProperSRC path has measurable priming. That was not a truthful normal-host PDC contract.

The production adapter exposes one path. Additional modes compile only into diagnostics:

| Mode | Path | Wet-only SRC delay (host domain) |
|------|------|----------------------------------|
| Production `ProperSRC` | r8brain conversion → `SchroederTankCore` @ 32,768 Hz → r8brain conversion | ~181–372 samples (~3.9–4.1 ms) with `kProperSrcQuality` |
| Diagnostic `LegacyAccumulator` | Hold/decimate without bandlimited SRC | 0 samples (but HF imaging) |

## Decision

### 1. SRC architecture — `FixedRateAdapter` sandwich

```
Host-rate block
  → RateConverterPair upsample (host → 32,768 Hz)
  → SchroederTankCore @ 32 kHz (sample-by-sample)
  → RateConverterPair downsample (32,768 Hz → host)
  → Host-rate wet output
```

`SchroederTank32` always invokes this ProperSRC block path. The shipping parameter layout contains no engine selector, and the production chain contains no host/fixed crossfade. DARK is the sole reverb-voicing switch. RT60 and damping remain smooth; the legacy path's 9-bit quantization is diagnostic only.

### 2. Library — r8brain-free-src (MIT)

Pinned in `cmake-local/R8brain.cmake`:

```cmake
GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
```

### 3. SRC quality — `kProperSrcQuality`

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

### 5. PDC policy — truthful normal host alignment

| Condition | Reported latency | Rationale |
|-----------|------------------|-----------|
| Production ProperSRC | Live `RateConverterPair::getRoundTripLatencySamples()` after `prepareToPlay` | Host PDC sees the actual prepared priming; direct, APVTS-bypass, and host-bypass paths receive the same fixed delay |
| Diagnostics | The same live query; `SrcLatencyTable` remains a canonical-rate regression table | Avoids a divergent measurement policy |

The production path has no latency-free topology. It must not map VST3
Low-Latency Mode or imply a zero-latency bypass route. A future low-latency
route requires a separately measured topology and a host-specific verification
receipt; normal PDC remains correct in the meantime.

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

## Parallel wet/dry alignment

The direct tap remains clean, pre-input-gain, ungated, and undistorted. It is
delayed by the live prepared SRC amount only to align it with the wet return;
that delay also applies during APVTS and host bypass so a host cannot receive
an early direct signal relative to reported PDC.

## Alternatives considered

| Policy | Disposition |
|--------|-------------|
| **A — Normal host PDC plus direct-path alignment** | **Accepted** |
| **B — Unreported wet-only delay** | Rejected — breaks wet/dry timing and makes host PDC false |
| **C — Zero always with default r8brain** | Rejected — lied about ~100 ms wet delay |
| **E — Short-quality ProperSRC preset** | **Accepted**; it remains nonzero and is reported truthfully |

## References

- `source/FixedRateAdapter.h`, `source/RateConverterPair.h`, `source/SrcLatencyTable.h`
- `cmake-local/R8brain.cmake`
- r8brain `CDSPResampler` — `ReqTransBand` / `ReqAtten` / `fprMinPhase`
