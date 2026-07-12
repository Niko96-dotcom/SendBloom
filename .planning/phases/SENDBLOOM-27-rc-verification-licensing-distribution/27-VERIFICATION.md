---
phase: 27
status: human_needed
verified_at: 2026-07-12T21:30:00+02:00
score: 8/20
---

# Phase 27 Verification

## Automated outcome

Fresh Release configure/build, 260/260 CTest, strictness-10 VST3 pluginval, actual `aumf` AU discovery/auval, legal metadata, reference claims, packaging, and SHA-256 verification passed locally.

## Human and external verification required

1. Confirm a valid JUCE commercial entitlement covers this distribution and record a private evidence reference/date.
2. Obtain positive `auval -a` enumeration for the installed component (direct `aumf` validation already succeeds).
3. Obtain a green GitHub Actions run for the exact candidate on macOS, Windows, and Linux.
4. Complete Logic, Cubase, and REAPER smoke plus a dated minimum 10-minute soak in each host.
5. Developer ID sign the public macOS bundles, notarize, validate/staple as applicable, and record identities/submission results without committing credentials.
6. Resolve the stale pre-existing `v1.0.0-rc0` tag only after every preceding gate passes; verify a clean tree at the final candidate commit.

## Truth boundary

The user's approval accepted recommended product/legal direction. It did not supply the external facts above. Phase status therefore remains `human_needed`; roadmap completion, RC promotion, milestone audit/archive, and cleanup are not authorized by the available evidence.
