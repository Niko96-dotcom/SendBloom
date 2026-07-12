# Phase 12: SchroederTankCore Extraction - Research

**Researched:** 2026-07-08
**Domain:** Schroeder tank DSP extraction (C++20 / JUCE 8 header-only realtime audio)
**Confidence:** HIGH

## Summary

Phase 12 is a surgical refactor of the existing monolithic `SchroederTank32` into a rate-parameterized `SchroederTankCore` plus a `HostRateReverbEngine` wrapper. The tank topology is already correct and tested: predelay → 4 series APFs → 4 parallel damped combs → modulated tank APF with 0.85 output gain [VERIFIED: codebase grep `source/SchroederTank32.h`]. The problem is entanglement: `hostRate`, `useAuthenticPath`, `scaleDelay(..., authentic)`, and `syncCombProcessingRate()` branch throughout `prepare`, `updateCoeffs`, and `processTank`, making the fixed 32,768 Hz core impossible to reuse in Phase 13's SRC adapter without double-scaling risk [VERIFIED: codebase grep; CITED: `.planning/research/PITFALLS.md` Pitfall 7].

The extraction boundary is clear. **`processTank` + host-path `updateCoeffs`** move into `SchroederTankCore`, parameterized by a single `processingRate` set at `prepare()`. Delay lengths become `table[i] * (processingRate / kInternalRate)` — at 32768 Hz the factor is 1.0, satisfying CORE-02 with no `authentic` boolean. **`processHostRate`** becomes `HostRateReverbEngine`, which owns a core prepared at `hostRate` and implements `IReverbEngine`. **`processAuthentic`** (accumulator + anti-image LPF) stays in `SchroederTank32` unchanged — explicitly out of scope per CONTEXT.md. RC1 safety (Phase 11) requires the host-rate path remain behavior-identical when `authentic_color=0` [VERIFIED: `12-CONTEXT.md` inherited constraints].

Existing test infrastructure is mature: `SchroederTank32Test.cpp` already measures RT60 via Schroeder backward energy integration at sizes 0.25/0.5/1.0 with ±15% tolerance [VERIFIED: `tests/SchroederTank32Test.cpp`]. Phase 12 adds parallel tests for `SchroederTankCore` at 32768 Hz and explicit host-wrapper parity (golden impulse or sample-max-diff). No new external packages; JUCE `juce_dsp` atoms (`DampedComb`, `SchroederAllpass`) are reused as-is.

**Primary recommendation:** Extract `SchroederTankCore` with `prepare(processingRate)` + `processSample(input, rt60, darkMix)` (no `authenticColor`), implement `HostRateReverbEngine` as thin `IReverbEngine` delegating to core@hostRate, thin-wrap `SchroederTank32` to route host path through wrapper while leaving `processAuthentic` inline, and add `SchroederTankCoreTest.cpp` for fixed-rate RT60 plus host-path golden parity.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Claude's Discretion
All implementation choices are at Claude's discretion — pure infrastructure/refactor phase. Use ROADMAP success criteria, CORE-01–CORE-04 requirements, existing `SchroederTank32`/`SchroederTank32DelayTable` code, and `IReverbEngine` contract as guides.

**Inherited constraints from v2.0 milestone:**
- RC1 host-rate path must remain bit-for-behavior identical (Phase 11 shipped with authentic_color off)
- Fixed-rate core must use unscaled 32,768 Hz delay table (CORE-02)
- Zero heap allocation in process path after prepare()
- Mono processing throughout

### Deferred Ideas (OUT OF SCOPE)
- Proper SRC / r8brain adapter — Phase 13
- Block-level `processBlock()` — Phase 14
- Three-path diagnostics — Phase 15
- Engine crossfade — Phase 16
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| CORE-01 | `SchroederTankCore` runs at a single `processingRate` with no `hostRate` or `useAuthenticPath` branches inside | Extract `processTank` + host-path coeff logic; single `processingRate` member set in `prepare()`; no `authenticColor` parameter on core API |
| CORE-02 | Fixed 32,768 Hz core uses unscaled delay table from `SchroederTank32DelayTable` | `prepare(32768.0)` → scale factor 1.0 → raw table integers 167/253/328/413, 1057–1202, 677 |
| CORE-03 | `HostRateReverbEngine` wrapper preserves existing host-rate tank behavior for RC1 primary path | Wrapper owns core@hostRate; golden impulse parity vs pre-refactor `processHostRate`; existing `SchroederTank32Test` RT60 cases pass |
| CORE-04 | RT60 within ±15% at size 0.25, 0.5, 1.0 for both host-rate and fixed-rate cores (impulse test) | Reuse `measureRT60` from `SchroederTank32Test.cpp`; new core tests at 32768 Hz; existing host tests via wrapper or `SchroederTank32` |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Tank DSP (APF/comb/LFO/predelay) | `SchroederTankCore` (audio thread) | — | CORE-01: single-rate, branch-free tank math |
| Host-rate delay scaling | `HostRateReverbEngine` | — | Scaling is a rate concern, not tank topology |
| Authentic accumulator + anti-image | `SchroederTank32` facade | — | Out of scope for extraction; Phase 13 replaces with SRC |
| `IReverbEngine` polymorphism | `HostRateReverbEngine`, `SchroederTank32` | `GatedBloomChain` | Chain calls `processSample` per sample; no block API yet |
| RT60 coefficient mapping | `SchroederTankCore` | `DampedComb::setFeedbackForRT60` | Per-comb feedback uses comb delay + `processingRate` |
| RT60 verification | Catch2 offline tests | — | Schroeder integration on rendered IR |
| Chain integration | `GatedBloomChain` (unchanged) | — | Phase 12 does not swap chain wiring |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE `juce_dsp` | 8.x (bundled) | `DelayLine`, `StateVariableTPTFilter`, `ProcessSpec` | Already used by `DampedComb`, `SchroederAllpass`, predelay [VERIFIED: codebase] |
| `SchroederTank32DelayTable` | — | Constexpr delay constants @ 32768 Hz | CORE-02 source of truth [VERIFIED: `source/SchroederTank32DelayTable.h`] |
| `DampedComb` / `SchroederAllpass` | — | Schroeder atoms | Unit-tested in `SchroederAtomsTest.cpp` [VERIFIED: codebase] |
| `ParameterCurves::sizeToRT60` | — | Size knob → seconds | `0.25 + 5.75×size^2.4` [VERIFIED: `source/ParameterCurves.h`] |
| Catch2 | 3.8.1 | Impulse + RT60 regression | `cmake/Tests.cmake` [VERIFIED: codebase] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `ChainTestHelpers.h` | — | `rms()`, Goertzel helpers | Impulse audibility checks |
| `Fdn8ReverbTest.cpp` patterns | — | `IReverbEngine` polymorphic render | Reference for engine-agnostic IR render helper |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Inline extraction in `source/` | `source/reverb/` subfolder | Subfolder requires CMake glob update — current `GLOB_RECURSE` pattern `source/*.h` does not pick up nested dirs [VERIFIED: `CMakeLists.txt` line 40-42] |
| Shared `measureRT60` helper | Duplicate in new test file | Shared header reduces drift; duplication matches existing `Fdn8ReverbTest` pattern |
| `IReverbEngine` block API extension | Keep per-sample only | Block API is Phase 14; extending interface now violates phase boundary |

**Installation:** No new packages. Header-only additions compiled via existing `SharedCode` INTERFACE target.

**Version verification:** Catch2 3.8.1 pinned in `cmake/Tests.cmake` via CPM [VERIFIED: codebase].

## Package Legitimacy Audit

> Phase 12 installs **no new external packages**. Existing dependencies only.

| Package | Registry | Verdict | Disposition |
|---------|----------|---------|-------------|
| — | — | — | N/A — no new installs |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
GatedBloomChain::processSample
        │
        ▼
  IReverbEngine::processSample (rt60, darkMix, authenticColor)
        │
        ├─ authenticColor=true ──► SchroederTank32::processAuthentic
        │                              (accumulator + anti-image LPF)
        │                              [UNCHANGED Phase 12]
        │
        └─ authenticColor=false ─► HostRateReverbEngine
                                        │
                                        ▼
                                   SchroederTankCore @ hostRate
                                   (scaled delays, bright damping)
                                        │
                    ┌───────────────────┴───────────────────┐
                    │  predelay → APF×4 → comb×4 → tank AP │
                    │  single processingRate, no mode branches│
                    └───────────────────────────────────────┘

Offline / Phase 13 path (not wired in Phase 12):
  SchroederTankCore @ 32768 (unscaled table) ← FixedRateAdapter in Phase 13
```

### Recommended Project Structure

```
source/
├── SchroederTankCore.h          # NEW — single-rate tank DSP
├── HostRateReverbEngine.h       # NEW — IReverbEngine @ hostRate
├── SchroederTank32.h            # MODIFIED — facade; host delegates to wrapper
├── SchroederTank32DelayTable.h  # unchanged
├── DampedComb.h                 # unchanged
├── SchroederAllpass.h           # unchanged
├── IReverbEngine.h              # unchanged (no processBlock in Phase 12)
└── GatedBloomChain.h            # unchanged

tests/
├── SchroederTankCoreTest.cpp    # NEW — fixed-rate RT60 + host parity
├── SchroederTank32Test.cpp      # unchanged — regression for host path
└── tests/ChainTestHelpers.h    # optional: extract shared measureRT60
```

**CMake constraint:** New headers must live directly under `source/` (or `source/ui/` pattern) unless `CMakeLists.txt` glob is updated. Do **not** use `source/reverb/` without editing the glob [VERIFIED: `CMakeLists.txt`].

### Pattern 1: Rate-Parameterized Core (No Mode Branches)

**What:** Tank owns one `processingRate`. All time-based math uses that rate. Delay samples = `table[i] * (processingRate / kInternalRate)`.

**When to use:** Every `SchroederTankCore` instance — host wrapper prepares at 48000, fixed adapter (Phase 13) prepares at 32768.

**Example:**

```cpp
// SchroederTankCore.h — conceptual API
class SchroederTankCore
{
public:
    void prepare (double processingRate, int maxBlockSize) noexcept;
    float processSample (float input, float rt60Seconds, float darkMix) noexcept;

private:
  double processingRate_ { SchroederTank32DelayTable::kInternalRate };

  float scaleDelay (int delayAt32k) const noexcept
  {
      return static_cast<float> (delayAt32k)
           * static_cast<float> (processingRate_ / SchroederTank32DelayTable::kInternalRate);
  }

  void resetDelayLengths() noexcept;  // uses scaleDelay only — no authentic bool
  void updateCoeffs (float rt60Seconds, float darkMix) noexcept; // bright damping only
  float processTank (float input) noexcept;
};
```

Source: extracted from `source/SchroederTank32.h` host-path branches [VERIFIED: codebase].

### Pattern 2: Thin Host Wrapper (CORE-03)

**What:** `HostRateReverbEngine` implements `IReverbEngine`, owns `SchroederTankCore`, forwards `prepare(hostRate)` and `processSample` (ignores or asserts `authenticColor==false`).

**When to use:** RC1 primary path; future `SchroederTank32` delegation target.

**Example:**

```cpp
class HostRateReverbEngine : public IReverbEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept override
    {
        core.prepare (sampleRate, maxBlockSize);
    }

    float processSample (float input, float rt60, float darkMix,
                         bool /*authenticColor*/) noexcept override
    {
        return juce::jlimit (-4.0f, 4.0f, core.processSample (input, rt60, darkMix));
    }

private:
    SchroederTankCore core;
};
```

Source: mirrors `processHostRate` clamp in `SchroederTank32.h:195-198` [VERIFIED: codebase].

### Pattern 3: Facade Preserves Authentic Path

**What:** `SchroederTank32` keeps accumulator state (`inputAccumulator`, `outputHold`, `antiImageFilter`) and `processAuthentic`. On `authenticColor` edge, toggles delay reset on the **authentic inline tank** (or shared state) — host path uses separate `HostRateReverbEngine` instance to avoid cross-contamination.

**When to use:** Phase 12 only — authentic path rewrite deferred to Phase 13.

**Trade-off:** Two tank states during transition if sharing one core — prefer **separate core instances** for host wrapper vs authentic inline tank to prevent delay-line corruption when toggling `authenticColor`.

### Anti-Patterns to Avoid

- **Authentic quantization in core:** `quantize9bit`, `kAuthenticBrightDampingHz`, RT60 6.25 s grid belong only in `processAuthentic` path — not in `SchroederTankCore` [VERIFIED: `SchroederTank32.h:142-148`]
- **Double scaling:** Fixed core at 32768 must not also apply host scaling — `prepare(32768)` only [CITED: `.planning/research/PITFALLS.md` Pitfall 7]
- **`source/reverb/` without CMake fix:** Files won't compile into `SharedCode` [VERIFIED: `CMakeLists.txt`]
- **Rewriting `processAuthentic` in Phase 12:** Explicitly out of scope; breaks Phase 13 SRC work sequencing
- **Heap in `processSample`:** All delay buffers allocated in `prepare()` — inherited v2.0 constraint

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Delay lines | Custom circular buffers | JUCE `DelayLine` | Interpolation modes already chosen (None for comb, Linear for APF) [VERIFIED: atoms] |
| Comb damping LPF | One-pole hack | `StateVariableTPTFilter` in `DampedComb` | Existing RT60-calibrated damping |
| RT60 → feedback | Ad-hoc gain | `DampedComb::setFeedbackForRT60(rt60, combDelay)` | `exp(-6.9078 × delaySec / rt60)` [VERIFIED: `DampedComb.h:58-64`] |
| RT60 measurement | Hilbert envelope only | Schroeder backward energy integration | Existing `measureRT60` in tests; standard practice [CITED: https://dsp.stackexchange.com/questions/17121] |
| Host-rate clamping | None | `juce::jlimit(-4.0f, 4.0f, ...)` | Matches shipped host path |

**Key insight:** The tank topology and atoms are done — Phase 12 is boundary drawing, not DSP invention.

## Common Pitfalls

### Pitfall 1: Shared State Between Host and Authentic Paths

**What goes wrong:** Single `SchroederTankCore` instance toggled between host and authentic modes corrupts delay-line state; RT60 tests pass in isolation but toggle causes clicks or wrong tail.

**Why it happens:** Current `SchroederTank32` resets delays on `authenticColor` edge via `resetDelayLengths(useAuthenticPath)`.

**How to avoid:** `HostRateReverbEngine` owns its own core; authentic path keeps separate inline tank (current behavior) until Phase 13 replaces authentic with `FixedRateAdapter`.

**Warning signs:** `authentic_color` toggle changes host-path IR when authentic is off; `ReleaseTruthTest` authentic vs host diff test fails.

### Pitfall 2: Subtle Host-Path Behavior Drift (CORE-03)

**What goes wrong:** Refactor changes predelay rate, LFO increment, or comb `setProcessingSampleRate` timing; RC1 sounds identical to users but regression tests fail or vice versa.

**Why it happens:** `updateCoeffs` recomputed per sample; any reorder of `setDampingCutoff` / `setFeedbackForRT60` / `setDelay` changes filter state.

**How to avoid:** Golden impulse test: render 48 kHz IR pre/post refactor, `maxAbsDiff < 1e-6` for host path with `authentic=false`, `darkMix=0`, fixed RT60. Run before merging.

**Warning signs:** `SchroederTank32Test` RT60 margins tighten; dark predelay test fails.

### Pitfall 3: Fixed-Core RT60 Test at Wrong Sample Rate

**What goes wrong:** Measuring 32768 Hz core IR with `measureRT60(ir, 48000)` over-estimates RT60 by ~48/32.768.

**Why it happens:** Test copies host-rate constant `kSampleRate = 48000`.

**How to avoid:** `core.prepare(32768.0, 512)`; render IR; `measureRT60(ir, 32768.0)`; buffer length `32768 * target * 3.0`.

**Warning signs:** Fixed-core RT60 consistently ~1.46× target.

### Pitfall 4: CMake Glob Miss

**What goes wrong:** New files in nested folder compile in IDE but not CI.

**How to avoid:** Place headers in `source/` root or update `file(GLOB_RECURSE ...)` in `CMakeLists.txt`.

## Code Examples

### Host-Path Delay Scaling (Current → Core)

```cpp
// Current SchroederTank32.h — host branch only
float scaleDelay (int delayAt32k, bool authentic) const noexcept
{
    if (authentic)
        return static_cast<float> (delayAt32k);
    return static_cast<float> (delayAt32k)
         * static_cast<float> (hostRate / SchroederTank32DelayTable::kInternalRate);
}

// SchroederTankCore — single rate, no authentic branch
float scaleDelay (int delayAt32k) const noexcept
{
    return static_cast<float> (delayAt32k)
         * static_cast<float> (processingRate_ / SchroederTank32DelayTable::kInternalRate);
}
```

Source: `source/SchroederTank32.h:89-95` [VERIFIED: codebase]

### updateCoeffs Host-Path Subset (Moves to Core)

```cpp
void SchroederTankCore::updateCoeffs (float rt60Seconds, float darkMix) noexcept
{
    const auto mix = juce::jlimit (0.0f, 1.0f, darkMix);
    predelaySamples = mix * SchroederTank32DelayTable::kDarkPredelaySeconds
                    * static_cast<float> (processingRate_);
    predelayLine.setDelay (predelaySamples);

    const auto dampingHz = juce::jmap (mix,
                                       SchroederTank32DelayTable::kBrightDampingHz,
                                       SchroederTank32DelayTable::kDarkDampingHz);
    const auto rt60 = juce::jmax (rt60Seconds, 0.05f);

    for (size_t i = 0; i < parallelCombs.size(); ++i)
    {
        const auto combDelay = scaleDelay (SchroederTank32DelayTable::kParallelCombDelays[i]);
        parallelCombs[i].setDampingCutoff (dampingHz);
        parallelCombs[i].setFeedbackForRT60 (rt60, combDelay);
    }
}
```

Source: derived from `SchroederTank32.h:116-159` with authentic branches removed [VERIFIED: codebase]

### RT60 Impulse Test (Reuse Pattern)

```cpp
TEST_CASE ("SchroederTankCore RT60 at 32768 Hz size 0.5", "[verb][SchroederTankCore][rt60]")
{
    sendbloom::SchroederTankCore core;
    constexpr double kCoreRate = 32768.0;
    core.prepare (kCoreRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderCoreImpulse (core, target, 0.0f,
                                       static_cast<int> (kCoreRate * target * 3.0));
    const auto measured = measureRT60 (ir, kCoreRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}
```

Source: `tests/SchroederTank32Test.cpp:120-144` adapted [VERIFIED: codebase]

### Golden Parity Test (CORE-03)

```cpp
TEST_CASE ("HostRateReverbEngine matches legacy host path impulse", "[verb][HostRate][parity]")
{
    // Capture baseline IR from git pre-refactor OR inline legacy render helper
    sendbloom::HostRateReverbEngine engine;
    engine.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
  const auto ir = renderEngineImpulse (engine, rt60, 0.0f, 48000);

    // Compare against stored golden vector or pre-extraction SchroederTank32 host IR
    REQUIRE (maxAbsDiff (ir, goldenHostIr) < 1.0e-5f);
}
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Monolithic `SchroederTank32` with mode branches | Core + host wrapper + facade | Phase 12 (this) | Enables Phase 13 SRC adapter |
| PlaceholderReverb | SchroederTank32 tank | v1 Phase 5 | Topology locked |
| Accumulator 32k path | Unchanged in Phase 12 | Phase 13+ | RC1 keeps `authentic_color=0` |

**Deprecated/outdated:**
- `scaleDelay(..., authentic bool)` inside shared tank — replace with rate-parameterized `scaleDelay(int)` in core only
- Planning doc `source/reverb/` layout — defer until CMake glob updated

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | Separate core instances for host wrapper vs authentic inline tank is acceptable | Pattern 3 | Shared-state refactor could be simpler but riskier |
| A2 | `prepare(32768)` with `scaleDelay = table * (rate/32768)` yields exact integer table values | CORE-02 | Floating rounding could off-by-1 sample on delays |
| A3 | Golden parity threshold `1e-5` sufficient for float host path | CORE-03 | May need per-sample exact match or looser bound after testing |
| A4 | `IReverbEngine` unchanged in Phase 12 | Architecture | Phase 14 adds `processBlock`; no blocker for extraction |

## Open Questions

1. **Should `SchroederTank32` authentic path share atoms with a second `SchroederTankCore@32768` or keep duplicated inline tank?**
   - What we know: Authentic path needs unscaled delays + quantization; host wrapper needs scaled delays.
   - What's unclear: Whether instantiating `SchroederTankCore(32768)` inside `processAuthentic` accumulator loop is cleaner than leaving inline `processTank` copy.
   - Recommendation: Minimum Phase 12 scope — leave authentic inline; optionally construct `SchroederTankCore` at 32768 inside facade for authentic only if it reduces duplication without changing behavior.

2. **Extract `measureRT60` to shared test header?**
   - What we know: Duplicated in `SchroederTank32Test.cpp` and `Fdn8ReverbTest.cpp`.
   - Recommendation: Add `tests/ReverbTestHelpers.h` in Phase 12 if `SchroederTankCoreTest.cpp` is created — reduces CORE-04 drift.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build + ctest | ✓ | 4.3.3 | — |
| CTest | Test discovery | ✓ | 4.3.3 | Run `./Tests` binary directly |
| C++20 compiler | `SharedCode` | ✓ (assumed dev env) | — | Required by project |
| JUCE 8 | DSP atoms | ✓ | bundled `JUCE/` | — |
| Catch2 | Unit tests | ✓ | 3.8.1 via CPM | — |
| Build directory | Run tests | ✗ | — | `cmake -B build && cmake --build build` before ctest |

**Missing dependencies with no fallback:**
- Configured `build/` directory (not present at research time) — executor must configure build before verification.

**Missing dependencies with fallback:**
- None for DSP work — all tooling is repo-local.

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (CPM + `catch_discover_tests`) |
| Quick run command | `ctest --test-dir build -C Debug -R "SchroederTankCore|SchroederTank32" --output-on-failure` |
| Full suite command | `ctest --test-dir build -C Debug --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| CORE-01 | Core has no hostRate/authentic branches | static review + compile | Code review / grep assertion in test | ❌ Wave 0 (grep test optional) |
| CORE-02 | 32768 Hz unscaled delays | unit | `ctest -R "SchroederTankCore.*32768"` | ❌ Wave 0 |
| CORE-03 | Host wrapper matches legacy host path | unit (golden IR) | `ctest -R "HostRate.*parity"` | ❌ Wave 0 |
| CORE-04 | RT60 ±15% @ 0.25/0.5/1.0 both rates | unit | `ctest -R "rt60"` | partial ✅ `SchroederTank32Test.cpp` host only |

### Sampling Rate

- **Per task commit:** `ctest --test-dir build -C Debug -R "SchroederTankCore|SchroederTank32|HostRate" --output-on-failure`
- **Per wave merge:** `ctest --test-dir build -C Debug --output-on-failure`
- **Phase gate:** Full suite green (135+ tests per Phase 11 baseline) before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `tests/SchroederTankCoreTest.cpp` — CORE-02, CORE-04 fixed-rate RT60 at 0.25/0.5/1.0
- [ ] `tests/SchroederTankCoreTest.cpp` — CORE-03 host golden impulse parity case
- [ ] `tests/ReverbTestHelpers.h` (optional) — shared `measureRT60` + `renderImpulseResponse`
- [ ] `source/SchroederTankCore.h` — new core implementation
- [ ] `source/HostRateReverbEngine.h` — new wrapper implementation

## Security Domain

Phase 12 is offline header-only DSP refactor on the audio thread — no network, auth, or user data. `security_enforcement` applies at ASVS L1 with minimal surface.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | yes | `juce::jlimit` on wet output (-4..4); `jmax(rt60, 0.05f)`; damping Hz clamped in `DampedComb` |
| V6 Cryptography | no | — |

### Known Threat Patterns for {stack}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| NaN/Inf propagation from host input | Tampering | `jlimit` on output; `std::isfinite` checks in existing authentic test pattern |
| Denormal CPU load | Denial of Service | JUCE DSP / FTZ assumed; no change in Phase 12 |
| Resource exhaustion via huge delay | Denial of Service | `maxDelay = ceil(sampleRate * 1.2)` caps in `prepare()` [VERIFIED: `SchroederTank32.h:21`] |

## Project Constraints (from .cursor/rules/)

No `.cursor/rules/` directory found in project root [VERIFIED: glob 2026-07-08]. No additional project rule directives beyond CONTEXT.md and REQUIREMENTS.md.

## Sources

### Primary (HIGH confidence)
- `source/SchroederTank32.h` — extraction boundary, host vs authentic branches
- `source/SchroederTank32DelayTable.h` — delay constants, rates, damping
- `source/DampedComb.h`, `source/SchroederAllpass.h` — atom APIs
- `tests/SchroederTank32Test.cpp` — RT60 test pattern, ±15% tolerance
- `cmake/Tests.cmake`, `CMakeLists.txt` — test/build wiring

### Secondary (MEDIUM confidence)
- `.planning/research/ARCHITECTURE.md` — core+adapter pattern, component responsibilities
- `.planning/research/PITFALLS.md` — Pitfall 7 double-scaling
- https://dsp.stackexchange.com/questions/17121 — Schroeder integration RT60 method [CITED]

### Tertiary (LOW confidence)
- WebSearch core+adapter pattern — aligns with project architecture docs; not authoritative for this codebase

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new deps; atoms and tests exist
- Architecture: HIGH — extraction boundary mapped line-by-line to current `SchroederTank32.h`
- Pitfalls: HIGH — grounded in PITFALLS.md + test patterns

**Research date:** 2026-07-08
**Valid until:** 2026-08-08 (stable C++ refactor; 30 days)
