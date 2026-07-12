# Roadmap: SendBloom

## Overview

Milestone **v1.0 — Interaction Truth, Realtime Safety & Release Candidate** turns the existing SendBloom AU/VST3 codebase into an honest, stable `v1.0.0-rc0`. Prior ProperSRC work is treated as conceptual Phases 11–18; this roadmap continues at **Phase 19** through **Phase 27**. Work freezes baseline truth and failing harnesses first, then corrects pressure-send semantics, realtime span processing and true bypass, MIDI/per-sample control delivery, Input/Level/Gate truth, reverb/dirt integrity, branding and release docs, reference-claim classification, and finally RC verification with licensing and distribution gates.

**Locked ADRs:** ADR-V1-01 … ADR-V1-17 (pressure state, MIDI purity, span quantum 128, Input curve, true bypass, PostHard ramp, continuous dark tap, time-invariant mod, SRC clear, wet-dirt filters, numeric `VERSION` 1.0.0, fidelity classification).

**Release gate rule:** Phase 27 must not begin while any automated requirement from Phases 19–25 is red. Human gates remain marked `human_needed` and must never be silently passed.

## Milestones

- ✅ **Pre-GSD foundation (conceptual Phases 11–18)** — ProperSRC dual-engine, wet-only dirt, gates, presets, CI — present in repo before planning import
- 🚧 **v1.0 Interaction Truth, Realtime Safety & Release Candidate** — Phases 19–27 (in progress)

## Phases

**Phase Numbering:** Continues from ProperSRC conceptual Phases 11–18. Do not renumber. Integer phases 19–27 are this milestone’s planned work.

- [x] **Phase 19: Baseline, Contracts & Failure Harness** - Freeze truth, map requirements, add failing defect tests and durable verifier (completed 2026-07-12)
- [x] **Phase 20: Pressure Send State Truth** - Dry-at-rest pressure mode with correct UI/preset resting semantics (completed 2026-07-12)
- [ ] **Phase 21: Realtime Span Engine & True Bypass** - No-alloc spans, oversized blocks, channel-preserving unity bypass
- [ ] **Phase 22: MIDI & Per-Sample Control Delivery** - CC1 realtime modulation and per-sample dynamic control consumption
- [ ] **Phase 23: Input, Level & Gate Truth** - Canonical Input −9/0/+9, wet-only Level, PostHard de-click
- [ ] **Phase 24: Reverb State & Wet-Dirt Integrity** - Continuous dark tap, time-invariant LFO, SRC clear, wet HP/DC
- [ ] **Phase 25: Presets, UI, Branding & Release Truth** - Original SendBloom branding, clean-room scan, truthful docs
- [ ] **Phase 26: Reference Capture & Sonic Classification** - Capture protocol, metrics tooling, ADR-V1-17 claim status
- [ ] **Phase 27: RC Verification, Licensing & Distribution** - `v1.0.0-rc0` automated + human release gates

## Phase Details

### Phase 19: Baseline, Contracts & Failure Harness

**Goal**: Current truth is frozen, every confirmed defect has a failing test for the right reason, and milestone traceability plus a durable verifier exist before production fixes.
**Depends on**: Nothing (first phase of this milestone; builds on conceptual Phases 11–18)
**Requirements**: BASE-01, BASE-02, BASE-03, BASE-04, BASE-05, BASE-06, BASE-07, BASE-08
**Success Criteria** (what must be TRUE):

  1. Baseline report records commit, branch, build/CI state, discovered test count, and known manual gaps before production DSP/UI changes
  2. Every confirmed defect has a contract test that fails for the intended reason (pressure release, oversized blocks, true bypass, PostHard snap, Input anchors, MIDI APVTS purity, banned shipping strings/filenames)
  3. All preexisting ProperSRC, HF, dry-integrity, and release-truth tests still pass unless a requirement explicitly updates their contract
  4. `scripts/verify-v1.sh` runs the full automated gate set and truthfully reports current red gates without hard-coded test totals
  5. Human-only gates are marked `human_needed` and never silently treated as pass

**Plans**: 3/3 plans complete
Plans:

- [x] 19-01-PLAN.md — Restore cmake submodule, freeze 19-BASELINE.md + metrics, complete requirement→artifact traceability (BASE-01/02/03/07)
- [x] 19-02-PLAN.md — Add seven failing [v1][contract] suites; prove green ProperSRC/HF/DryPath/release remain green (BASE-04)
- [x] 19-03-PLAN.md — Ship scripts/verify-v1.sh with human_needed gates and no hard-coded totals (BASE-05/06/08)

**ADRs**: ADR-V1-01…17 referenced as locked constraints for later phases

### Phase 20: Pressure Send State Truth

**Goal**: Pressure Mode tells the truth from UI and presets — connected-and-resting is dry with decaying tails; release never flips back to always-on wet feed.
**Depends on**: Phase 19
**Requirements**: SEND-01, SEND-02, SEND-03, SEND-04, SEND-05, SEND-06, SEND-07, SEND-08, SEND-09, SEND-10, SEND-11, SEND-12, SEND-13, SEND-14, UX-01, UX-02, UX-03, UX-04, UX-05
**Success Criteria** (what must be TRUE):

  1. Disconnected mode is always-on wet feed; connected-at-rest feeds no new wet input while existing tails continue; press sends; release zeros amount without disconnecting
  2. On-screen pad release keeps `send_connected=true`, sets `send_amount=0`, and pressed overlay follows pressure/pressed state — not connection alone
  3. Advanced UI exposes persistent Pressure Mode; pad press may auto-connect without disconnecting on release
  4. Factory pressure presets load connected with `send_amount=0`; always-on presets load disconnected; XML and `FactoryPresets.cpp` recall identical state; parameter IDs unchanged; default `send_amount` is 0
  5. Firm vs Soft remain distinct; attack/release are ~3 ms / ~25 ms; behavior is invariant across host block sizes

**Plans**: 3/3 plans complete
Plans:

- [x] 20-01-PLAN.md — PressureController + defaults + processor wiring (SEND-01…04/09/10/12/13, UX-01/02)
- [x] 20-02-PLAN.md — Pad release / overlay / Advanced toggle → flip [pressure-release] green (SEND-05…08)
- [x] 20-03-PLAN.md — Preset matrix + UX + SEND-14 caveat tests (SEND-11/14, UX-01…05)

**UI hint**: yes
**ADRs**: ADR-V1-01, ADR-V1-02, ADR-V1-04

### Phase 21: Realtime Span Engine & True Bypass

**Goal**: Host blocks of any size retain full wet processing without audio-thread allocation, and settled bypass is channel-preserving unity.
**Depends on**: Phase 20
**Requirements**: RT-01, RT-02, RT-03, RT-05, RT-08, RT-09, RT-10, RT-11, RT-12, RT-13, RT-14, RT-15, CORE-14, CORE-15, CORE-16, CORE-17, CORE-18
**Success Criteria** (what must be TRUE):

  1. `processBlock` performs zero heap allocations (including oversized blocks and engine-crossfade completion); dry-only oversized fallback and `dryBuffer.setSize` in process are gone
  2. A 2048-sample host block prepared at 512 matches equivalent smaller-block renders within tolerance; wet remains nonzero for oversized blocks
  3. Control-rate reverb values update in spans of at most 128 samples; `preparedMaxBlock_ <= 0` is handled safely
  4. Settled bypass preserves each input channel at unity within floating tolerance and ignores Input, Distn, Gate, Level, and Output; transitions remain click-bounded; engaged mono-first behavior unchanged
  5. Authentic-mode changes request exactly one engine target transition per parameter change; reported latency stays zero under ADR-003; crossfade begins in the first block after change, converges after rapid toggles, and resets only the idle engine with zero allocations; 10,000-block stress stays finite

**Plans**: TBD
**ADRs**: ADR-V1-05, ADR-V1-07, ADR-V1-10

### Phase 22: MIDI & Per-Sample Control Delivery

**Goal**: MIDI CC1 is sample-position-aware realtime pressure modulation (not APVTS mutation), and fast dynamic controls are consumed per sample.
**Depends on**: Phase 21
**Requirements**: MIDI-01, MIDI-02, MIDI-03, MIDI-04, MIDI-05, MIDI-06, MIDI-07, MIDI-08, MIDI-09, MIDI-10, RT-04, RT-06, RT-07
**Success Criteria** (what must be TRUE):

  1. When pressure mode is connected, CC1 modulates effective pressure without calling `setValueNotifyingHost` or writing `send_amount` APVTS state; saved plugin state stays clean
  2. CC1 sample positions and multiple events in one block are applied in order; value 0 releases MIDI pressure; host/UI and MIDI combine via `max`; non-CC1 messages do not change pressure
  3. Span boundaries respect CC1 event positions; send/distortion/threshold arrays and input/level/output/bypass remain per-sample; behavior stays finite and deterministic at block sizes 1–2048

**Plans**: TBD
**ADRs**: ADR-V1-03, ADR-V1-06

### Phase 23: Input, Level & Gate Truth

**Goal**: Input, Level, and Gate controls agree with their labels — clockwise Input drives wet harder, Level scales wet only, PostHard is brutal but de-clicked.
**Depends on**: Phase 22
**Requirements**: CORE-01, CORE-02, CORE-03, CORE-04, CORE-05, CORE-06, CORE-07, CORE-08, CORE-09, CORE-10, CORE-11, CORE-12, CORE-13
**Success Criteria** (what must be TRUE):

  1. Input maps −9/0/+9 dB at 0/0.5/1 via one canonical curve shared by display and DSP; clockwise increases wet drive and detector level; dry tap remains before Input gain
  2. Level changes wet return only; dead dry-gain fields/smoothers/tests are gone; Gate Sens remains advanced with existing ID and canonical dB display
  3. PostHard closes with a 0.75 ms ramp (not a one-sample snap), reaches zero within 1 ms, and still chops wet within 15 ms after silence; PreSoft retains long unobtrusive close

**Plans**: TBD
**UI hint**: yes
**ADRs**: ADR-V1-08, ADR-V1-09, ADR-V1-11

### Phase 24: Reverb State & Wet-Dirt Integrity

**Goal**: Predelay, modulation, ProperSRC underfill, and wet-dirt filtering are correct without retuning the reverb character.
**Depends on**: Phase 23
**Requirements**: DSP-01, DSP-02, DSP-03, DSP-04, DSP-05, DSP-06, DSP-07, DSP-08, DSP-09, DSP-10, DSP-11, DSP-12, DSP-13, DSP-14, DSP-15
**Success Criteria** (what must be TRUE):

  1. Predelay line is clocked continuously; dark mode uses a fixed 55 ms tap blended by dark mix; re-enabling Dark emits no stale frozen burst; bright/dark automation stays finite and click-bounded
  2. LFO modulation depth is invariant in seconds across sample rates; ProperSRC output is pre-cleared and unwritten samples remain zero; existing ProperSRC imaging/HF gates stay green
  3. Wet dirt implements 100 Hz pre-clip HP and 20 Hz post-clip DC blocker; long-run DC stays below gate; dry path unaffected; `dirt_os` stays disabled; `authentic_color` remains off by default and in all factory presets

**Plans**: TBD
**ADRs**: ADR-V1-12, ADR-V1-13, ADR-V1-14, ADR-V1-15

### Phase 25: Presets, UI, Branding & Release Truth

**Goal**: Shipping surfaces, scanner, presets, and docs match SendBloom’s clean-room position and truthful Pressure Mode copy.
**Depends on**: Phase 24
**Requirements**: UX-06, UX-07, UX-08, UX-09, UX-10, UX-11, UX-12, UX-13, UX-14, UX-15, UX-16
**Success Criteria** (what must be TRUE):

  1. No product-facing source string or shipping filename contains third-party product/brand/controller names; procedural fallback says `SENDBLOOM`; reference faceplate is removed unless written permission exists
  2. Niko-approved original faceplate ships, or original procedural faceplate ships (`human_needed` for asset approval); editor hotspots/overlays remain hittable and aligned
  3. Legal metadata scan normalizes punctuation/spacing/case and scans filenames; `design-qa.md` uses portable repo-relative paths with current evidence
  4. UI explains Pressure Mode without third-party controller naming; pre-v1 sessions are explicitly classified with no hidden migration promise; README and clean-room docs describe only verified behavior

**Plans**: TBD
**UI hint**: yes

### Phase 26: Reference Capture & Sonic Classification

**Goal**: Fidelity claims never exceed evidence — protocol and tooling exist, and ADR-V1-17 status is assigned honestly.
**Depends on**: Phase 25
**Requirements**: REF-01, REF-02, REF-03, REF-04, REF-05, REF-06, REF-07, REF-08, REF-09, REF-10, REF-11, REF-12
**Success Criteria** (what must be TRUE):

  1. Reproducible reference-capture protocol and measurement tooling exist for predelay, decay, spectral centroid, gate envelope, harmonics, and DC; metrics store capture metadata and knob positions
  2. No third-party firmware/EEPROM/bytecode/schematics/dumps are committed; any hardware recordings are user-created audio captures only
  3. Closeout assigns exactly one ADR-V1-17 status (`original-inspired` if hardware unavailable); public copy matches that status; hardware comparison grids and listening remain `human_needed` when hardware/listening are unavailable — never silently passed

**Plans**: TBD
**ADRs**: ADR-V1-17

### Phase 27: RC Verification, Licensing & Distribution

**Goal**: A real `v1.0.0-rc0` exists with green automated gates, documented human evidence, resolved licensing, and distributable artifacts.
**Depends on**: Phases 19–26 (must not begin while automated requirements from Phases 19–25 are red)
**Requirements**: REL-01, REL-02, REL-03, REL-04, REL-05, REL-06, REL-07, REL-08, REL-09, REL-10, REL-11, REL-12, REL-13, REL-14, REL-15, REL-16, REL-17, REL-18, REL-19, REL-20
**Success Criteria** (what must be TRUE):

  1. `VERSION` is numeric `1.0.0`; RC identity is tag `v1.0.0-rc0`; clean CMake Release build + full Catch2 suite pass; VST3 pluginval strictness 10 passes; AU validation uses discovered `auval -a` type; CI green on macOS/Windows/Linux
  2. Logic/Cubase/REAPER smoke and 10-minute soak are signed with real tester/date/result evidence (`human_needed`); no human gate is represented as automated success
  3. JUCE commercial-vs-GPL decision is documented and approved by Niko (`human_needed`); repo/distribution license matches; macOS public binaries are Developer ID signed and notarized/stapled when credentials available (`human_needed`); artifacts have SHA-256 checksums; working tree is clean at tag; release checklist/report is complete

**Plans**: TBD
**ADRs**: ADR-V1-16

## Coverage Map

| Requirement | Phase |
|-------------|-------|
| BASE-01 … BASE-08 | 19 |
| SEND-01 … SEND-14 | 20 |
| UX-01 … UX-05 | 20 |
| RT-01, RT-02, RT-03, RT-05, RT-08 … RT-15 | 21 |
| CORE-14 … CORE-18 | 21 |
| MIDI-01 … MIDI-10 | 22 |
| RT-04, RT-06, RT-07 | 22 |
| CORE-01 … CORE-13 | 23 |
| DSP-01 … DSP-15 | 24 |
| UX-06 … UX-16 | 25 |
| REF-01 … REF-12 | 26 |
| REL-01 … REL-20 | 27 |

**Coverage:** 128/128 v1 requirements mapped ✓ — no orphans, no duplicates.

**RT ownership resolution:** Phase 21 owns span architecture, no-alloc, oversized blocks, authentic transition, and true bypass (`RT-01..03`, `RT-05`, `RT-08..15`, `CORE-14..18`). Phase 22 owns MIDI and per-sample dynamic/control delivery (`MIDI-*`, `RT-04`, `RT-06`, `RT-07`).

## Progress

**Execution Order:** 19 → 20 → 21 → 22 → 23 → 24 → 25 → 26 → 27

| Phase | Plans Complete | Status | Completed |
|-------|----------------|--------|-----------|
| 19. Baseline, Contracts & Failure Harness | 3/3 | Complete    | 2026-07-12 |
| 20. Pressure Send State Truth | 3/3 | Complete   | 2026-07-12 |
| 21. Realtime Span Engine & True Bypass | 0/TBD | Not started | - |
| 22. MIDI & Per-Sample Control Delivery | 0/TBD | Not started | - |
| 23. Input, Level & Gate Truth | 0/TBD | Not started | - |
| 24. Reverb State & Wet-Dirt Integrity | 0/TBD | Not started | - |
| 25. Presets, UI, Branding & Release Truth | 0/TBD | Not started | - |
| 26. Reference Capture & Sonic Classification | 0/TBD | Not started | - |
| 27. RC Verification, Licensing & Distribution | 0/TBD | Not started | - |

---
*Roadmap created: 2026-07-12 for milestone v1.0 Interaction Truth*
