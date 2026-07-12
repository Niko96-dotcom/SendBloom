# Phase 2: Parameters & State - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure/implementation phase — smart discuss skipped)

<domain>
## Phase Boundary

All effect parameters exist with correct smoothing and bypass; dummy DSP hooks exercise host automation. Delivers APVTS layout, ParameterSnapshot, per-parameter smoothing, 5 ms bypass crossfade, and curve mapping hooks verifiable before real DSP ships in later phases.

</domain>

<decisions>
## Implementation Decisions

### Parameter Architecture
- All parameter IDs immutable in `ParameterIDs.h` per engineering architecture
- `ParameterSnapshot` created once per `processBlock()` — no APVTS atomics in DSP inner loops
- Bypass: 5 ms clickless crossfade
- Curve mappings: size→RT60, distn→pow(2.8), level→sin equal-power, send curves per architecture spec

### Dummy DSP Hooks
- Placeholder processing responds to param changes so DAW automation is audibly verifiable before real DSP (Phase 3+)
- Hooks should be minimal gain/offset or logging-free audible indicators, not full effect chain

### Claude's Discretion
Implementation file layout, exact smoothing time constants from architecture doc, and dummy hook audibility strategy at planner's discretion within architecture constraints.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 1 passthrough `PluginProcessor` in `source/`
- pamplejuce Catch2 test harness in `tests/`
- CMake SharedCode pattern established

### Established Patterns
- `namespace sendbloom`
- JUCE 8 APVTS standard pattern
- C++20, zero heap alloc in processBlock

### Integration Points
- `createParameterLayout()` in processor
- `prepareToPlay` for smoothing sample rate setup
- State save/restore via APVTS

</code_context>

<specifics>
## Specific Ideas

- Follow `SendBloom_engineering_architecture.md` parameter table if present in planning artifacts; else derive from REQUIREMENTS.md PARM-* items
- Unit tests for curve mappings (size→RT60, distn pow, level sin, send curves)

</specifics>

<deferred>
## Deferred Ideas

- Real DSP modules (gates, reverb, OD) — Phase 3+
- UI bindings — Phase 9

</deferred>
