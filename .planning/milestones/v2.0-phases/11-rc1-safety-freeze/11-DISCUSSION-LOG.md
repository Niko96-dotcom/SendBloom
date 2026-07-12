# Phase 11: RC1 Safety Freeze - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-07-08
**Phase:** 11-RC1 Safety Freeze
**Areas discussed:** Allowed files, Factory presets, APVTS default, Documentation, UI tooltip, Tests, Avoid list

---

## Allowed file touch list

| Option | Description | Selected |
|--------|-------------|----------|
| Minimal surgical set | ParameterLayout, presets XML, ReleaseTruthTest, AdvancedDrawer tooltip, README, RELEASE_CHECKLIST | ✓ |
| Include PluginProcessor | Change init/default program logic | |
| Include SchroederTank32 | Patch accumulator path | |

**User's choice:** Minimal surgical set — no DSP/routing/UI-layout changes.
**Notes:** User hard constraints: no r8brain, no SchroederTank32 rewrite, no WetOverdrive, no gate/send/dry routing.

---

## Factory presets — all `authentic_color=0`

| Option | Description | Selected |
|--------|-------------|----------|
| All 8 presets to 0 | Matches SAFE-02 | ✓ |
| APVTS only | Leave presets with auth=1 | |
| Presets only | Leave APVTS default true | |

**User's choice:** All 8 presets to 0.
**Notes:** Codebase audit: 7/8 currently `value="1"`; only Hot_Clip already 0.

---

## APVTS default — `authentic_color=false`

| Option | Description | Selected |
|--------|-------------|----------|
| Default false | Matches SAFE-01 | ✓ |
| Keep default true | Rely on presets only | |

**User's choice:** Default false (bundled with presets decision).
**Notes:** `ParameterLayout.cpp:56` currently passes `true` as default.

---

## README + RELEASE_CHECKLIST wording

| Option | Description | Selected |
|--------|-------------|----------|
| Both updated | README off-by-default; checklist RC1 safety + honest accumulator disclaimer | ✓ |
| Checklist only | | |
| README only | | |

**User's choice:** Both updated.
**Notes:** Do not claim ProperSRC shipped; accumulator truth when enabled stays honest.

---

## UI label and tooltip

| Option | Description | Selected |
|--------|-------------|----------|
| Tooltip experimental warning | Keep "32k Color" label; tooltip adds experimental/advanced framing | ✓ |
| Minimal tooltip | One line only | |
| No UI changes | | |

**User's choice:** Tooltip experimental warning.
**Notes:** INTEG-04 — no new controls; label unchanged.

---

## Safety-freeze test coverage

| Option | Description | Selected |
|--------|-------------|----------|
| Extend ReleaseTruthTest | `[release][safe]` cases: default off, all presets 0, host-rate HF at 48 kHz | ✓ |
| New dedicated test file | Rc1SafetyFreezeTest.cpp | |
| Layout + preset only | No HF assertion | |

**User's choice:** Extend ReleaseTruthTest.
**Notes:** Keep existing distinct-response test for when user enables 32k Color.

---

## Files to avoid

**User's choice:** Align with hard constraints — SchroederTank32, WetOverdrive, GatedBloomChain routing, r8brain/SRC, UI layout beyond tooltip, product rename, release tags.
**Notes:** Prevents clash with Phase 9 UI and Phases 12–18 SRC work.

---

## Claude's Discretion

- Exact tooltip and doc prose within approved intent.
- SAFE-01 placement (ParameterLayoutTest vs ReleaseTruthTest).
- HF fixture selection for SAFE-03.

## Deferred Ideas

- ProperSRC, core extraction, crossfade, PDC, preset re-enable with auth=1 — Phases 12–18.
