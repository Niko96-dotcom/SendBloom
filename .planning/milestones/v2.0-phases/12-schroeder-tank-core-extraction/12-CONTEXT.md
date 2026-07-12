# Phase 12: SchroederTankCore Extraction - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — smart discuss skipped)

<domain>
## Phase Boundary

Extract a rate-agnostic `SchroederTankCore` from `SchroederTank32` that runs at a single `processingRate` with no `hostRate` or `useAuthenticPath` branches inside. Create `HostRateReverbEngine` wrapper preserving existing host-rate tank behavior for RC1 primary path. Fixed 32,768 Hz core uses unscaled delay table from `SchroederTank32DelayTable`.

**In scope:** New `SchroederTankCore` class, `HostRateReverbEngine` wrapper, RT60 parity impulse tests at size 0.25/0.5/1.0 for both cores, refactor `SchroederTank32` to delegate or thin-wrap.

**Out of scope:** r8brain/SRC adapter (Phase 13), block-level integration in GatedBloomChain (Phase 14), accumulator `processAuthentic` path rewrite, enabling 32k Color by default, UI changes, PDC/latency policy.

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure/refactor phase. Use ROADMAP success criteria, CORE-01–CORE-04 requirements, existing `SchroederTank32`/`SchroederTank32DelayTable` code, and `IReverbEngine` contract as guides.

**Inherited constraints from v2.0 milestone:**
- RC1 host-rate path must remain bit-for-behavior identical (Phase 11 shipped with authentic_color off)
- Fixed-rate core must use unscaled 32,768 Hz delay table (CORE-02)
- Zero heap allocation in process path after prepare()
- Mono processing throughout

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `source/SchroederTank32.h` — current monolithic tank with `processHostRate()` and `processAuthentic()` branches
- `source/SchroederTank32DelayTable.h` — fixed delay constants at 32,768 Hz internal rate
- `source/DampedComb.h`, `source/SchroederAllpass.h` — comb/APF building blocks
- `source/IReverbEngine.h` — virtual interface used by `GatedBloomChain`
- `tests/HighFrequencyRingingDiagnosticsTest.cpp`, `tests/ReleaseTruthTest.cpp` — HF/RT60 test patterns

### Established Patterns
- Per-sample `processSample()` on `IReverbEngine` implementations
- `prepare(sampleRate, maxBlockSize)` allocates delay lines; `processSample` is RT-safe
- RT60 sizing via `sizeToRT60()` in `ParameterCurves.h`
- Catch2 tagged tests: `[release]`, `[dsp]`, etc.

### Integration Points
- `GatedBloomChain` holds `SchroederTank32` (or `IReverbEngine*`) for wet reverb
- `authenticColor` bool routes between host-rate and authentic paths in current `SchroederTank32`
- Phase 13 will wrap fixed-rate core with SRC; Phase 12 must deliver clean core + host wrapper

</code_context>

<specifics>
## Specific Ideas

No specific requirements — infrastructure extraction phase. Refer to ROADMAP Phase 12 success criteria and CORE-01–CORE-04.

</specifics>

<deferred>
## Deferred Ideas

- Proper SRC / r8brain adapter — Phase 13
- Block-level `processBlock()` — Phase 14
- Three-path diagnostics — Phase 15
- Engine crossfade — Phase 16

</deferred>
