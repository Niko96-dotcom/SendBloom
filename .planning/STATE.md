---
gsd_state_version: 1.0
milestone: v1.0
milestone_name: Interaction Truth, Realtime Safety & Release Candidate
current_phase: 27
current_phase_name: RC Verification, Licensing & Distribution
current_plan: 27-03 complete
status: awaiting_human
stopped_at: Phase 27 automated work complete; external RC gates remain human_needed
last_updated: "2026-07-12T21:34:00+02:00"
last_activity: 2026-07-12
last_activity_desc: Phase 27 local automated verification complete; RC promotion blocked on external evidence
progress:
  total_phases: 8
  completed_phases: 7
  total_plans: 25
  completed_plans: 24
---

# Project State

## Project Reference

See: .planning/PROJECT.md (updated 2026-07-12)

**Core value:** Pressure Mode, MIDI, and host automation must tell the truth — dry at rest with decaying tails; realtime never lies about blocks, bypass, or allocation.
**Current focus:** Phase 27 RC verification, licensing, and distribution truth

## Current Position

Phase: 27 of 27 (RC Verification, Licensing & Distribution)
Current Plan: 27-03 complete
Total Plans in Phase: 3
Status: Awaiting required human/external release evidence
Last activity: 2026-07-12 — local automated RC gates completed; no tag transition or milestone archive

Progress: [███████░░░] 67%

## Performance Metrics

**Velocity:**

- Total plans completed: 23
- Average duration: ~10 min
- Total execution time: ~0.6 hours

**By Phase:**

| Phase | Plans | Total | Avg/Plan |
|-------|-------|-------|----------|
| 19 | 3 | - | - |
| 20 | 3 | - | - |
| 21 | 3 | - | - |
| 22 | 3 | - | - |
| 23 | 3 | - | - |
| 24 | 3 | - | - |
| 25 | 3 | - | - |
| 26 | 2 | - | - |

**Recent Trend:**

- Last 5 plans: ~10 min
- Trend: —

*Updated after each plan completion*
| Phase 22 P01–P03 | ~25 min combined | MIDI purity + sample-accurate + per-sample |
| Phase 23 P01–P03 | ~20 min combined | Input anchors + wet Level + PostHard ramp |

## Accumulated Context

### Decisions

Decisions are logged in PROJECT.md Key Decisions table. Locked for this milestone:

- Phase numbering continues at 19–27 (ProperSRC conceptual 11–18)
- ADRs ADR-V1-01 … ADR-V1-17 locked
- Phase 27 must not begin while automated requirements from Phases 19–25 are red
- Human gates stay `human_needed`
- RT ownership: Phase 21 = span/no-alloc/bypass; Phase 22 = MIDI + per-sample delivery (RT-04/06/07)
- [Phase 22]: MIDI CC1 → PressureController midi target only (no APVTS store / setValueNotifyingHost)
- [Phase 22]: MIDI target persists across blocks; span = min(..., distanceToNextCc1)
- [Phase 22]: GatedBloomChain consumes per-sample distn/send/threshold arrays (ADR-V1-06)
- [Phase 22]: MidiSendAmountTest + ReleaseTruth MIDI updated to ADR-V1-03 purity
- [Phase 23]: inputGainDb = −9+18*smoothstep; UI/Gate Sens formatters call ParameterCurves
- [Phase 23]: Level wet-only (dry=1); removed dryGain/levelDryGain
- [Phase 23]: PostHard 0.75 ms linear close ramp (ADR-V1-11)

### Pending Todos

None yet.

### Blockers/Concerns

- Phase 27 canonical blockers are resolved: fresh Release CTest is 260/260 PASS with one explicit unavailable ZipFile API skip; strictness-10 VST3 pluginval and direct `aumf` auval pass
- `auval -a` did not positively enumerate the copied AU even though direct `aumf` validation succeeded; REL-06 remains open
- Exact-candidate macOS/Windows/Linux CI is not green; latest visible main run failed
- Logic/Cubase/REAPER smoke and three 10-minute soaks remain `human_needed`
- Commercial JUCE path is approved, but covering entitlement is unverified; Developer ID signing and notarization evidence are absent
- Pre-existing `v1.0.0-rc0` tag points to old commit `3d097d2`, not this candidate; it was not moved or deleted
- Several REL/REF/UX gates require human evidence (`human_needed`)
- Phase 25 automated shipping-policy, legal, preset, release-safety, UI geometry, snapshot-build, and Phase 19–24 regression gates are green
- Phase 25 visual/host alignment and editorial gates were approved by Niko; Path A remains explicitly deferred post-RC0
- Phase 26 Outcome B passed with sole status `original-inspired`; hardware grids and Niko listening remain `human_needed`

## Session Continuity

**Last session:** 2026-07-12T17:12:55.783Z
**Stopped At:** Phase 27 automated work complete; external RC gates remain `human_needed`
**Resume File:** None

**Next action:** Supply the external evidence listed in `27-VERIFICATION.md`; then re-run Phase 27 verification before any tag or milestone lifecycle action

**Roadmap:** `.planning/ROADMAP.md` (9 phases, 128/128 requirements mapped)
**Requirements:** `.planning/REQUIREMENTS.md`
**Spec:** `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md`
