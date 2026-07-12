---
phase: 25-presets-ui-branding-release-truth
plan: 02
subsystem: presets-release-docs
tags: [presets, pre-v1, design-qa, clean-room, release-truth]

requires:
  - phase: 25-01
    provides: Procedural production faceplate and normalized legal scanner
provides:
  - Explicit pre-v1 development classification on all eight factory preset XML roots
  - Portable design QA based on procedural production truth and runtime test discovery
  - Accurate clean-room documentation for normalized content and filename scanning
affects: [25-03-copy-verification, 26-reference-capture]

key-files:
  created: []
  modified:
    - resources/presets/Sparkle_Verb.xml
    - resources/presets/Cut_Sample_Gate.xml
    - resources/presets/Spacerock_Burn.xml
    - resources/presets/Dry_Dub_Sends.xml
    - resources/presets/Dark_Bloom.xml
    - resources/presets/Firm_Pressure.xml
    - resources/presets/Gated_Room.xml
    - resources/presets/Hot_Clip.xml
    - design-qa.md
    - docs/CLEAN_ROOM.md

key-decisions:
  - "Preset classification is root XML metadata, not an APVTS parameter"
  - "Factory preset loading remains a pure replaceState operation with no migration promise"
  - "Design QA records runtime-discovered test totals and treats the procedural Path B faceplate as production truth"

requirements-completed: [UX-13, UX-15, UX-16]

coverage:
  - id: UX-15
    description: "All factory presets declare pre-v1 development state without parameter or loader changes"
    requirement: UX-15
    verification:
      - kind: integration
        ref: "Builds/Tests \"[release][preset][xml]\""
        status: pass
      - kind: integration
        ref: "Builds/Tests \"[release][safe]\""
        status: pass
    human_judgment: false
  - id: UX-13-16
    description: "Portable design QA and accurate clean-room release documentation"
    requirement: UX-13
    verification:
      - kind: source
        ref: "design-qa.md and docs/CLEAN_ROOM.md"
        status: pass
      - kind: integration
        ref: "Builds/Tests \"[release][legal]\""
        status: pass
    human_judgment: false

duration: 9min
completed: 2026-07-12
status: complete
---

# Phase 25 Plan 02: Preset Classification and Portable Release Docs Summary

**All eight factory presets now declare explicit pre-v1 development state, while portable design and clean-room documentation reflects the procedural production faceplate and current scanner behavior.**

## Accomplishments

- Added `preset_class="pre-v1-dev"` as root metadata to all eight embedded factory preset XML files without changing any parameter entry or loader behavior.
- Rewrote `design-qa.md` around `source/ui/PedalFaceplatePaint.cpp`, generated screenshot evidence, runtime test-count discovery, Path B production truth, and the post-RC0 Path A human gate.
- Updated `docs/CLEAN_ROOM.md` to describe normalized content matching, normalized filename coverage, the full product-facing scan surface, and the internal citation-record boundary.
- Verified README truth without editing it: the pinned `firmware-derived` and `32,768 Hz` strings remain, prohibited firmware-storage terms remain absent, and no exact-fidelity claim was found.

## Task Commits

1. **Task 1: Classify all factory presets as pre-v1 development state** — `3460f18`
2. **Task 2: Rewrite design QA for portable procedural release truth** — `8a05ce2`
3. **Task 3: Document normalized clean-room scanner coverage** — `5380567`
4. **Task 4: Verify README compliance** — no file change; verification-only
5. **Task 5: Run release regressions** — no file change; verification-only

## Verification

- `cmake -B Builds -S .` — PASS
- `cmake --build Builds --config Release --target Tests` — PASS
- `Builds/Tests "[release][preset][xml]"` — PASS (128 assertions, 1 test case)
- `Builds/Tests "[release][safe]"` — PASS (24,020 assertions, 3 test cases)
- `Builds/Tests "[v1][contract][shipping-policy]"` — PASS (8 assertions, 3 test cases)
- `Builds/Tests "[release][legal]"` — PASS (2 assertions, 1 test case)
- `Builds/Tests "[release][verb][authentic]"` — PASS (10 assertions, 2 test cases)
- Eight preset files contain `preset_class="pre-v1-dev"` and eight retain `authentic_color` value `0` — PASS
- `source/FactoryPresets.cpp` SHA-256 matches the plan base version — PASS; `applyEmbeddedXml` remains pure `replaceState`
- `design-qa.md` and `docs/CLEAN_ROOM.md` contain no absolute user paths; the stale fixed test count is absent — PASS

## Deviations from Plan

None. Tasks 4 and 5 were explicitly verification-only and produced no artificial commits.

## Issues Encountered

None. The Release build emitted existing third-party/compiler warnings but completed successfully.

## Next Phase Readiness

- Plan 25-03 can verify Pressure Mode and 32k Color copy and record remaining human gates.
- UX-13, UX-15, and UX-16 automated evidence is green.
- Path A original-asset approval and host/platform visual sign-off remain explicitly `human_needed` where applicable.

## Self-Check: PASSED

- FOUND: commits `3460f18`, `8a05ce2`, and `5380567`
- FOUND: all eight classified preset XML roots
- CONFIRMED: FactoryPresets loader unchanged and README unchanged
- PASS: all five plan-level regression filters
- PASS: documentation portability and stale-count checks

---
*Phase: 25-presets-ui-branding-release-truth*
*Completed: 2026-07-12*
