# Phase 15: Three-Path Diagnostics - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning
**Mode:** Auto-generated (infrastructure phase — smart discuss skipped)

<domain>
## Phase Boundary

Build engineering diagnostics harness comparing three reverb paths side-by-side: host-rate, LegacyAccumulator, and ProperSRC. Quantify HF imaging metrics across fixture suite. Prove ProperSRC architecturally fixes imaging before user-facing 32k Color enablement (Phase 18).

**In scope:** Three-path render harness, HF metrics (RMS >10k/14k, dominant tail peak, narrowband dominance, spectral centroid), multi-rate ProperSRC consistency tests, DIAG-01 through DIAG-04, TEST-08, TEST-11.

**Out of scope:** User-facing UI for diagnostics, engine crossfade (Phase 16), PDC (Phase 17), enabling 32k Color for users (Phase 18).

</domain>

<decisions>
## Implementation Decisions

### Claude's Discretion
All implementation at Claude's discretion. Reuse HighFrequencyRingingDiagnosticsTest patterns and Phase 13 Authentic32Mode diagnostics API. Harness is engineering/test-only, not user-facing.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — Goertzel 14825 Hz, narrowband dominance, config A fixtures
- `source/FixedRateAdapter.h` — ProperSRC and LegacyAccumulator modes
- `source/HostRateReverbEngine.h` — host-rate path
- `source/SchroederTank32.h` — setAuthentic32ModeForDiagnostics() from Phase 14
- `tests/ReverbTestHelpers.h` — impulse/RT60 helpers

</code_context>

<specifics>
## Specific Ideas

No specific requirements — diagnostics harness per ROADMAP Phase 15.

</specifics>

<deferred>
## Deferred Ideas

- Engine crossfade — Phase 16
- User enablement — Phase 18

</deferred>
