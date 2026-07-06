# SendBloom

## What This Is

SendBloom is a clean-room JUCE 8 audio plugin that recreates the public behavior of a gated dirty ambience guitar pedal — parallel wet reverb, wet-only overdrive, dual gate placement, and a momentary pressure/send control — without referencing the source hardware or maker in name or metadata. It targets guitarists and producers who want that specific "edited sample" ambience in a DAW, with Authentic mode (mono-first, brutal, immediate) as the default experience and Extended mode reserved for stereo, oversampling, and alternative engines later.

## Core Value

The dry guitar stays clean while the wet path delivers gated, distorted reverb that chops hard when you stop playing — the signature "bloom then cut" behavior that defines the effect.

## Requirements

### Validated

(None yet — ship to validate)

### Active

- [ ] AU and VST3 plugin loads as passthrough, then full effect chain
- [ ] Parallel signal chain: clean dry tap + wet path (pre/post gate → pressure send → reverb → wet OD → mix)
- [ ] SchroederTank32 as primary authenticity reverb (32.768 kHz internal coloration, FV-style topology)
- [ ] Fdn8Reverb as fallback/Extended reference engine behind same interface
- [ ] Dual NoiseGate profiles: PreSoft (hum silencer) and PostHard (≤15 ms wet chop, keyed from input detector)
- [ ] WetOverdrive with fixed drive circuit; `distn` blends clean wet ↔ fully driven wet only
- [ ] PressureSend pad/MIDI with send_connected toggle; release does not clear reverb tank
- [ ] Full APVTS parameter set with ParameterSnapshot smoothing (no atomics in DSP inner loops)
- [ ] Authentic mode: mono-first, zero reported latency, no dry-path distortion at distn=1
- [ ] 8 factory presets with round-trip save/load tests
- [ ] Catch2 unit tests for curves, gates, RT60, dry-path integrity, pressure trails, realtime safety
- [ ] pluginval strictness 10 passes in CI before release
- [ ] Pedal-style UI: five knobs, Dark/Gate toggles, pressure pad, clip LED, advanced drawer
- [ ] pamplejuce-style scaffold: CMake, JUCE 8, Catch2, GitHub Actions build matrix

### Out of Scope

- CLAP and AAX formats — deferred to post-v1
- Commercial licensing / storefront — personal/portfolio project for now
- EEPROM decompilation or FV-1 bytecode reverse engineering — legal boundary
- Rainger / Reverb-X / Igor names in product metadata or marketing
- Hardware A/B measurement protocol — only if physical unit becomes available
- Extended stereo decorrelator, OD oversampling, alternative reverb engines — v1 ships disabled in advanced drawer
- Standalone app — optional, not blocking v1
- Graphs, confidence meters, or algorithm language in main UI

## Context

**Engineering handoff:** `SendBloom_engineering_architecture.md` (v0.1) is the authoritative implementation spec — source tree, signal chain pseudocode, parameter table, DSP class contracts, test acceptance criteria, and 20-step build order.

**Prior research (superseded where conflicting):** Eight-lane GitHub investigation in `.planning/RESEARCH_CORPUS.md`, user dossiers, and `BUILD_MICROSTEPS.md` informed scaffold choice (pamplejuce), gate patterns (BYOD), and legal guardrails. ADR-001 (pamplejuce base) remains valid. ADR-002 (FDN-primary) is superseded — SchroederTank32 is now primary; Fdn8Reverb is fallback only.

**Repo samples:** `.planning/repo-samples/` contains shallow clones for reference (pamplejuce, chowdsp SimpleReverb, BYOD gate, etc.) — study only, not forks.

**Competitive landscape:** No OSS plugin matches the full gated-dirty-ambience routing at ≥7/10 fidelity; routing topology is the product moat. Northern Valley DV-1 and claytonyen/gated-reverb-distortion are A/B references, not code sources.

**Namespace:** `sendbloom` | **Manufacturer placeholder:** Niko Audio Labs | **Codes:** NkMo / SbLm (verify uniqueness before ship)

## Constraints

- **Tech stack:** JUCE 8, C++20, CMake, Catch2, pluginval — pamplejuce-style repository pattern
- **Realtime safety:** Zero heap allocation, locks, logging, file I/O, or UI access in `processBlock()` after `prepare()`
- **License:** Personal/portfolio — prefer MIT-compatible references only; no GPL forks as base
- **Legal:** Clean-room implementation inspired by public behavior only; no trademarked names; FTO review before any future commercial release
- **Audio contract:** Mono-authentic processing; stereo buses sum to mono internally unless Extended stereo enabled; zero reported plugin latency in v1
- **Parameter stability:** APVTS IDs immutable after release; all continuous params smoothed per architecture spec

## Key Decisions

| Decision | Rationale | Outcome |
|----------|-----------|---------|
| Product name: SendBloom | Working title from architecture handoff; avoids third-party trademarks | — Pending |
| SchroederTank32 primary, Fdn8Reverb fallback | Closest to pedal FV-1 behavior at 32 kHz; FDN for Extended/tests | — Pending |
| Architecture doc supersedes FDN-first ADR-002 | User confirmed on project init | — Pending |
| Personal/portfolio release (no commercial v1) | Defers JUCE Indie license decision | — Pending |
| pamplejuce-style scaffold | Catch2 + pluginval CI + packaging patterns already researched | — Pending |
| Post gate keyed from input detector, not wet tail | Preserves "edited sample" chop when player stops | — Pending |
| Authentic vs Extended modes | Mono-first brutal default; stereo/OS/fine controls gated behind Extended | — Pending |

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
*Last updated: 2026-07-06 after initialization*
