---
phase: 25-presets-ui-branding-release-truth
plan: 03
subsystem: release-truth
tags: [ui-copy, clean-room, release-gates, human-needed]

requires:
  - phase: 25-01
    provides: Procedural production faceplate and normalized legal scanner
  - phase: 25-02
    provides: Pre-v1 preset classification and portable release documentation
provides:
  - Canonical Pressure Mode UI copy without controller naming
  - Evidence-aligned 32k Color tooltip
  - Phase 25 release-truth confirmation with explicit human gates
affects: [26-reference-capture, 27-release-closeout]

key-files:
  created:
    - .planning/phases/SENDBLOOM-25-presets-ui-branding-release-truth/25-SUMMARY.md
    - .planning/phases/SENDBLOOM-25-presets-ui-branding-release-truth/25-03-SUMMARY.md
  modified:
    - source/ui/AdvancedDrawer.cpp

key-decisions:
  - "Keep the canonical short Pressure Mode tooltip as the sole UI explanation"
  - "Remove the unsupported blanket HF-artifacts warning after supported-rate ProperSRC gates passed"
  - "Ship procedural Path B now; defer Path A asset approval until post-RC0"

requirements-completed: [UX-06, UX-07, UX-08, UX-09, UX-10, UX-11, UX-12, UX-13, UX-14, UX-15, UX-16]
completed: 2026-07-12
status: complete
---

# Phase 25: Presets, UI, Branding & Release Truth Summary

**Phase 25 is green by automation. The procedural SENDBLOOM chassis is the production faceplate for this release, shipping-policy is green because prohibited material was removed and scanning was strengthened, and all remaining human judgments stay explicitly `human_needed`.**

## Release Decision

- **Path B ships now:** the procedural chassis is the sole production paint path for this release. It carries the `SENDBLOOM` wordmark and original central geometry while retaining the locked 420x780 canvas, hotspot grid, and overlay coordinates.
- **Path A is deferred, not rejected:** a future Niko-approved `resources/ui/sendbloom-faceplate.png` may replace Path B post-RC0. Asset approval is `human_needed` under UX-11 and BASE-08. Any Path A asset must reuse the identical 420x780 canvas, hotspot grid, and overlay coordinates locked in `25-UI-SPEC.md` section 5.
- Final pixel-level visual sign-off for the procedural faceplate is `human_needed`. Source geometry and automated contracts prove coordinate preservation; they do not substitute for human visual approval.

## Copy Truth

- UX-06 is green by source and automated verification: the sole Pressure Mode tooltip remains verbatim, `Pressure Mode: when on, wet feed follows pressure; when off, reverb stays always-on.` The word `controller` and normalized banned UI tokens are absent from the inspected UI source.
- The 32k Color tooltip retains `Experimental — off by default until validated.`, `Original software — not firmware-derived.`, and the host-rate production-default statement.
- The blanket `May exhibit HF artifacts at some host rates` warning was removed. Phase 24 proves ProperSRC output pre-clear, zero unwritten samples, and green supported-rate SRC/HF imaging gates; no current failing supported-rate evidence justified that warning. This does not change 32k Color behavior or its off-by-default policy.
- Final editorial copy sign-off remains `human_needed`; the automated checks prove exact source text and prohibited-token absence.

## Requirement Status

| Requirement | Status | Evidence / human boundary |
|---|---|---|
| UX-06 | Green by automation | Canonical Pressure Mode tooltip exact; prohibited controller wording absent. Final copy sign-off `human_needed`. |
| UX-07 | Green by automation | Product-facing normalized content scan passes. |
| UX-08 | Green by automation | Legacy named shipping asset absent; normalized filename scan passes. |
| UX-09 | Green by automation | Procedural title is `SENDBLOOM`. |
| UX-10 | Green by automation | Reference faceplate removed from disk, CMake/BinaryData, and paint path. |
| UX-11 | Satisfied by Path B | Procedural original ships. Path A asset approval and final visual sign-off remain `human_needed`. |
| UX-12 | Green by automation | Legal scanner normalizes punctuation, spacing, case, and scans filenames. |
| UX-13 | Green by automation | `design-qa.md` uses repository-relative paths and runtime evidence. |
| UX-14 | Green by source contract | Hotspot and overlay coordinates preserved. Final pixel/hit-target visual sign-off `human_needed`. |
| UX-15 | Green by automation | Eight preset roots declare `preset_class="pre-v1-dev"`; loader has no migration promise. |
| UX-16 | Green by automation | README and clean-room assertions describe verified behavior only. |

No human-only gate was silently represented as automated success (BASE-08).

## What Flipped Green

- `[v1][contract][shipping-policy]` is green across Plans 01–03. Plan 01 removed the prohibited product-facing material and strengthened normalized content/filename scanning; no scanner rule was weakened or bypassed.
- Plan 02 added explicit pre-v1 preset classification and brought design/clean-room documentation onto portable current evidence.
- Plan 03 locked canonical Pressure Mode copy and aligned the 32k Color tooltip with Phase 24 evidence.
- `[release][safe]` remains green: factory preset parameter parity is intact and `authentic_color=0` remains the default and preset state.

## Phase 25 Roadmap Acceptance

1. **Met:** no product-facing prohibited string or shipping filename remains; procedural paint says `SENDBLOOM`; the reference faceplate is removed.
2. **Met with explicit human boundary:** procedural Path B ships and source coordinates preserve hotspot/overlay alignment. Path A approval and final visual sign-off are `human_needed`.
3. **Met:** legal scanning normalizes spelling variants and filenames; design QA is repository-relative and uses current evidence.
4. **Met:** Pressure Mode copy avoids third-party controller naming; preset sessions declare pre-v1 state without a migration promise; README and clean-room docs remain evidence-bound.

## Commits

- Plan 25-01 production/docs: `422f4ec`, `fe9df02`, `8b4952a`, `fe49cdb`, `380a386`, `ce4f4fc`
- Plan 25-02 production/docs: `3460f18`, `8a05ce2`, `5380567`, `28ec15d`
- Plan 25-03 tooltip evidence alignment: `2ed6c84`

## Final Verification

Build at Plan 03 HEAD:

- `cmake --build Builds --config Release --target Tests` — PASS
- `[v1][contract][shipping-policy]` — PASS (8 assertions, 3 test cases)
- `[release][legal]` — PASS (2 assertions, 1 test case)
- `[release][safe]` — PASS (24,020 assertions, 3 test cases)
- `[release][preset][xml]` — PASS (128 assertions, 1 test case)
- `[release][verb][authentic]` — PASS (10 assertions, 2 test cases)
- `[pressure-release]` — PASS (8 assertions, 1 test case)
- `[oversized-block]` — PASS (2 assertions, 1 test case)
- `[true-bypass]` — PASS (4 assertions, 1 test case)
- `[midi-apvts]` — PASS (5 assertions, 2 test cases)
- `[input-anchors]` — PASS (3 assertions, 1 test case)
- `[posthard]` — PASS (7 assertions, 1 test case)
- `[per-sample]` — PASS (18 assertions, 3 test cases)
- `[realtime]` — PASS (26,150,428 assertions, 12 test cases)
- `[authentic]` — PASS (48,063 assertions, 6 test cases)
- `[wet-dirt]` — PASS (4,111 assertions, 7 test cases)

## Deviations

The plan allowed retaining the HF warning only with residual Phase 24 evidence. Current supported-rate gates instead support removing it, so the default trim path was taken. No DSP, preset policy, parameter, or production-default behavior changed.

## Human Gates Retained

- `human_needed`: Niko approval of any future Path A original asset (post-RC0).
- `human_needed`: final pixel-level visual sign-off of the procedural production faceplate.
- `human_needed`: final editorial sign-off of Pressure Mode and 32k Color wording.

## Self-Check: PASSED

- FOUND: `source/ui/AdvancedDrawer.cpp` with exact canonical Pressure Mode copy and evidence-aligned 32k tooltip.
- FOUND: phase-level `25-SUMMARY.md` and plan-discovery `25-03-SUMMARY.md`.
- PASS: all task acceptance criteria and the complete final regression bundle.
- CONFIRMED: unrelated `.planning/config.json`, `.serena/`, and `--help` were not staged or modified by this plan.

