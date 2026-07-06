# SendBloom v1.0 Release Checklist

**Milestone:** v1.0  
**Date:** 2026-07-06

## Pre-Release Automated Gates

- [x] Full Catch2 suite passes (`ctest --test-dir Builds --output-on-failure`) — **103/103**
- [x] Release AU + VST3 build succeeds (`cmake --build Builds --config Release`)
- [x] pluginval strictness **10** passes on Release VST3
- [x] Legal metadata audit passes (`bash scripts/check-legal-metadata.sh`)
- [x] Clean-room positioning documented (`docs/CLEAN_ROOM.md`)

## Artifacts

| Format | Path (local Release build) |
|--------|----------------------------|
| VST3 | `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3` |
| AU (macOS) | `Builds/SendBloom_artefacts/Release/AU/SendBloom.component` |

## Legal & Metadata (LEG-01, LEG-02)

- [x] No Rainger / Reverb-X / Igor in product name, CMake metadata, sources, presets, or README
- [x] Factory preset XML included in legal scan (`resources/presets/*.xml`)
- [x] Clean-room statement published (`docs/CLEAN_ROOM.md`)

## Multi-DAW Smoke (TEST-07) — Human Required

- [ ] **Logic** — AU loads, UI renders, audio processes, presets accessible
- [ ] **Cubase** — VST3 loads, plugin info correct, bypass works
- [ ] **REAPER** — VST3 loads in FX chain, wet/dry and pressure pad functional

See README **Phase 10 — Multi-DAW Release Smoke** for step-by-step procedures.

## Version Tag (on human approval)

```bash
git tag -a v1.0.0 -m "SendBloom v1.0 — gated dirty ambience AU/VST3"
```

## Post-v1 Deferred

- CLAP / AAX formats
- Commercial storefront
- Extended stereo / dirt oversampling (shown disabled in UI)
