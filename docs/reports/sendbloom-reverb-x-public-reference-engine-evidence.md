# SendBloom Reverb-X Public-Reference Engine Evidence

**Date:** 2026-07-30

**Starting revision:** `0c0cbdc6987bef5a7e2a4f74c07e1f89a0f6ebaf`

**Candidate base revision:** `efb5c5df0bb5e5a9241c4ce74929f04132c26c1b` plus the hashed, uncommitted candidate files in the metric receipt

**Fidelity status:** `original-inspired`

**Hardware comparison / human verdict:** `human_needed`

**Current engine status:** The two-allpass density and 5× wet-dirt candidates were
rejected in interactive level-matched listening: the prior baseline was preferred
in all four cells. The working tree has been reverted to the baseline engine;
the measurements below are retained as historical candidate evidence.

## Outcome

The public search established product behaviour and useful listening anchors,
but found no sample-aligned, downloadable dry/wet stem pair. Public streams are
lossy, edited, level-uncalibrated, and include undocumented performance and
amp/cab/room variables. They therefore set behavioural priorities; they are not
used as numeric golden audio or redistributed in this repository.

Two offline engine candidates addressed the strongest supported gaps:

1. A ring variant followed the vendor-published two-allpass-plus-delay block
   pattern and lowered early-window crest and kurtosis objectively.
2. A wet-only reciprocal-dirt variant moved breakup into ordinary bloom levels
   and normalized polarity below its knees.

Neither candidate survived the structured listening screen, so neither is the
current shipping engine.

The detailed source hierarchy, verified time codes, pair classifications, and
listening scorecard are in
[`../reverb-x-public-reference-catalog.md`](../reverb-x-public-reference-catalog.md).
The complete machine-readable before/after receipt is
[`data/sendbloom-engine-metrics-2026-07-30.json`](data/sendbloom-engine-metrics-2026-07-30.json).

## Reference-derived targets and engineering decisions

Manufacturer material supports the 5–6 second maximum, Bright/immediate versus
Dark/predelayed behaviour, clean dry plus wet-only overdrive, movable hard gate,
and Igor release that stops new send while preserving an existing tail. Public
demos additionally make dense long trails, hard gate edits, dry articulation,
and increasingly obvious wet grit the priority listening anchors.

Numeric onset, damping, density, knee, and gate-time values below are SendBloom
engineering targets, not measurements of the pedal.

| Decision | Evidence and result |
|---|---|
| Keep 1.2–6.0 s Size mapping | Public material establishes only the maximum. Requested 0.25/0.40/0.60 s targets collapse to approximately 0.90/0.90/0.94 s Bright and 0.88/0.88/0.90 s Dark in the current ring, so a lower displayed floor would create a dead control region. |
| Keep 55 ms Dark predelay and current damping | Current rendered onset is 6.409 ms Bright / 61.462 ms Dark and preserves the documented relative mode contrast. Public streams cannot justify a finer numeric retune. |
| Complete two-allpass ring blocks | Spin documents two allpasses plus delay per block; exact clean-baseline renders show a large density-direction improvement without moving onset or materially moving RMS. |
| Recalibrate wet dirt | Public sources call for clean-to-obvious wet overload. Moving the reciprocal knees from +0.303/−0.333 to +0.182/−0.200 makes normal blooms reach the bend, while branch normalization removes unintended low-level polarity tilt. |
| Retain the locked dirty-branch HP | The 100 Hz ADR-V1-15 / DSP-09 cutoff remains explicit and is now asserted directly; its response test stays below the clip knees so drive calibration cannot move the filter contract. It never touches the dry or clean-wet path. |

## Objective engine evidence

Both sides of the density comparison were built Release/arm64 with Apple clang
21.0.0 in clean or explicitly hashed states. The baseline is an isolated clean
worktree at the exact starting revision; its renderer SHA-256 is
`8fae57d9768a8d925c1e3a122c974bfb08c44ee8b1add422761c5afb92d5e88a`.
The candidate renderer SHA-256 is
`d4ddfbf2f5914907aee63f6449b8b36a16f1e3763b6a693141acd4e5c88e3907`.

Deterministic 32,768 Hz ring impulses, target RT60 3.0 s, four-second render,
normalized Schroeder fit over −5…−35 dB:

| Metric | Single-allpass baseline | Candidate |
|---|---:|---:|
| Bright crest, 100–250 ms | 7.3300 | **6.8039** |
| Bright kurtosis, 100–250 ms | 11.5753 | **6.0626** |
| Dark crest, 100–250 ms | 7.0463 | **6.9240** |
| Dark kurtosis, 100–250 ms | 16.5733 | **9.3407** |
| Bright T30 / fit R² | 3.0023 s / 0.99890 | 2.9103 s / 0.99884 |
| Dark T30 / fit R² | 2.6659 s / 0.99219 | 2.5777 s / 0.99204 |
| Bright onset | 6.4087 ms | 6.4087 ms |
| Dark onset | 61.4624 ms | 61.4624 ms |
| Bright RMS change | baseline | −0.045 dB |
| Dark RMS change | baseline | −0.023 dB |

The frozen ProperSRC HF ratio is `1.4148 < 1.5000`; the ceiling was not raised.

Wet-dirt transfer and deterministic active-chain fixtures compare the exact
pre-change `efb5c5d` test binary with the hashed candidate:

| Metric | Before | Candidate |
|---|---:|---:|
| Positive / negative quiet slope | 1.32 / 1.20 | **1.20 / 1.20** |
| Positive / negative knee | +0.303 / −0.333 | **+0.182 / −0.200** |
| Stateless scan peak, `x ∈ [-4, 4]` | 0.770 | **0.468** |
| Guitar-pluck dirty/clean tail RMS | 1.173 | **1.111** |
| 220 Hz decay dirty/clean tail RMS | 1.052 | **0.996** |
| DI-transient dirty/clean tail RMS | 1.152 | **1.096** |
| Sub-knee 30 Hz / 1 kHz dirty-branch RMS | contract `<0.250` | **0.244** |

This is earlier, more bounded compression rather than a volume boost: quiet
signals remain straight and equal on both polarities; 0.20–0.25 blooms enter the
reciprocal bend; maximum output and fixture tail swell fall.

## Real-time and product-truth validation

- Full CTest: **289 discovered; 288 passed; 1 capability-dependent ZIP test
  skipped; 0 failed**.
- `scripts/verify-v1.sh`: **GREEN**, including legal/reference-claim checks,
  Release AU/VST3 artifacts, ENAB acceptance, and VST3 pluginval strictness 10.
- Focused realtime suite: **10 cases / 25,546,419 assertions passed**; fixed-rate
  random-block, bypass, oversized-block, dry-null, gate, send-release, HF, and
  finite-output contracts passed.
- Three alternating hidden-benchmark runs per binary across 80 cells measured a
  **+5.5% median** all-cell CPU ratio and **+5.5% median** one-instance ratio.
  Candidate maximum one-instance aggregate was 0.808% of one core; maximum
  32-instance aggregate was 25.49%. The benchmark renders only 0.1 s and has no
  deadline assertion, so this is comparative diagnostic evidence, not a
  real-time certification.
- The evaluated candidate added no containers, allocation, locks, logging, file
  I/O, or data-dependent loops to sample processing. Existing “allocation” tests
  are source/finite-output checks, not allocator interception; that limitation is
  retained explicitly rather than overstated.
- The built AU and VST3 bundles pass strict code-sign verification after local
  ad-hoc signing. The candidate VST3 passed pluginval strictness 10. The installed
  AU executable hash does not match this candidate, so `auval` was not claimed
  against the build and remains `human_needed` unless the candidate is installed.

## Structured listening status

### Interactive screen — 2026-08-01

Niko auditioned all four level-matched A/B cells directly in chat. The answer key
was hidden until each response. The prior baseline was preferred in every cell:

| Cell | Selection | Revealed preferred build |
|---|---|---|
| Bright density | A | baseline |
| Dark density | B | baseline |
| Dirt pluck | A | baseline |
| Long Dark dirt chord | B | baseline |

The candidate therefore remains rejected as a perceptual improvement, despite
its favorable offline crest/kurtosis and transfer-curve metrics. The listener
did not attribute the first two choices to a meaningful RMS mismatch; all four
pairs were matched within the pack's documented tolerance.

`artifacts/listening/sendbloom-reverbx-2026-07-30/` is a deterministic,
randomized, RMS-matched A/B pack made only from repository-owned stimuli. Its
density cells compare the clean starting revision to the candidate; its dirt
cells compare `efb5c5d` to the candidate. The manifest records renderer and WAV
hashes, settings, order, and answer key. The included scorecard directs the
listener to the exact public-reference time codes without embedding third-party
audio. This is a focused engine-change screen, not the full gate/send/dry
hardware-approval matrix; those unchanged behaviours remain covered by automated
contracts and the complete human scorecard remains `human_needed`. Twelve mono
packed-24-bit WAVs across four cells were checked against the
manifest; maximum A/B RMS mismatch after quantization is
`0.0000000232 dB`, and the highest peak is `-1.0000009 dBFS`. The manifest
SHA-256 is
`3831fac9b80d1d0c0a7f86d9662bc80d921cb864cd48b4fcd8b8f8b827fb0187`.

The interactive listening screen is now recorded: baseline preferred in all four
candidate cells. Niko's owned-hardware capture grid remains the only gate that
can move the fidelity status beyond `original-inspired`.

## Truth boundary

The historical receipts prove lower early-window crest/kurtosis and more
intentionally engaged wet dirt for the rejected candidate. They do **not** prove
that those changes improved perception, a numeric distance to Reverb-X, an
installed-AU result, DAW soak, or Windows/Linux CI. The baseline engine is the
current working-tree behavior.
