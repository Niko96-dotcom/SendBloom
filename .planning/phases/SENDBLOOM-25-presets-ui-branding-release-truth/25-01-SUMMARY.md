---
phase: 25-presets-ui-branding-release-truth
plan: 01
subsystem: ui-release
tags: [branding, clean-room, procedural-ui, legal-scanner]

requires: []
provides:
  - Procedural SENDBLOOM faceplate as the sole production paint path
  - Removal of the legacy faceplate asset and BinaryData reference
  - Normalized product-facing content and filename legal scanning
affects: [25-02-presets-docs, 25-03-release-truth]

key-files:
  created: []
  modified:
    - source/ui/PedalFaceplatePaint.cpp
    - source/ui/PedalFaceplatePaint.h
    - source/PluginEditor.cpp
    - CMakeLists.txt
    - scripts/check-legal-metadata.sh
    - tests/V1ContractShippingPolicyTest.cpp
  deleted:
    - resources/ui/reverbx-faceplate.png

key-decisions:
  - "Path B procedural chassis ships as the production faceplate"
  - "Legal scanning normalizes content, filenames, and banned tokens to alphanumeric form"
  - "INITIAL PATCH is painted by the procedural chassis for the default selection"

requirements-completed: [UX-07, UX-08, UX-09, UX-10, UX-12, UX-14]

coverage:
  - id: UX-07-12
    description: "Shipping-policy and normalized legal scanner contracts"
    requirement: UX-07
    verification:
      - kind: integration
        ref: "Builds/Tests \"[v1][contract][shipping-policy]\""
        status: pass
      - kind: integration
        ref: "Builds/Tests \"[release][legal]\""
        status: pass
    human_judgment: false
  - id: UX-14
    description: "Procedural faceplate geometry and unchanged interaction coordinates"
    requirement: UX-14
    verification:
      - kind: source
        ref: "source/ui/PedalFaceplatePaint.cpp and source/PluginEditor.cpp"
        status: pass
    human_judgment: true

duration: 10min
completed: 2026-07-12
status: complete
---

# Phase 25 Plan 01: Procedural Branding and Legal Truth Summary

**The procedural SENDBLOOM chassis is now the sole production faceplate, the legacy reference asset is absent, and the legal scanner detects normalized content and filename variants.**

## Accomplishments

- Replaced the former title and central letterform art with the SENDBLOOM wordmark and original procedural geometry while preserving the established canvas, controls, hotspots, and overlay coordinates.
- Removed image loading from the production faceplate path and restored `INITIAL PATCH` for the default preset under Path B.
- Removed the legacy faceplate from BinaryData and deleted it from the repository while preserving `knob.png`.
- Strengthened the legal scanner to normalize both scanned material and tokens and to inspect product-facing filenames as well as contents.

## Task Commits

1. **Tasks 1-3 and 6: Procedural faceplate, wordmark, original art, and default name** — `422f4ec`
2. **Task 4: Remove legacy faceplate from BinaryData** — `fe9df02`
3. **Task 5: Delete legacy faceplate asset** — `8b4952a`
4. **Tasks 7-8: Normalize legal scanning and contract coverage** — `fe49cdb`
5. **Scanner executable-mode preservation** — `380a386`

## Verification

- `cmake --build Builds --config Release --target Tests` — PASS
- `Builds/Tests "[v1][contract][shipping-policy]"` — PASS (8 assertions, 3 test cases)
- `Builds/Tests "[release][legal]"` — PASS (2 assertions, 1 test case)
- `resources/ui/reverbx-faceplate.png` absent and `resources/ui/knob.png` present — PASS
- Final visual pixel sign-off remains `human_needed`; source-level coordinate preservation and automated shipping-policy checks pass.

## Deviations from Plan

### Auto-fixed Issues

- Added the faceplate header to shared build sources so all build targets compile the procedural paint path consistently.
- Updated the shipping-policy test to validate the strengthened scanner without embedding prohibited literals in the product-facing scan surface.

## Issues Encountered

The original executor completed and committed production work but stalled before creating this summary. Resume reconciliation inspected the scoped commits and reran the primary automated gates; production work was not repeated.

## Next Phase Readiness

- Plan 25-02 can classify factory preset XML roots and update portable release documentation.
- Shipping policy and legal scanning are green.
- Visual approval remains explicitly `human_needed` for Phase 25 final reporting.

## Self-Check: PASSED

- FOUND: commits `422f4ec`, `fe9df02`, `8b4952a`, `fe49cdb`, `380a386`
- FOUND: procedural production paint path and SENDBLOOM wordmark
- CONFIRMED: legacy faceplate absent; knob asset present
- PASS: shipping-policy and release-legal test filters

---
*Phase: 25-presets-ui-branding-release-truth*
*Completed: 2026-07-12*
