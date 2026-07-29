# Reverb-X Public Reference Catalogue and Listening Protocol

**Purpose:** Truthful catalogue of publicly available listening references for Reverb-X–class behaviour, plus a structured human listening protocol. This document does **not** upgrade fidelity classification, import third-party media into the repo, or treat streamed demos as calibrated measurements.

**Product mapping:** SendBloom (Niko Audio Labs) — behavioural inspiration only. See [`../CLAIM_STATUS.md`](../CLAIM_STATUS.md) and [`CLEAN_ROOM.md`](CLEAN_ROOM.md).

**Last updated:** 2026-07-29 (catalogue attempt `public-reference-catalog-attempt-1`)

---

## Classification and approval gates (unchanged)

| Gate | Status |
|------|--------|
| ADR-V1-17 fidelity classification | **`original-inspired`** — unchanged by this catalogue |
| Hardware capture / comparison grids | **`human_needed`** |
| Level-matched blind listening verdict (Niko) | **`human_needed`** |
| Stronger fidelity wording review | **`human_needed`** |

Public copy must remain generic. Do not claim exact emulation, hardware matching, or approved fidelity unless superseded by a new evidence-backed ADR-V1-17 closeout.

**Evidence boundary:** Only user-created captures of lawfully owned hardware may enter a formal capture package per [`reference-capture-protocol.md`](reference-capture-protocol.md). Third-party YouTube streams are **representative listening aids** only. No exact public downloadable dry/wet stem pair was identified in the reference search; do not describe looped before/after segments as exact pairs.

---

## How to use this catalogue

1. **Discovery** — pick a source and time-coded segment for the behaviour you are checking (onset, dark predelay, distortion, gate, Igor send).
2. **Pair status** — respect `representative wet`, `pseudo-pair`, or `no pair`; never level-match or A/B against SendBloom using lossy stream audio as ground truth.
3. **Confounds** — note amp/cab/mic/room, other pedals, and missing knob callouts before drawing DSP conclusions.
4. **Confidence** — treat **high-confidence behaviour targets** as alignment goals consistent with official docs; treat **medium/low** items as perceptual hypotheses until hardware capture or scorecard verdict.

Streams are lossy, may include post-processing, and often omit exact settings. They support coarse perceptual ranking and musician-facing sanity checks, not RT60/filter/transfer-curve calibration or a hardware fidelity claim.

---

## Official primary sources

| Source | URL | Role | Pair status | Confidence |
|--------|-----|------|-------------|------------|
| Rainger FX product page | https://www.raingerfx.com/shop/p/reverb-x | Stated behaviour: 5–6 s max decay; bright/immediate vs dark/pre-delay; wet-only blendable distortion; post hard gate; dry unaffected; Igor send with trails | no pair | **high** (manufacturer copy) |
| Reverb-X user manual (PDF) | https://www.raingerfx.com/s/Reverb-X-Manual.pdf | Control definitions and routing consistent with product page | no pair | **high** (manufacturer manual) |

**High-confidence behaviour targets (from official sources only):**

- Total ambience decay reaches **up to 5–6 seconds** maximum.
- **Bright:** immediate onset; **Dark:** clear **pre-delay** before the wash (SendBloom implementation target: ~55 ms predelay crossfade per [`fv1-reverb-architecture.md`](fv1-reverb-architecture.md)).
- **Distortion** affects the **wet path only**, blendable; dry path integrity preserved.
- **Gate** is **post**-reverb, **hard** close (edited-sample character).
- **Igor** momentary send: releasing control stops new excitation while **preserving** existing tank tail.

---

## Third-party video catalogue

Segments are **starting points** for listening; timestamps are as reported in the reference search (verify in player if chapters differ).

### TUNNEL OF REVERB — *Reverb X \| Rainger FX - Deep Dive In The Studio*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=iagH2FIFI6A |
| Content role | Best available **one-variable / pseudo-pair** walkthrough of individual controls; studio context |
| Pair status | **pseudo-pair** (before/after within same take; **no dry stem**; not level-certified) |
| Overall confidence | **medium** for control semantics; **low** for numeric calibration |

| Time (approx.) | Segment | Listening focus | Confounds |
|----------------|---------|-----------------|-----------|
| 1:44 | Level | Wet/dry blend feel | Room, amp chain |
| 2:11 | Size | Decay length / room size | Same |
| 2:50 | Distortion | Wet-only dirt on transients | Same |
| 3:08 | Input | Drive into reverb | Same |
| 3:45 | Output | Level / headroom | Same |
| 4:37 | Acoustic guitar | Short-decay bright-ish context | Mic, room |
| 7:21 | Dark switch | Predelay vs bright | **ADG-1 delay also in chain** — dark/predelay not isolated |
| 9:47 | Drum kit | Percussive bloom, gate context | Full kit mic, room |
| 11:10 | Gate on synth | Post-gate chop | Synth source, other FX unknown |
| 12:54 | Igor on glitch drums | Send release vs tail preservation | Performance layering |

### Knobs — *Rainger FX - Reverb-X*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=QrPpxCW4EzI |
| Content role | Overview, trails, gated mode, Igor, non-guitar source |
| Pair status | **representative wet** (not exact dry/wet pairs) |
| Overall confidence | **medium** perceptual |

| Time (approx.) | Segment | Listening focus | Confounds |
|----------------|---------|-----------------|-----------|
| 0:27 | Overview | General character | Guitar amp |
| 1:53 | Trails | Tail length / decay | Same |
| 2:53 | Gated mode | Hard gate edit | Same |
| 4:15 | Igor | Send / tail behaviour | Same |
| 4:44 | Non-guitar | Source generality | Source-dependent |

### collector//emitter — *Sound Study // Rainger FX - Reverb X*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=6IeXMyhjhlI |
| Content role | Performance-oriented **wet** examples |
| Pair status | **no pair** |
| Overall confidence | **low–medium** (no spoken settings, no dry stem) |

| Time (approx.) | Segment | Listening focus | Confounds |
|----------------|---------|-----------------|-----------|
| Full 6:30 video | Performance | Overall ambience texture, density impression | Full performance mix, settings unknown |

### Pedals and Effects — *Reverb-X by Rainger FX*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=8JS3rW3g-Qs |
| Content role | Broad guitar/bass demonstrations and commentary |
| Pair status | **no pair** (settings vary) |
| Overall confidence | **low–medium** |

| Time (approx.) | Segment | Listening focus | Confounds |
|----------------|---------|-----------------|-----------|
| (full video) | Guitar/bass demos | Musical context, brightness/gate impressions | Amp, bass chain, varying knobs |

### Effects Database — model index

| Field | Value |
|-------|--------|
| URL | https://www.effectsdatabase.com/model/rainger/reverbx |
| Content role | **Discovery index** linking multiple demos |
| Pair status | n/a |
| Overall confidence | **low** — not a measurement authority; use only to find URLs |

---

## Behaviour targets for DSP alignment (actionable, product-truth preserving)

These targets express **allowed clean-room goals**. They do not constitute proof that SendBloom matches hardware.

### High-confidence (official + architecture doc)

| Target | Acceptance direction |
|--------|----------------------|
| RT60 span | Total decay **reaches 5–6 s** at maximum Size; minimum usable tail consistent with FV-1-class ring (~1.2 s floor) per [`fv1-reverb-architecture.md`](fv1-reverb-architecture.md) |
| Bright onset | **Immediate** wash (no intentional predelay on Bright) |
| Dark onset | **Clear predelay** before main wash |
| Early tail | **Dense**, **non-fluttering** early reflection build (contrast with sub-200 ms loop failure mode) |
| Dark HF | Retains **useful upper-mid texture** while **decaying faster in HF** than Bright (not “dark = extreme lowpass”) |
| Distortion | **Wet-only**, **level-conscious**, **transient-emphasizing**; quiet trails and dry integrity preserved |
| Gate | **Post** reverb, **hard** close |
| Igor / send | Release stops **new** excitation; **existing tank tail** continues |

### Medium / low-confidence (perceptual, from demos — not metrics)

| Observation | Confidence | Notes |
|-------------|------------|-------|
| Gated synth has “sample” chop without killing pre-gate wash | medium | TUNNEL 11:10; verify on hardware capture |
| Igor on rhythmic material preserves ghost tails | medium | TUNNEL 12:54, Knobs 4:15 |
| Acoustic guitar stays articulate at moderate Size | low | TUNNEL 4:37; mic confound |
| Drum bloom feels wide without obvious pitch wobble | low | Multiple demos; no isolated wet |

**Prohibited inference:** Do not cite public video loudness, spectrum, or decay numbers as calibrated measurements. Do not use `/tmp` or other downloaded third-party audio as tracked project assets.

---

## Engine change and measured status

Within the **unchanged** 32,768-word FV-1 delay RAM budget and **unchanged total loop delay**, the engine now uses **two serial allpass filters per ring block** (instead of one) to increase early reflection density inside each block, consistent with Spin’s documented “2 allpass + delay” block pattern.

**Measured direction:** Bright 100–250 ms crest fell **7.331 → 6.805** and kurtosis **11.578 → 6.064**; Dark kurtosis fell **16.577 → 9.343**. Bright/Dark onset remained **6.409 / 61.462 ms**, total 0–4 s RMS moved by less than **0.05 dB**, and the unchanged ProperSRC HF ceiling passed at **1.415 < 1.500**. See [`fv1-reverb-architecture.md`](fv1-reverb-architecture.md).

**Acceptance boundary:**

1. Directional early-window density screen: **passed** against the prior built binary.
2. Automated topology, predelay, HF, finite-output, real-time, and regression suite: see the dated evidence report; these gates do not prove hardware fidelity.
3. **Human_needed:** Niko level-matched listening approval per scorecard below.

Status: **implemented and objectively regression-tested; listening and hardware comparison remain `human_needed`**.

---

## Structured listening scorecard (Niko)

**Prerequisites**

- Level-match A and B within **±0.2 dB** RMS (or agreed LUFS) on the comparison window.
- **Randomized A/B** order per trial; minimum one repeat per cell.
- Reference: lawful **hardware capture** when available; until then, SendBloom vs hardware or vs prior build — record which.
- Verdict field remains **`human_needed`** until a **dated** signed row exists below.

**Rating scale (0–4 per attribute)**

| Score | Meaning |
|-------|---------|
| 0 | Strongly wrong / missing |
| 1 | Clearly off |
| 2 | Usable but noticeable gap |
| 3 | Close; minor difference |
| 4 | Indistinguishable or preferred match |

**Attributes (rate each cell)**

| Attribute | What to listen for |
|-----------|-------------------|
| Onset / immediacy | Bright immediate vs Dark delayed wash |
| Grain / flutter | Periodic ripple in tail (4–8 Hz class) |
| Bloom / density | Early build-up, smear vs richness |
| Spectral character | Bright air vs Dark upper-mid retention |
| Decay shape | Length and curvature to silence |
| Distortion feel | Wet-only, transient bite vs tail cleanliness |
| Gate edit | Hard post chop, no pre-chop pump |
| Send trails | Igor release stops new input, keeps tail |
| Dry integrity | Dry path unchanged by wet dirt |
| Preference | Musical choice (optional tie-break) |

**Required cells (minimum set)**

| Cell ID | Mode / settings (hardware or preset doc) | Stimulus |
|---------|------------------------------------------|----------|
| bright-short | Bright, low Size | Short pluck |
| bright-long | Bright, high Size | Pluck or chord |
| dark-long | Dark, high Size | Same as bright-long |
| dirty-long | Distortion on, moderate–high, long decay | Transient-rich |
| post-gated | Gate post, burst then silence | Burst + silence |
| send-release | Igor press/release during decay | Rhythmic or sustained |

**Scorecard template**

| Date | Cell | A | B | Onset | Grain | Bloom | Spectrum | Decay | Dirt | Gate | Send | Dry | Pref | Notes | Verdict |
|------|------|---|---|-------|-------|-------|----------|-------|------|------|------|-----|------|-------|---------|
| human_needed | | | | | | | | | | | | | | | **human_needed** |

---

## Legal and reference-claim alignment

- Manufacturer URLs and manual are cited for **musician-facing behaviour**, not firmware provenance.
- Video entries are **third-party**; trademarks belong to their owners.
- This file lives under `docs/` as an internal citation record (allowlisted per [`CLEAN_ROOM.md`](CLEAN_ROOM.md) § Legal Metadata Controls).
- Aligns with [`reference-capture-protocol.md`](reference-capture-protocol.md): formal grids and metrics require owned hardware captures; public streams do not replace them.

---

## Changelog

| Date | Run ID | Change |
|------|--------|--------|
| 2026-07-29 | public-reference-catalog-attempt-1 | Initial catalogue, DSP targets, engine hypothesis, listening scorecard |
| 2026-07-29 | engine-two-allpasses-attempt-2 | Recorded accepted measured engine result; fidelity classification unchanged |
