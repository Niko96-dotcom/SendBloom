# SendBloom

## What This Is

SendBloom is a gated dirty-ambience guitar effect plugin (AU / VST3, JUCE 8) from Niko Audio Labs. It keeps dry guitar clean while a wet reverb path blooms, can be pressure-sent, distorted wet-only, and hard-chopped by a post gate — an original software recreation of publicly described pedal behavior, not a firmware dump or circuit clone.

## Core Value

Pressure Mode, MIDI, and host automation must tell the truth: connected-and-resting is dry with decaying tails; releasing pressure never flips back to always-on wet feed; realtime processing never lies about blocks, bypass, or allocation.

## Current Milestone: v1.0 Interaction Truth, Realtime Safety & Release Candidate

**Goal:** Ship an honest, stable `v1.0.0-rc0` by fixing false interaction/realtime contracts, shipping original branding, and closing automated + human release gates.

**Authoritative source:** `sendbloom_v1_interaction_truth_release_master_milestone.md` (imported 2026-07-12). Phase range **19–27**.

**Target features:**
- Baseline contracts & failing harness before production fixes (Phase 19)
- Pressure-send state truth + UI/preset resting semantics (Phase 20)
- Realtime span engine & channel-preserving true bypass (Phase 21)
- MIDI CC1 as realtime modulation + per-sample control delivery (Phase 22)
- Input / Level / Gate display and DSP truth (Phase 23)
- Predelay continuity, time-invariant LFO depth, SRC underfill, wet-dirt HP/DC (Phase 24)
- Presets, original SendBloom branding, strengthened clean-room scanner (Phase 25)
- Reference-capture tooling and honest fidelity claim status (Phase 26)
- RC verification, licensing, signing/notarization, DAW smoke (Phase 27)

## Requirements

### Validated

Capabilities already present in the repository that must not regress unless a v1 requirement explicitly updates the contract:

- ✓ JUCE 8 AU / VST3 builds with Catch2 + pluginval strictness 10 CI — pre-v1 foundation
- ✓ Parallel dry/wet topology; dry tap before wet input gain — pre-v1 foundation
- ✓ Wet-only distortion via `WetOverdriveState` — pre-v1 foundation
- ✓ PreSoft and PostHard gate placement — pre-v1 foundation
- ✓ `IReverbEngine` + host-rate tank + optional ProperSRC 32,768 Hz path — ProperSRC program
- ✓ Zero reported PDC with measured wet-only delay (ADR-003 Path B) — ProperSRC program
- ✓ Engine equal-power 35 ms wet crossfade — ProperSRC program
- ✓ `authentic_color` off by default on fresh load and presets — ProperSRC program
- ✓ Eight factory presets with state round-trip tests — pre-v1 foundation
- ✓ Immutable parameter IDs; manufacturer/plugin codes `NkMo` / `SbLm`; bundle `com.nikoaudiolabs.sendbloom` — locked product identity

### Active

- [ ] Pressure Mode dry-at-rest with tail preservation (UI, host, MIDI) — SEND / MIDI
- [ ] No-allocation span processing for all host block sizes — RT
- [ ] True channel-preserving unity bypass — CORE
- [ ] Canonical Input −9/0/+9, wet-only Level, PostHard de-click — CORE
- [ ] Predelay / mod / SRC / wet-dirt integrity — DSP
- [ ] Original branding + meaningful clean-room scan — UX
- [ ] Honest fidelity classification — REF
- [ ] `v1.0.0-rc0` automated + human release gates — REL

### Out of Scope

- New reverb topology / FDN as production default — v1 locks current Schroeder tank
- Changing ProperSRC quality preset — evidence-only later
- Making 32k Color default-on — product policy unchanged
- Implementing Extended Stereo or Dirt OS — deferred; params may remain disabled
- Oversampling as a new feature — deferred with Dirt OS
- CLAP / AAX — not this release
- New preset-browser architecture, cloud licensing, storefront, telemetry — post-v1
- Visual redesign beyond required original branding / state truth — branding only
- Renaming immutable parameter IDs or changing plugin codes / bundle ID — locked
- Claiming component-level circuit emulation or exact hardware fidelity without Phase 26 evidence

## Context

Brownfield import: live repo had no `.planning/` history. Prior ProperSRC work is treated as completed Phases 11–18 conceptually; this milestone continues at Phase 19. Current `VERSION` is `0.0.1`. Confirmed defects include backwards pressure-pad release, MIDI APVTS mutation from `processBlock`, oversized-block dry fallback, fake per-sample smoothing, PostHard one-sample snap, Input display/DSP mismatch, non-true bypass, conditionally clocked dark predelay, rate-dependent LFO depth, ProperSRC underfill, incomplete wet-dirt filters, third-party naming in shipping surfaces, and incomplete release gates.

Master ADRs ADR-V1-01 … ADR-V1-17 are locked for this milestone (pressure state, MIDI purity, span quantum 128, Input curve, true bypass, PostHard 0.75 ms ramp, continuous 55 ms dark tap, time-invariant mod, SRC clear, wet-dirt filters, numeric `VERSION` 1.0.0, fidelity classification).

## Constraints

- **Tech stack**: C++20, JUCE 8, CMake, Catch2 — existing plugin codebase
- **Realtime**: No heap allocation, host notification, locks, logging, or file I/O in `processBlock`
- **Identity**: Parameter IDs, `NkMo`/`SbLm`, bundle ID immutable
- **Claims**: Public fidelity wording must match Phase 26 `CLAIM_STATUS.md`
- **Human gates**: DAW smoke, licensing choice, branding approval, and listening remain `human_needed`
- **Legal**: Enforce repository clean-room policy; no third-party firmware/dumps/schematics

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Phase numbering continues at 19–27 | Master milestone contract; ProperSRC occupied 11–18 | — Pending |
| MIDI pressure is realtime-only (`max` with host/UI) | Avoid APVTS/host notify from audio thread | — Pending |
| Span quantum `kControlQuantum = 128` | Oversized blocks + MIDI positions + bounded control rate | — Pending |
| Canonical Input = −9 + 18×smoothstep(norm) | Unify display/DSP; clockwise increases drive | — Pending |
| True bypass is final per-channel crossfade | Settled bypass ignores Output and mono collapse | — Pending |
| Default fidelity status `original-inspired` | No hardware evidence required to ship correctly positioned v1 | — Pending |
| Extended Stereo / Dirt OS stay disabled | Explicit v1 deferral | — Pending |

## Evolution

This document evolves at phase transitions and milestone boundaries.

**After each phase transition** (via `/gsd-transition`):
1. Requirements invalidated? → Move to Out of Scope with reason
2. Requirements validated? → Move to Validated with phase reference
3. New requirements emerged? → Add to Active
4. Decisions to log? → Add to Key Decisions
5. "What This Is" still accurate? → Update if drifted

**After each milestone** (via `/gsd-complete-milestone`):
1. Full review of all sections
2. Core Value check — still the right priority?
3. Audit Out of Scope — reasons still valid?
4. Update Context with current state

---
*Last updated: 2026-07-12 after starting milestone v1.0 Interaction Truth*
