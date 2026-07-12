---
phase: 01-scaffold-passthrough
plan: 04
subsystem: compliance
tags: [legal, daw-smoke, metadata]
requires:
  - phase: 01-03
    provides: CI workflow for legal audit step
provides:
  - scripts/check-legal-metadata.sh
  - README DAW smoke procedure
affects: [phase-2]
tech-stack:
  added: [check-legal-metadata.sh]
  patterns: [CI legal grep gate]
key-files:
  created: [scripts/check-legal-metadata.sh]
  modified: [README.md, .github/workflows/build_and_test.yml]
key-decisions:
  - "Exclude include(Pamplejuce*) cmake module names from product-name ban"
requirements-completed: [SCAF-05]
coverage:
  - id: D1
    description: Legal metadata audit passes on clean tree
    requirement: SCAF-05
    verification:
      - kind: unit
        ref: "bash scripts/check-legal-metadata.sh"
        status: pass
    human_judgment: false
  - id: D2
    description: DAW load and passthrough audible in host
    requirement: SCAF-05
    verification:
      - kind: manual_procedural
        ref: "README.md ## DAW Smoke Test"
        status: unknown
    human_judgment: true
    rationale: DAW host load cannot be automated in CI; requires human smoke test
duration: 10min
completed: 2026-07-06
status: complete
human_needed: true
---

# Phase 1 Plan 04: Legal Audit & DAW Smoke Summary

**Automated legal metadata audit in CI with documented DAW passthrough smoke procedure**

## Performance

- **Duration:** ~10 min
- **Tasks:** 2 auto + 1 human-verify (deferred)
- **Files modified:** 3

## Accomplishments

- `scripts/check-legal-metadata.sh` scans for Rainger/Reverb-X/Igor and demo template IDs
- CI legal audit step added before pluginval
- README documents reproducible DAW smoke steps and ad-hoc codesign for macOS

## Human Verification (Pending)

**Status:** `human_needed` — DAW smoke test not executed in this session.

Follow README **DAW Smoke Test** section:
1. Load `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3` or AU `.component`
2. Confirm passthrough audio and SendBloom / Niko Audio Labs metadata
3. Reply `approved` or list issues

## Deviations from Plan

None — human-verify checkpoint documented per autonomous workflow instead of blocking execution.

## Self-Check: PASSED

- FOUND: scripts/check-legal-metadata.sh
- FOUND: README DAW Smoke Test section
- FOUND: 1d69996
