# ADR-001: Repository Fork Decision

**Status:** PROPOSED  
**Date:** 2026-07-06

## Context

We need a JUCE 8 plugin scaffold with CI, tests, and pluginval before any DSP. Multiple public templates exist.

## Decision

**Fork [sudara/pamplejuce](https://github.com/sudara/pamplejuce) (MIT)** as the repository base.

Supplement with CMake patterns from **[anthonyalfimov/JUCE-CMake-Plugin-Template](https://github.com/anthonyalfimov/JUCE-CMake-Plugin-Template)** if pamplejuce's JUCE pin needs updating.

Do **not** fork ChowDSP JUCEPluginTemplate as base (adds vendor workflow).

## Rationale

| Criterion | pamplejuce | anthonyalfimov | ChowDSP template |
|-----------|------------|----------------|------------------|
| Catch2 wired | ✓ | ✗ | ✗ |
| pluginval CI | ✓ | partial | ✗ |
| JUCE 8 | ✓ | ✓ | ✓ |
| License | MIT | MIT | BSD |
| Packaging/notarize | ✓ | ✗ | ✗ |
| Complexity | medium | low | low |

## Consequences

- Strip pamplejuce demo DSP; keep `tests/`, `.github/workflows/`, CMake structure
- Rename product to working title (see ADR naming)
- Pin JUCE tag explicitly in CMake
- CLAP deferred to MB-080; pamplejuce may need CLAP module added later

## References

- Local clone: `.planning/repo-samples/sudara-pamplejuce/`
- Validation reference: `.planning/repo-samples/anthonyalfimov-JUCE-CMake-Plugin-Template/.github/workflows/Validation.yml`
