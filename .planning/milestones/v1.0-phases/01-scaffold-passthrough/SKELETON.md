# Walking Skeleton — SendBloom

**Phase:** 1
**Generated:** 2026-07-06

## Capability Proven End-to-End

A developer can configure the CMake project, run Catch2 passthrough tests locally, push to GitHub Actions for a three-platform build with pluginval strictness 5, and load SendBloom as AU/VST3 in a DAW with audio passing through unchanged.

## Architectural Decisions

| Decision | Choice | Rationale |
|---|---|---|
| Framework | JUCE 8 (CMake `juce_add_plugin`) | Industry standard; ADR-001 + PROJECT.md mandate; AU/VST3 bundle layout handled by JUCE |
| Build system | CMake ≥ 3.25 + pamplejuce SharedCode pattern | ODR-safe source sharing between plugin targets and Tests; proven CI patterns |
| C++ standard | C++20 | PROJECT.md constraint; override pamplejuce C++23 default |
| Test framework | Catch2 3.8.1 via CPM (`cmake/Tests.cmake`) | pamplejuce standard; ctest integration |
| CI validation | GitHub Actions matrix + pluginval 1.0.3 strictness 5 | SCAF-03/04; VST3 on all OSes, AU build on macOS only |
| Plugin formats | AU + VST3 only | ROADMAP v1 scope; CLAP/AAX/Standalone deferred |
| Namespace | `sendbloom` | PROJECT.md; processor/editor live in this namespace |
| Manufacturer metadata | Niko Audio Labs / NkMo / SbLm / com.nikoaudiolabs.sendbloom | SCAF-05 placeholders; verify uniqueness before ship |
| Base template | Copy pamplejuce structure (not git fork) | ADR-001; strip demo names, CLAP, IPP, signing for Phase 1 |
| Deployment target | GitHub Actions CI + documented local build | No cloud runtime; artifacts are plugin binaries |
| Directory layout | `source/`, `tests/`, `cmake/` (submodule), `JUCE/` (submodule), `.github/workflows/` | Matches pamplejuce + RESEARCH.md recommended structure |

## Stack Touched in Phase 1

- [x] Project scaffold (CMake, JUCE 8 submodule, Catch2, .clang-format, VERSION)
- [x] Routing — N/A (audio plugin; host routes to `processBlock`)
- [x] Database — N/A (stateless passthrough; no persistence in Phase 1)
- [x] UI — minimal `PluginEditor` (blank or version label; wired to processor)
- [x] Deployment — GitHub Actions `build_and_test.yml` + README local build commands

## Out of Scope (Deferred to Later Slices)

- Effect DSP (gates, reverb, overdrive, pressure send) — Phase 3+
- APVTS parameters and automation — Phase 2
- CLAP, AAX, Standalone, AUv3 formats — post-v1 / later phases
- Code signing, notarization, installer packaging — Phase 10 / when secrets exist
- pluginval strictness 10 — Phase 10 (SCAF-04 allows 5 now)
- Intel IPP SIMD — not needed for passthrough
- Pedal UI, presets, factory XML — Phase 9
- Multi-DAW automated smoke — Phase 10; Phase 1 documents manual DAW load only

## Subsequent Slice Plan

Each later phase adds one vertical slice on top of this skeleton without altering its architectural decisions:

- Phase 2: APVTS parameter layout, smoothing, bypass, dummy DSP hooks
- Phase 3: Ugly signature chain — crude parallel wet/dry routing proof
- Phase 4: Real IO stages and dual gate profiles
- Phase 5: SchroederTank32 reverb engine
- Phase 6: Wet overdrive + dry integrity
- Phase 7: Pressure send and MIDI CC1
- Phase 8: Full integration, realtime safety, Fdn8 fallback
- Phase 9: Pedal UI + factory presets
- Phase 10: pluginval 10, full test suite, legal hardening, multi-DAW smoke
