# Phase 1: Scaffold & Passthrough - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — smart discuss skipped)

<domain>
## Phase Boundary

Developer can build, CI-verify, and load a legal SendBloom passthrough plugin in AU and VST3. This phase establishes the pamplejuce-style repository foundation, empty passthrough DSP, GitHub Actions build matrix, pluginval strictness 5 in CI, and correct plugin metadata (SendBloom / NkMo / SbLm) with no third-party trademarked names.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP success criteria, PROJECT.md constraints (JUCE 8, C++20, CMake, Catch2, pamplejuce pattern), ADR-001 (pamplejuce base), and `.planning/repo-samples/` for reference patterns. Namespace: `sendbloom`. Manufacturer placeholder: Niko Audio Labs.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `.planning/repo-samples/` — shallow clones of pamplejuce, chowdsp SimpleReverb, BYOD gate, PinkGuitarFX for study (not forks)
- `.planning/ADR-001-fork-decision.md` — pamplejuce base decision
- `.planning/RESEARCH_CORPUS.md` — prior eight-lane GitHub investigation

### Established Patterns
- Greenfield project — no source code in repo yet
- Engineering architecture doc referenced in PROJECT.md as authoritative spec (locate or derive from planning artifacts)

### Integration Points
- CMake + JUCE 8 module integration
- GitHub Actions CI matrix (macOS, Windows, Linux)
- pluginval in CI pipeline
- AU + VST3 format targets only (CLAP/AAX deferred)

</code_context>

<specifics>
## Specific Ideas

- Follow pamplejuce scaffold conventions: Catch2 unit tests, pluginval CI, packaging patterns
- Passthrough only — no effect DSP in this phase
- Legal metadata guardrails: no Rainger / Reverb-X / Igor references

</specifics>

<deferred>
## Deferred Ideas

None — infrastructure phase.

</deferred>
