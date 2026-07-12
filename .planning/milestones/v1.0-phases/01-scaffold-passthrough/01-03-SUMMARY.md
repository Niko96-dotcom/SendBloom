---
phase: 01-scaffold-passthrough
plan: 03
subsystem: infra
tags: [github-actions, pluginval, ci]
requires:
  - phase: 01-02
    provides: Buildable VST3 artifact path
provides:
  - Three-platform GHA build_and_test workflow
  - pluginval strictness 5 validation step
affects: [01-04]
tech-stack:
  added: [pluginval 1.0.3 in CI]
  patterns: [matrix macOS/Windows/Linux, Xvfb on Linux]
key-files:
  created: [.github/workflows/build_and_test.yml]
  modified: []
key-decisions:
  - "Omitted signing, IPP, CLAP, packaging from Phase 1 CI"
  - "VST3-only pluginval on all platforms"
requirements-completed: [SCAF-03, SCAF-04]
coverage:
  - id: D1
    description: GHA matrix defines ubuntu-22.04, macos-14, windows-latest
    requirement: SCAF-03
    verification:
      - kind: unit
        ref: "grep matrix os entries in build_and_test.yml"
        status: pass
    human_judgment: false
  - id: D2
    description: pluginval runs at strictness level 5
    requirement: SCAF-04
    verification:
      - kind: unit
        ref: "env.STRICTNESS_LEVEL: 5 in workflow"
        status: pass
    human_judgment: false
  - id: D3
    description: CI workflow green on all three matrix legs
    requirement: SCAF-03
    verification:
      - kind: integration
        ref: "GitHub Actions run"
        status: unknown
    human_judgment: true
    rationale: Workflow not yet pushed to remote; local VST3 build verified only
duration: 15min
completed: 2026-07-06
status: complete
---

# Phase 1 Plan 03: CI Matrix Summary

**Slimmed GitHub Actions workflow with three-platform matrix and pluginval strictness 5**

## Performance

- **Duration:** ~15 min
- **Tasks:** 3/3
- **Files modified:** 1

## Accomplishments

- `build_and_test.yml` adapted from pamplejuce without IPP, signing, CLAP, or packaging
- STRICTNESS_LEVEL 5 enforced in env and pluginval CLI
- Local VST3 Release build succeeds as dry-run equivalent

## CI Status

- **Remote workflow:** Pending first push to GitHub remote
- **Local gate:** VST3 target builds; ctest green

## Self-Check: PASSED

- FOUND: .github/workflows/build_and_test.yml
- FOUND: 9699971
