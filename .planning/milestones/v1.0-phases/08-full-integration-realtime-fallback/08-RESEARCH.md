# Phase 8: Full Integration / Realtime / Fallback - Research

**Researched:** 2026-07-06
**Domain:** IReverbEngine abstraction, Fdn8Reverb fallback, zero-latency audit, realtime stress (JUCE 8 / C++20)
**Confidence:** HIGH

## Summary

Phase 8 hardens the production chain: extract **`IReverbEngine`** from the embedded `SchroederTank32` in `GatedBloomChain`, add **`Fdn8Reverb`** as ADR-002 fallback behind the same interface (tests + Extended reference; Authentic production path keeps SchroederTank32). **`PluginProcessor::getLatencySamples()`** must explicitly return 0 (CHN-04). **`processBlock`** already preallocates `dryBuffer` in `prepareToPlay`; stress test runs 10,000 blocks at varying sizes to catch regressions (CHN-05). CI **`STRICTNESS_LEVEL`** bumps from 5 → 7.

**Primary recommendation:** Virtual `IReverbEngine` interface; `SchroederTank32` + `Fdn8Reverb` implement it; `GatedBloomChain` holds `std::unique_ptr<IReverbEngine>` created in `prepare()` (not audio thread); `Fdn8ReverbTest` + `RealtimeStressTest` + latency unit test; pluginval 7 in workflow.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

#### Latency & Realtime
- Zero reported plugin latency; no lookahead in processBlock
- Realtime safety: 10,000 blocks varying sizes; no heap alloc, locks, logging, file I/O in audio thread after prepare()
- Preallocate all buffers in prepareToPlay

#### Fdn8Reverb Fallback
- Same IReverbEngine interface as SchroederTank32
- Available for tests and Extended reference; Authentic mode keeps SchroederTank32 primary
- ADR-002: Fdn8 is fallback only

#### CI
- Raise pluginval to strictness 7 for this phase gate

### Claude's Discretion
Fdn8 implementation depth, interface shape, stress harness details.

### Deferred Ideas (OUT OF SCOPE)
- Extended stereo decorrelator (post-v1)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CHN-04 | Zero reported plugin latency; no lookahead | `getLatencySamples() override { return 0; }` + unit test |
| CHN-05 | processBlock realtime-safe after prepare | Preallocated buffers; stress 10k blocks varying sizes |
| VERB-06 | Fdn8Reverb fallback behind same interface | `IReverbEngine` + `Fdn8Reverb` + swap test in chain |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Owner | Rationale |
|------------|-------|-----------|
| Reverb interface | `IReverbEngine.h` | VERB-06 polymorphic swap |
| Primary engine | `SchroederTank32` (implements interface) | Authentic FV-1 path unchanged |
| Fallback engine | `Fdn8Reverb.h` | 8-line FDN per ADR-002 |
| Chain integration | `GatedBloomChain.h` | `unique_ptr<IReverbEngine>`; default Schroeder |
| Zero latency | `PluginProcessor.h` | CHN-04 explicit override |
| Realtime stress | `RealtimeStressTest.cpp` | CHN-05 offline gate |
| Fdn8 proofs | `Fdn8ReverbTest.cpp` | VERB-06 RT60 tail + interface |
| CI strictness | `.github/workflows/build_and_test.yml` | pluginval 7 |
| Human extended smoke | `VERIFICATION.md` | Extended DAW session |

## IReverbEngine Interface

```cpp
class IReverbEngine
{
public:
    virtual ~IReverbEngine() = default;
    virtual void prepare (double sampleRate, int maxBlockSize) noexcept = 0;
    virtual float processSample (float input,
                                 float rt60Seconds,
                                 float darkMix,
                                 bool authenticColor) noexcept = 0;
};
```

`SchroederTank32` and `Fdn8Reverb` both implement this signature (already matches existing `processSample`).

## Fdn8Reverb Design (minimal viable)

Per ADR-002: 8 parallel damped delay lines, input diffusion (2 series allpasses), Householder-style mixing via weighted sum, RT60 via `exp(-6.9078 * Td / RT60)` on each line.

```
input → [predelay if dark] → 2× series APF → split to 8 lines
     → each: DampedComb(delay[i], feedback(rt60), damping(dark))
     → sum(lines) * 0.125 → out
```

Coprime delays at 48 kHz (ms): 37, 41, 43, 47, 53, 59, 61, 67 — scaled to samples. `authenticColor` ignored (Fdn8 is Extended/fallback only).

## Latency Audit

- `PluginProcessor` does not call `setLatencySamples()` — JUCE default 0 ✓
- `SchroederTank32` predelay is wet-path only (not reported latency) ✓
- No `DelayLine` lookahead in dry path ✓
- Add explicit `getLatencySamples() const override { return 0; }` for auditability

## Realtime Stress Harness

```cpp
// 10,000 blocks, sizes cycling 32..1024
// Toggle APVTS params every 100 blocks (prepare-time safe pattern: raw atomic writes)
// REQUIRE_NOTHROW on each processBlock
// Optional: track peak output finite
```

Cannot detect heap alloc portably without custom allocator; stress + code review satisfies CHN-05 for v1.

## CI Change

```yaml
env:
  STRICTNESS_LEVEL: 7  # was 5
```

## Anti-Patterns

- **Heap alloc in processBlock** — `unique_ptr` engine created only in `prepare()`
- **Replacing Schroeder with Fdn8 in default chain** — Fdn8 is fallback/tests only
- **Forking chowdsp GPL** — clean-room Fdn8 using existing `DampedComb` / `SchroederAllpass`

## Integration Regression

Run full ctest after changes; Phase 3–7 routing tags: `[chain]`, `[routing]`, `[send]`, `[verb]`, `[integration]`.

## Sources

### Primary (HIGH)
- `08-CONTEXT.md`, `REQUIREMENTS.md`, `ADR-002-reverb-engine.md`
- `source/GatedBloomChain.h`, `source/SchroederTank32.h`, `source/PluginProcessor.cpp`
- `.github/workflows/build_and_test.yml`

### Secondary (MEDIUM)
- `05-RESEARCH.md` — RT60 measurement helper
- `DampedComb.h`, `SchroederAllpass.h` — reusable atoms

---

*Phase: 08-full-integration-realtime-fallback*
*Research completed: 2026-07-06*
*Ready for planning: yes*
