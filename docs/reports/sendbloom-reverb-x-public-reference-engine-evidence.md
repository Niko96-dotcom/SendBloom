# SendBloom Reverb-X Public-Reference Engine Evidence

**Date:** 2026-07-29

**Starting revision:** `0c0cbdc6987bef5a7e2a4f74c07e1f89a0f6ebaf`

**Fidelity status:** `original-inspired`

**Human/hardware comparison:** `human_needed`

## Outcome

Public manufacturer material establishes control behaviour and a 5–6 second
maximum decay, but the searched public demos do not provide a calibrated,
downloadable dry/wet stem pair. They therefore informed behavioural priorities,
not numeric fitting. The highest-leverage clean-room engine correction was to
complete the vendor-documented two-allpass-per-ring-block topology.

The accepted implementation preserves the 27,086-sample loop, 31,532-word RAM
use, 496.9 ms plain-delay loop, taps, gain, shelves, parameter mapping, and
Bright/Dark onset. It adds no allocation, lock, file I/O, or unbounded work to
the sample path.

## Objective before/after evidence

Deterministic `RenderTank` impulse renders, 32,768 Hz, 3.0 s target, 4.0 s
measurement window:

| Metric | Prior built binary | Accepted candidate |
|---|---:|---:|
| Bright crest, 100–250 ms | 7.3308 | 6.8046 |
| Bright kurtosis, 100–250 ms | 11.5777 | 6.0639 |
| Dark crest, 100–250 ms | 7.0470 | 6.9247 |
| Dark kurtosis, 100–250 ms | 16.5767 | 9.3426 |
| Bright T30 | 3.0023 s | 2.9103 s |
| Dark T30 | 2.6659 s | 2.5777 s |
| Bright onset | 6.4087 ms | 6.4087 ms |
| Dark onset | 61.4624 ms | 61.4624 ms |
| Bright total RMS | 0.00203387 | 0.00202328 (−0.045 dB) |
| Dark total RMS | 0.00183578 | 0.00183085 (−0.023 dB) |

The first symmetric split was rejected because the frozen ProperSRC HF ratio
regressed to 1.655 against a 1.500 ceiling. The accepted asymmetric split reads
1.415; the prior binary reads 1.122. The ceiling was not changed.

## Structured listening status

The scorecard and source/time-code catalogue live in
[`../reverb-x-public-reference-catalog.md`](../reverb-x-public-reference-catalog.md).
No agent listening verdict is claimed: the available audio analysis path could
not audition the downloaded streams, and lossy public demos are not calibrated
hardware captures in any case. Level-matched randomized listening by Niko and
the owned-hardware grid remain explicit approval gates.

A deterministic prior/current wet-only pack was rendered outside the repository
at `sendbloom-reverbx-listening/`: Bright short, Bright long, Dark long, and
Dirty long. Every WAV is fixed-gain trimmed to exactly -8.000 dB RMS; no EQ,
compression, or loudness normalization was applied. The pack compares the code
change, not SendBloom against Reverb-X hardware.

## Validation receipts

- Focused density contract: 15 assertions passed.
- Frozen ProperSRC HF regression: `1.4148 < 1.5000` passed; prior binary
  `1.1222`; rejected first candidate `1.6547`.
- Real-time: 25,546,419 assertions across 10 cases passed.
- Stress: 22,974,007 assertions across 3 cases passed.
- Allocation checks: 4,122 assertions across 3 cases passed.
- Full CTest: 285 discovered, 284 passed, 1 capability-dependent ZIP test
  skipped, 0 failed.
- `scripts/verify-v1.sh`: GREEN, including Release AU/VST3 artifacts, legal and
  reference-claim checks, ENAB acceptance, and VST3 pluginval strictness 10.

## Worker ledger

Cursor workers supplied candidates only; Sol/Codex reran decisive tests and
accepted or rejected each result.

| Run ID | Model | Role | Result |
|---|---|---|---|
| `scout-engine-tests` | `composer-2.5` | Read-only topology/test scout | Accepted as inventory input |
| `engine-two-allpasses-attempt-1` | `cursor-grok-4.5-high` | Initial implementation | Rejected as-is on HF regression |
| `public-reference-catalog-attempt-1` | `composer-2.5` | Catalogue draft | Reviewed and corrected |
| `engine-two-allpasses-attempt-2` | `cursor-grok-4.5-high` | Narrow correction | Worker stalled; partial delay split independently completed and verified |

Run logs and metadata were captured outside the repository under `/tmp`; no
third-party media or worker scratch files are tracked.

## Truth boundary

This evidence proves a denser, regression-safe implementation aligned with
publicly documented architecture and product behaviour. It does **not** prove
perceptual identity with Reverb-X, replace owned-hardware captures, or authorize
stronger public fidelity claims.
