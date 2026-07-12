# Phase 13: FixedRateAdapter + r8brain - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — smart discuss skipped)

<domain>
## Phase Boundary

Build `FixedRateAdapter` wrapping `SchroederTankCore` with bandlimited hostRate ↔ 32,768 Hz conversion via r8brain-free-src. Realtime-safe: zero heap allocation in `processBlock()`. Internal `Authentic32Mode` enum (Off / LegacyAccumulator / ProperSRC) for diagnostics only — not user-facing.

**In scope:** r8brain integration, FixedRateAdapter class, Authentic32Mode enum, SRC imaging comparison test vs LegacyAccumulator, reset semantics, multi-rate support (44.1/48/88.2/96 kHz).

**Out of scope:** GatedBloomChain block integration (Phase 14), three-path diagnostics harness (Phase 15), engine crossfade (Phase 16), PDC/latency policy (Phase 17), user-facing 32k Color enablement (Phase 18).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation choices at Claude's discretion. Use ROADMAP SRC-01–SRC-06, TEST-10 requirements, Phase 12 SchroederTankCore/HostRateReverbEngine as upstream, and PROJECT.md r8brain-first SRC decision.

**Inherited constraints:**
- r8brain-first prototype (MIT license, proven latency API)
- Zero heap allocation in processBlock after prepare
- Mono processing
- RC1 host-rate path unchanged (authentic_color off default)

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `source/SchroederTankCore.h` — fixed-rate tank core from Phase 12
- `source/HostRateReverbEngine.h` — host-rate wrapper (RC1 primary path)
- `source/SchroederTank32.h` — still has `processAuthentic()` accumulator path (LegacyAccumulator reference)
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — HF imaging metrics at 14825 Hz
- `tests/ReverbTestHelpers.h` — RT60/impulse helpers from Phase 12

### Integration Points
- Phase 14 will wire FixedRateAdapter into GatedBloomChain block processing
- Authentic32Mode enum enables three-path comparison in Phase 15 diagnostics

</code_context>

<specifics>
## Specific Ideas

No specific requirements — SRC adapter infrastructure phase per ROADMAP Phase 13.

</specifics>

<deferred>
## Deferred Ideas

- Block-level GatedBloomChain integration — Phase 14
- Three-path diagnostics UI — Phase 15
- Engine crossfade — Phase 16

</deferred>
