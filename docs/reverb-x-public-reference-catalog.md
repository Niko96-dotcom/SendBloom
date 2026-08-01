# Reverb-X Public Reference Catalogue and Listening Protocol

**Purpose:** Truthful catalogue of publicly available listening references for Reverb-X–class behaviour, plus a structured human listening protocol. This document does **not** upgrade fidelity classification, import third-party media into the repo, or treat streamed demos as calibrated measurements.

**Product mapping:** SendBloom (Niko Audio Labs) — behavioural inspiration only. See [`../CLAIM_STATUS.md`](../CLAIM_STATUS.md) and [`CLEAN_ROOM.md`](CLEAN_ROOM.md).

**Last updated:** 2026-07-30
**Public-source retrieval date:** 2026-07-30

**Implementation follow-up:** The two-allpass density and recalibrated wet-dirt
candidate described below were tested interactively and rejected: the prior
baseline was preferred in all four level-matched cells. The working tree is
reverted to that baseline; the candidate measurements remain historical only.

---

## Classification and approval gates (unchanged)

| Gate | Status |
|------|--------|
| ADR-V1-17 fidelity classification | **`original-inspired`** — unchanged by this catalogue |
| Hardware capture / comparison grids | **`human_needed`** |
| Level-matched blind listening verdict (Niko) | **`human_needed`** |
| Stronger fidelity wording review | **`human_needed`** |

Public copy must remain generic. Do not claim exact emulation, hardware matching, or approved fidelity unless superseded by a new evidence-backed ADR-V1-17 closeout.

**Evidence boundary:** Only user-created captures of lawfully owned hardware may enter a formal capture package per [`reference-capture-protocol.md`](reference-capture-protocol.md). Third-party YouTube streams are **representative listening aids** only. **No sample-aligned public dry/wet stems were found.** Do not describe adjacent bypass passages, repeated reamps, or looped before/after segments as exact pairs.

Public streamed audio is lossy, may be loudness-normalized or edited, and contains undocumented source/amp/cab/mic/room variables. It is **qualitative listening evidence**, not a calibrated measurement source. Do not download public media into the repository, redistribute extracted audio/video, or use third-party streams as tracked golden fixtures.

---

## How to use this catalogue

1. **Discovery** — pick a source and time-coded segment for the behaviour you are checking (onset, dark predelay, distortion, gate, Igor send).
2. **Pair status** — respect the classifications below; never level-match or A/B against SendBloom using lossy stream audio as ground truth.
3. **Confounds** — note amp/cab/mic/room, other pedals, and missing knob callouts before drawing DSP conclusions.
4. **Confidence** — treat **high-confidence behaviour targets** as alignment goals consistent with official docs; treat **medium/low** items as perceptual hypotheses until hardware capture or scorecard verdict.

Streams are lossy, may include post-processing, and often omit exact settings. They support coarse perceptual ranking and musician-facing sanity checks, not RT60/filter/transfer-curve calibration or a hardware fidelity claim.

---

## Evidence hierarchy and pair-status vocabulary

The sources answer different questions. Higher placement here does not make a source calibrated audio.

1. **Manufacturer product page and manual** — product/control truth: routing, control intent, maximum stated decay, and mode semantics. These are descriptive sources, not listening measurements.
2. **Spin Semiconductor primary material** — public architecture-class constraints and recommended reverb structures. It does not disclose the Reverb-X program or prove a product-specific implementation.
3. **Audible public demonstrations** — qualitative character and interaction evidence. Knobs is embedded on the manufacturer product page; independent studio/reviewer demos add useful corroboration and occasional controlled changes.
4. **Retailer indexes and owner reviews** — discovery or low-confidence corroboration only. Where they conflict with manufacturer documentation, manufacturer documentation wins.
5. **SendBloom renders and regression metrics** — measurements of SendBloom only. They show implementation direction and regression safety, never distance from Reverb-X hardware.

| Pair status | Meaning | Sources found |
|-------------|---------|---------------|
| `sample-aligned dry/wet pair` | Same recorded input, aligned dry and processed stems, stable settings | **None found publicly** |
| `adjacent bypass pseudo-pair` | Same performer and chain around a visible bypass transition, but phrases are not identical or aligned | Get Offset 3:23–3:45 |
| `controlled repeated-source wet variation` | Recorded source is reamped/repeated while effect settings change, but no labelled dry stem is supplied | TUNNEL acoustic and drum sections |
| `candidate bypass pseudo-pair` | Wet/bypass states are visible, but editing or source continuity prevents a strong pair claim | The Pedal Collaborative 2:52–3:09 |
| `representative wet` | Audible processed examples without a usable dry counterpart | Knobs, Do Noise, Eric Merrow, collector//emitter |
| `no pair` | Descriptive source, commentary, index, or settings vary without a defensible comparison | Official docs, owner review, Pedals and Effects |

---

## Official primary sources

| Source | URL | Role | Pair status | Confidence |
|--------|-----|------|-------------|------------|
| Rainger FX product page | https://www.raingerfx.com/shop/p/reverb-x | Stated behaviour: 5–6 s max decay; bright/immediate vs dark/pre-delay; wet-only blendable distortion; post hard gate; dry unaffected; Igor send with trails | no pair | **high** (manufacturer copy) |
| Reverb-X user manual (PDF) | https://static1.squarespace.com/static/636e500201d1fa72da31bfd4/t/637f32dffe0a2325cc77c9da/1669280480630/Reverb-X%2BManual.pdf | Control definitions, true bypass, routing, and stated 5–6 s maximum | no pair | **high** (manufacturer manual) |
| Premier Guitar launch announcement | https://www.premierguitar.com/rainger-fx-introduces-the-reverb-x | Contemporary product announcement naming the FV-1 and post-reverb overdrive | no pair | **medium** (trade press / supplied launch copy, not a datasheet) |
| Spin FV-1 product/datasheet | https://www.spinsemi.com/products.html and https://www.spinsemi.com/Products/datasheets/spn1001/FV-1.pdf | Public chip-class limits: 32,768 Hz standard operation, one second total delay RAM, converter bandwidth, coefficients/instructions | no pair | **high** for FV-1; **not product-specific proof** |
| Spin effects knowledge base | https://www.spinsemi.com/knowledge_base/effects.html | Vendor-published allpass-ring, damping, modulation, gate, and distortion design guidance | no pair | **high** for architecture guidance; **not Reverb-X firmware evidence** |
| RockBoard PedalPedia Reverb-X index | https://www.rockboard.de/en/pedalPedia/Rainger-FX/Reverb-X/68983651/ | Public secondary attribution that Reverb-X uses an FV-1 | no pair | **low–medium** (retailer index, not manufacturer confirmation) |

The located Rainger FX page and manual do not name the DSP chip. The public FV-1 product attribution in contemporary launch coverage and retailer indexes is therefore secondary evidence. The clean-room architecture rationale uses Spin's own published material and remains documented in [`fv1-reverb-architecture.md`](fv1-reverb-architecture.md); it does not assert access to Reverb-X firmware.

**High-confidence behaviour targets (from official sources only):**

- Total ambience decay reaches **up to 5–6 seconds** maximum.
- **Bright:** immediate onset; **Dark:** darker wash with audible **pre-delay**.
- **Distortion** affects the **wet path only**, blendable; dry path integrity preserved.
- With the Gate button out, a low-threshold gate cleans the input before the wet processing; with it in, a **post**-reverb gate closes hard for an edited-sample character.
- **Igor** momentary send: releasing control stops new excitation while **preserving** existing tank tail.

The official manual says minimum Level produces no reverb. A Get Offset reviewer reports slight residual reverb at minimum on their sample; the official zero-wet definition remains the product-truth target unless deliberate hardware leakage is separately approved.

---

## Third-party video catalogue

Segments are **starting points** for listening. Timestamps below were checked against the public streams retrieved on 2026-07-30; platform edits can still move them.

### Get Offset — *Rainger FX Reverb-X Demo and Review*

| Field | Value |
|-------|-------|
| URL | https://www.youtube.com/watch?v=fT_9XnoZ4I0; transcript/article: https://getoffsetpodcast.com/rainger-fx-reverb-x/ |
| Content role | Strongest located adjacent true-bypass comparison plus live control/mode sweeps |
| Pair status | **adjacent bypass pseudo-pair** — same chain/player, but phrases are neither identical nor sample-aligned |
| Known chain | Squier Paranormal Offset Tele, middle position; Strymon Iridium Round B |
| Overall confidence | **medium** for audible relative behaviour; **low** for numeric calibration |

| Exact useful time | Segment / visible state | Listening focus | Confounds |
|-------------------|-------------------------|-----------------|-----------|
| wet until ~3:22.0; **dry/bypassed 3:23.0–3:30.25; wet 3:31.0–3:44.8** | Footswitch/LED transition; LVL, SIZE, and DISTN visually at minimum | Best public dry-versus-wet state change found | Adjacent live phrases, not a repeated source; low-effect wet state |
| 4:43.8–5:04.1 | Level sweep ending at maximum | Wet amount/taper | Live notes and unknown stream normalization |
| 5:12.4–5:57.7 | Size minimum→maximum; Level returned to noon; Distn minimum | Decay-span progression | Not an impulse or aligned replay |
| 6:35.6–7:31.6 | Distn progressively increased; Size maximum; Input had been reduced | Wet-drive onset and increasing grit | Reduced Input changes both drive and gate interaction |
| **Dark 11:20.2–11:26.6; Bright 11:31.4–11:46.6; Dark again ~11:54.8–11:56.7** | Adjacent mode toggles | Delayed/damped Dark versus immediate/open Bright | Different live phrases |
| 12:10.4–14:42.3 | Gate after wet processing | Hard chop, dynamics-linked bursts, reverse-like impression | Long musical passage; settings change within section |
| 15:42.6–16:22.0 | Gate returned before wet processing | Tail allowed to continue versus post-gate chop | Live performance |
| 16:39.8–17:40.7 | Igor with pre-gate/trails behaviour | New send stops on release while tail survives | Pressure and phrases vary |
| 17:49.4–17:57.7 | Igor with post-gate | Harder stop on release | Short example |

At 4:03–4:07 the reviewer reports a little reverb with Level fully down; this is an observed unit/sample comment, not a replacement for the official manual's “no reverb” endpoint. Spoken routing commentary around 7:31–8:15 is self-correcting and ambiguous; use the official wet-only post-reverb distortion description for topology truth.

### TUNNEL OF REVERB — *Reverb X \| Rainger FX - Deep Dive In The Studio*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=iagH2FIFI6A |
| Content role | Best controlled repeated-source wet walkthrough; acoustic guitar and drums are routed through a reamp setup |
| Pair status | **controlled repeated-source wet variation** — no labelled dry/bypass stem and not level-certified |
| Overall confidence | **medium** for control semantics; **low** for numeric calibration |

| Exact useful time | Segment | Listening focus | Confounds |
|-------------------|---------|-----------------|-----------|
| 1:44–2:10 | Level | Wet/dry blend feel | Spoken/live demo; no aligned dry stem |
| 2:11–2:49 | Size | Decay length / room size | Same |
| 2:50–3:07 | Distortion | Wet-path dirt | Same |
| 3:08–3:44 | Input | Drive/gate sensitivity interaction | Same |
| 3:45–4:36 | Output | Master level/headroom | Same |
| 4:37–7:20 | Reamped acoustic guitar | Controlled wet variations, clean through increasingly driven texture | AEA R84/room/reamp chain; settings change; no labelled dry clip |
| 7:21–9:46 | Dark switch context | Darker, delayed onset versus brighter/immediate mode | **ADG-1 modulated delay also in chain**; not isolated Reverb-X evidence |
| 9:47–11:09 | Reamped drums | Crunchy drum-reverb texture on a repeatable source | Mic/room/reamp chain; no labelled dry clip |
| 11:10–12:53 | Gate on synth | Dynamics-linked chop | Synth source |
| 12:54–15:12 | Igor on glitch drums | Send/release behaviour on rhythmic material | Performance layering |

### Knobs — *Rainger FX - Reverb-X*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=QrPpxCW4EzI |
| Content role | Manufacturer-page-embedded signature demo: overview, trails, gated mode, Igor, non-guitar source |
| Pair status | **representative wet** (not exact dry/wet pairs) |
| Overall confidence | **medium–high** for intended character/behaviour; **low** for numeric calibration |

| Exact useful time | Segment | Listening focus | Confounds |
|-------------------|---------|-----------------|-----------|
| 0:27–1:52 | Overview | General long, dirty ambience character | Danelectro guitar/Fender Blues Jr; edited performance |
| 1:53–2:52 | Trails mode | Full decay while dry notes remain playable over the tail | Settings not numerically called out |
| 2:53–4:14 | Gated mode | Wet field disappears on stop; dynamics-linked bursts and sample-like editing | Live playing |
| 4:15–4:43 | Igor | Dry without pressure; pressure sends wet; release returns dry while tail decays | Not an identical dry/wet replay |
| 4:44–5:19 | OP-1 synth/drums | Reverse-like gate character, selective snare send, short-decay effects | Source-dependent |

The main-demo knob positions are visually estimated around LVL 4:30, SIZE 5:30/maximum, DISTN 4:30, INPT 5:00, and OUT 7:30. The markings are tiny and the video is edited; record these as approximate visual context, not reproducible calibrated settings. The description points to standalone audio on Patreon, so it is not a public downloadable reference asset.

### Do Noise — *Rainger FX Reverb-X (Gated reverb with distortion)*

| Field | Value |
|-------|-------|
| URL | https://www.youtube.com/watch?v=4Lr4aX38ajQ |
| Content role | Clear control/mode explanations followed by interaction examples |
| Pair status | **representative wet** |
| Overall confidence | **medium** behaviour corroboration; pedal was supplied by Rainger FX and affiliate links are disclosed |

| Exact useful time | Segment | Listening focus | Confounds |
|-------------------|---------|-----------------|-----------|
| 1:08–3:00 | Knob tweaking | Level, Size, Distn, Input, and Out interactions | Live material; multiple controls move |
| 3:01–3:42 | Post-gated reverb | Threshold-linked wet cutoff | No aligned dry |
| 3:43–4:10 | Dark switch | Bright/out versus dark/in | Live phrases |
| 4:11–5:25 | Igor | Dry-until-pressed send and surviving trails | Live phrases |
| 5:26–7:02 | Choppy tremolo interaction | Gate response to amplitude modulation | Additional pedal/effect context |
| 7:03–7:24 | Glitchy low-input behaviour | Gate/drive instability as a musical effect | Application example, not calibration |
| 7:25–8:14 | Fuzz interaction | Dense source into wet path | Additional fuzz |
| 8:15–9:46 | Lo-fi/Shallow Water interaction | Texture in a larger chain | Additional effect; low isolation |

### Eric Merrow — *Rainger FX Reverb X (NAMM 2018 Pedal Demo Marathon)*

| Field | Value |
|-------|-------|
| URL | https://www.youtube.com/watch?v=E6ScqDpbLYE; series context: https://delicious-audio.com/best-new-pedals-2018-at-namm/ |
| Content role | Short produced-style musical survey captured in the NAMM environment |
| Pair status | **representative wet** |
| Overall confidence | **low–medium** character corroboration; chain/settings and show-floor conditions are undocumented |

| Exact useful time | Segment | Listening focus | Confounds |
|-------------------|---------|-----------------|-----------|
| ~0:20–1:35 | Control/mode sweeps | Broad wet texture and range | Music-only, live NAMM capture, settings not labelled |
| ~1:40–3:35 | Igor/gate/dark changes | Momentary and gated musical gestures | Same; visual inspection bounds rather than authored chapters |

### The Pedal Collaborative — *FIRST SOUNDS // Rainger FX Reverb-X*

| Field | Value |
|-------|-------|
| URL | https://www.youtube.com/watch?v=HAWxtPG0Q-o |
| Content role | Secondary top-down demo with visible bypass states and Igor section |
| Pair status | **candidate bypass pseudo-pair**, not a verified repeated-source pair |
| Overall confidence | **low** for comparison; **medium** as representative wet listening |

| Exact useful time | Segment | Listening focus | Confounds |
|-------------------|---------|-----------------|-----------|
| dry/LED off ~2:52–2:55; wet/on ~2:56–3:02; dry/off ~3:03–3:08; wet/on after ~3:09 | Visible bypass transitions | Candidate adjacent dry/wet state changes | Video edits and source continuity prevent a sample-pair claim |
| ~5:50–7:15 | Igor section | Momentary send gestures | Unlabelled settings |

### collector//emitter — *Sound Study // Rainger FX - Reverb X*

| Field | Value |
|-------|--------|
| URL | https://www.youtube.com/watch?v=6IeXMyhjhlI |
| Content role | Performance-oriented **wet** examples |
| Pair status | **representative wet** |
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

### Audiofanzine — owner review

| Field | Value |
|-------|-------|
| URL | https://en.audiofanzine.com/guitar-reverb/rainger-fx/reverb-x/user_reviews/ |
| Content role | Subjective owner vocabulary only: short Size described as a snappy/drum-room or slap-like effect, maximum as cavernous; distortion described as increasingly sizzling grit |
| Pair status | **no pair** |
| Overall confidence | **low** — single owner, no controlled audio or measurements |

The review is useful only as corroborating language. Its statement that the gate can cut guitar and reverb conflicts with the manufacturer's dry-unaffected routing description and is not adopted as product truth.

---

## Behaviour targets for DSP alignment (actionable, product-truth preserving)

These targets express **allowed clean-room goals**. They do not constitute proof that SendBloom matches hardware.

### Reference-derived behaviour

These are externally supported product behaviours. Only the stated maximum decay is numeric.

| Target | Evidence-supported acceptance direction | Evidence class |
|--------|-----------------------------------------|----------------|
| Maximum decay | Size reaches approximately **5–6 s** at maximum | Manufacturer page/manual |
| Size range | Low Size is much shorter/snappier than maximum; do not infer a numeric minimum from streams | Official control description plus low-confidence owner/demo corroboration |
| Bright onset | Bright is immediate/open relative to Dark | Manufacturer page/manual; repeated audible corroboration |
| Dark onset/spectrum | Dark adds audible pre-delay and a darker wash; it is not merely a renamed decay setting | Manufacturer page/manual; Get Offset/Knobs/TUNNEL qualitative evidence |
| Routing | Clean dry remains present and unaffected; reverb Level controls wet amount | Manufacturer page/manual |
| Distortion | Clean-to-distorted blend is applied after reverb to the wet path; dry remains undistorted | Manufacturer page/manual |
| Input interaction | Input changes overload/drive and gate sensitivity | Manufacturer page/manual; demos |
| Pre-gate | Button out provides low-threshold cleanup before wet processing and allows generated tails to decay | Manufacturer page/manual |
| Post-gate | Button in closes the processed wet path abruptly on silence for an edited/sample-like effect | Manufacturer page/manual; Knobs/Get Offset/Do Noise |
| Igor/send | With Igor connected, no pressure gives dry-only input; pressure sends into wet; release stops new excitation while the existing tail survives in pre-gate mode | Manufacturer page/manual; Knobs/Get Offset |

### Public architecture-derived clean-room constraints

These come from Spin Semiconductor material, not from measuring Reverb-X. The public product-specific FV-1 attribution is secondary, so these are architecture-class implementation guidance rather than proof of the hardware algorithm.

| Constraint | Clean-room direction | Source |
|------------|----------------------|--------|
| Fixed resource envelope | Design within FV-1's 32,768 Hz standard operation and one second total delay RAM | FV-1 datasheet |
| Dense reverb structure | Multiple delays and allpasses in a recirculating loop; input diffusion and internal taps are vendor-documented options | Spin effects knowledge base |
| Flutter control | At least 200 ms total plain loop delay; avoid fixed 4–8 Hz tail repetition | Spin effects knowledge base |
| Spectral decay | In-loop high/low shelving is an appropriate way to shape frequency-dependent decay | Spin effects knowledge base |
| Motion | Slow SIN/COS modulation of selected delays can reduce static ringing/flutter | Spin effects knowledge base |
| Drive shape | A straight low-level region with nonlinear breakup on larger transients follows Spin's published distortion guidance | Spin effects knowledge base |

### Provisional SendBloom engineering windows — not reference measurements

These numbers are implementation/tuning choices. They are useful regression targets, but public audio did not measure or validate them against hardware.

| Parameter | Provisional window/current choice | Status boundary |
|-----------|-----------------------------------|-----------------|
| Size mapping | Current **1.2–6.0 s** exponential mapping | 6 s endpoint is reference-derived; 1.2 s floor and curve are architecture/engineering choices |
| Bright onset | No intentional predelay; practical rendered onset currently **6.409 ms** | SendBloom measurement only |
| Dark predelay | A **30–80 ms** listening/tuning window is reasonable; current crossfade adds **55 ms**, rendered onset **61.462 ms** | Window and current value are provisional engineering choices, not public-reference measurements |
| Dark HF damping | Faster HF decay than Bright while retaining audible upper-mid texture | Filter corners/depth are SendBloom choices; streams cannot calibrate them |
| Post-gate close | Roughly **5–20 ms** is a provisional hard-close tuning window | Derived from “hard/sudden” behaviour, not timed from lossy video |
| Igor/send smoothing | Roughly **2–10 ms** smoothing to avoid zipper/clicks while feeling immediate | Real-time engineering choice, not a hardware measurement |
| Early density | Lower crest/kurtosis and no obvious short-loop flutter versus the prior build | SendBloom regression direction only |

### Medium / low-confidence (perceptual, from demos — not metrics)

| Observation | Confidence | Notes |
|-------------|------------|-------|
| Gated synth has a sample-like chop while the pre-gate mode retains a wash | medium | TUNNEL 11:10; verify with owned hardware capture |
| Igor on rhythmic material leaves ghost tails after release | medium | TUNNEL 12:54, Knobs 4:15, Get Offset 16:39.8 |
| Dry attacks remain articulate under a large wet tail | medium | Knobs trails section; live chain confound |
| Increasing drive progresses from clean wet to sizzling/crunchy texture without intentionally dirtying dry | medium | Official routing plus Get Offset/TUNNEL/Do Noise listening |
| Acoustic guitar remains articulate at moderate Size | low | TUNNEL 4:37; mic/reamp confound |
| Drum bloom appears dense without an obvious fixed short-loop flutter | low | Multiple demos; no isolated wet or impulse |

**Prohibited inference:** Do not cite public video loudness, spectrum, predelay, gate time, distortion transfer, or decay numbers as calibrated measurements. Do not use `/tmp` or other downloaded third-party audio as tracked project assets, redistribute it, or claim that the catalogue demonstrates perceptual or measurable closeness.

---

## Engine candidate and measured status

An offline candidate used two serial allpass filters per ring block (instead of one)
within the **unchanged** 32,768-word FV-1 delay RAM budget and **unchanged total
loop delay**, consistent with Spin’s documented “2 allpass + delay” block pattern.
Its wet-only reciprocal-dirt candidate also moved the knees into the measured
bloom range with per-branch normalization. Neither change is retained in the
current working-tree engine after the interactive listening result.

**Measured direction:** Bright 100–250 ms crest fell **7.330 → 6.804** and kurtosis **11.575 → 6.063**; Dark kurtosis fell **16.573 → 9.341**. Bright/Dark onset remained **6.409 / 61.462 ms**, total 0–4 s RMS moved by less than **0.05 dB**, and the unchanged ProperSRC HF ceiling passed at **1.415 < 1.500**. See [`fv1-reverb-architecture.md`](fv1-reverb-architecture.md).

For wet dirt, the rejected candidate changed the prior +1.32/−1.20 quiet slopes to **+1.20/−1.20**, moved reciprocal knees from **+0.303/−0.333** to **+0.182/−0.200**, and reduced deterministic guitar-pluck dirty/clean tail RMS from **1.173 to 1.111**. This was earlier, more bounded breakup rather than a level boost; it is retained as historical evidence only. See the dated [engine evidence report](reports/sendbloom-reverb-x-public-reference-engine-evidence.md) and [machine-readable receipt](reports/data/sendbloom-engine-metrics-2026-07-30.json).

**Acceptance boundary:**

1. Directional early-window density screen: **passed** against the prior built binary.
2. Automated topology, predelay, dirt, dry-null, HF, finite-output, real-time, and regression suite: see the dated evidence report; these gates do not prove hardware fidelity.
3. A deterministic randomized A/B pack compares the density/dirt engine changes using owned stimuli only; it does not contain or replace the public references and does not satisfy the full gate/send/dry hardware scorecard below.
4. **Human_needed:** Niko level-matched listening approval per scorecard below.

Status: **candidate objectively regression-tested but rejected by interactive listening; hardware comparison remains `human_needed`**.

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
| 2026-07-30 | public-reference-catalog-verified | Added verified retrieval date, evidence hierarchy, pair classifications, exact useful timestamps, source/conflict notes, and separated external behaviour from provisional engineering windows; fidelity classification unchanged |
