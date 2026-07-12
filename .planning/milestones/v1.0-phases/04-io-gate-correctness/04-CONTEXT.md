# Phase 4: IO + Gate Correctness - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Real InputStage, OutputStage, EnvelopeDetector, and dual NoiseGate profiles (PreSoft, PostHard) replace Phase 3 stubs without breaking proven routing. Mono contract: stereo summed in, dual-mono out. Post gate remains input-keyed.

</domain>

<decisions>
## Implementation Decisions

### IO Stages
- InputStage: input gain dB, soft clip limiter, 50 ms clip-hold flag for UI
- OutputStage: output gain dB trim
- Mono bus contract per IO-03

### Gates
- EnvelopeDetector: peak follower with configurable attack/release
- PreSoft: 150 ms release, hum silencer on wet input only
- PostHard: ≤7 ms Authentic release, brutal wet chop keyed from input detector
- Gate Pre/Post toggle repositions gate on wet path only; dry never gated
- input_threshold_db with 3 dB hysteresis

### Claude's Discretion
Swap stubs in GatedBloomChain incrementally; exact class file layout and test fixtures at planner discretion. Preserve Phase 3 routing proofs as regression tests.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- GatedBloomChain, ParallelWetMixer, gate/reverb/dirt/send stubs from Phase 3
- ParameterSnapshot, SmoothedParameterBank, APVTS from Phase 2

### Integration Points
- Replace stub gate headers with real NoiseGate + EnvelopeDetector
- Insert InputStage before chain, OutputStage after mix
- Clip-hold flag exposed for future UI (Phase 9)

</code_context>

<specifics>
## Specific Ideas

- Study BYOD gate patterns in `.planning/repo-samples/`
- dry-never-gated and input-keyed post-gate unit tests required

</specifics>

<deferred>
## Deferred Ideas

- SchroederTank32 (Phase 5), real PressureSend (Phase 7), UI clip LED (Phase 9)

</deferred>
