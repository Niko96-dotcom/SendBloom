# Research Addendum — User Dossier (DE + EN)

**Ingested:** 2026-07-06  
**Sources:** Plugin-Umsetzungs-Dossier (DE), Complete Technical Deep Research (EN)

---

## What was already covered

Our 8-lane GitHub research + `BUILD_MICROSTEPS` already aligned on:

- Topology over algorithm family (routing = product identity)
- FV-1 as hardware hypothesis; no public schematic
- Parallel dry + wet-only OD + dual gate + Igor send
- Split INPUT_GAIN / INPUT_THRESHOLD
- No Rainger patents found
- DV-1 as closed commercial reference
- CloudSeedCore / FDN for plugin engine (not EEPROM clone)

**~70% of both dossiers confirms existing plan.** The value is in specificity, legal guardrails, and a few course corrections below.

---

## Net-new findings (actionable)

### 1. Reverb engine — FV-1 Schroeder topology detail

Research 2 documents the likely FV-1 program structure:

- 4× series allpass diffusion → 4× parallel comb (feedback ~0.5–0.98) → modulated tank allpass
- **32,768 Hz**, 24-bit fixed-point, **1 s delay RAM** cap
- 9-bit POT quantization + hysteresis on hardware knobs

**Impact:** ADR-002 FDN path stays valid for *musical* tails, but **pedal lo-fi character** may need an optional **Authentic coloration** layer (32 kHz decimate/reconstruct or Schroeder compact tank A/B). Not required for MVP; add as Phase 8 `MB-085`.

**Dattorro** elevated in DE dossier as “best price/performance” — worth Faust spike (`reverb_trickery` already uses `dattorro_rev`) before locking FDN-only.

### 2. Post-gate behavior — sharper than our MB-022

Manual quotes: post-gate closing is **not user-adjustable** — brutal “edited sample” chop, not a musical release knob.

**Impact:** `MB-022` post profile should target **near-instant close** (≤5–10 ms) with optional **Authentic** lock (no release param exposed). Pre-gate keeps soft 150 ms hum kill.

### 3. Distortion — fixed grind, blend only

`DISTN` = crossfade clean wet ↔ fully distorted wet. **No drive knob** on hardware.

**Impact:** Confirms `MB-025`/`MB-026` — single asymmetric clipper at fixed gain; `distn` is wet-path blend only. OD oversampling **only on dirt branch** (DE dossier + JUCE docs) — `MB-082` placement correct.

### 4. Igor electrical + curve

- TRS 3.5 mm, **sleeve unconnected**, ~**1 MΩ → tens of Ω** under pressure
- Non-linear pressure macro (not linear expression pedal)

**Impact:** `PressureSendPad` mapping: exponential/log curve + dual sensitivity toggle (foot vs hand). Already sketched in RESEARCH_CORPUS; now has component-level numbers.

### 5. Authentic vs Extended product modes (NEW)

DE dossier recommends two visible modes:

| Mode | Scope |
|------|--------|
| **Authentic** | Mono I/O default, hardware param set, no stereo width |
| **Extended** | Optional stereo decorrelator, OD oversampling toggle, finer gate times |

**Impact:** Reframe `MB-081` (stereo) and `MB-082` (OD OS) under **Extended** flag; default export = Authentic.

### 6. Competitive landscape — new OSS entry

| Project | Notes |
|---------|--------|
| [claytonyen/gated-reverb-distortion](https://github.com/claytonyen/gated-reverb-distortion) | MIT; hardware Reverb-X-inspired; **PT2399** not FV-1; early stage |
| **Northern Valley DV-1** | Commercial; ~2 s decay, bright, wet OD + threshold gate — **primary A/B target** if no hardware |

### 7. Legal guardrails (strengthen)

DE dossier explicit warnings we should adopt:

- **Do not** read Reverb-X EEPROM / FV-1 bytecode (DMCA / EU decompilation limits)
- **Do not** use Rainger / Reverb-X / Igor in product name or marketing
- General reverb patents exist (e.g. Neunaber US8204240B2, active to 2031) — FTO before wide release
- Position as **“inspired by gated dirty ambience”**, independent implementation

**Impact:** Add `ADR-003-legal-boundaries.md` before commercial ship (not blocking MB-002).

### 8. Validation methodology (NEW detail)

Re-amp pipeline: DI → hardware sweeps → ESS deconvolution → EDC/RT60 + crest/gate-edge metrics → then ear-tuning.

Subjective: **MUSHRA** (engine panel) + **ABX** (musician panel) — guitarists judge “plays like the pedal,” not RT60 accuracy.

**Impact:** Expand `MB-075` hardware A/B protocol with ESS + feature vectors; optional `MB-086` MUSHRA script.

### 9. Feature ship order (confirms critical path)

DE priority list matches our dependency graph:

1. Wet-only OD after verb  
2. Gate pre/post toggle  
3. Bright/Dark + predelay  
4. Igor send  
5. Mono authentic  
6. OD oversampling  
7. Extended stereo / extras  

### 10. Product history clarifications

- **No documented Mk II / V2** — single public hardware generation
- **Pull Focus** (2024): analog OD from Reverb-X lineage + FV-1 digital verb/chorus — not 1:1 Reverb-X successor
- **Echo-X**: PT2399 delay sibling, 29 mA vs Reverb-X 76 mA

---

## What does NOT change

| Topic | Still holds |
|-------|-------------|
| Scaffold | pamplejuce template |
| No EEPROM / firmware extraction | Independent DSP |
| Gate implementation base | BYOD FSM + LSP/airwindows concepts |
| Send UI | Greenfield PressureSendPad |
| OSS moat | No ≥7/10 full-chain clone |

---

## Suggested micro-build additions

| ID | Goal |
|----|------|
| MB-085 | Optional FV-1 coloration (32 kHz path or Schroeder tank A/B) |
| MB-086 | Re-amp + ESS capture protocol (`MB-075` superset) |
| MB-087 | Preset bank: Sparkle Verb, Cut Sample Gate, Spacerock Burn, Dry Dub Sends |

---

## Open question surfaced

**How much does analog post-drive PCB matter vs FV-1 core?** Dossier argues analog OD frame may be significant (Pull Focus cites “all-analog distortion from Reverb-X”). Only answerable via hardware re-amp A/B — prioritize if unit available.
