# Phase 11: RC1 Safety Freeze - Research

**Researched:** 2026-07-08
**Domain:** JUCE 8 / C++20 audio plugin — APVTS defaults, factory presets, release-truth testing
**Confidence:** HIGH

## Summary

Phase 11 is a **configuration and documentation safety freeze**, not a DSP rewrite. The broken 32k Color path (`SchroederTank32::processAuthentic()` accumulator + hold pseudo-SRC) remains in code but is gated behind `authentic_color`. Today, fresh loads and 7 of 8 factory presets enable it by default, exposing a stable 14–15 kHz imaging whistle at 48 kHz host rate. The host-rate path (`processHostRate()`) is already clean — verified by existing `HighFrequencyRingingDiagnosticsTest` config `A_rev_bright` (`authenticColor=false`). [VERIFIED: codebase grep + `source/SchroederTank32.h`, `tests/HighFrequencyRingingDiagnosticsTest.cpp`]

Implementation is deliberately minimal: flip one APVTS default in `ParameterLayout.cpp` (line 56: `true` → `false`), set `authentic_color value="0"` in all 8 preset XML files (7 currently `1`, only `Hot_Clip.xml` is `0`), extend `ReleaseTruthTest.cpp` with three `[release][safe]` cases, update tooltip and docs. BinaryData preset blobs regenerate automatically on the next CMake build — no manual `BinaryData*.cpp` edits. [VERIFIED: `CMakeLists.txt` `juce_add_binary_data`, `11-CONTEXT.md` D-01]

The test suite currently has **132 Catch2 cases** via ctest. `RealtimeStressTest` intentionally forces `authentic_color=1` for engine robustness when toggled — this must not change. The existing `authentic_color produces distinct response` test in `ReleaseTruthTest.cpp` must remain; it validates paths still differ when users explicitly enable 32k Color. [VERIFIED: `ctest --test-dir Builds -C Release -N`, `tests/RealtimeStressTest.cpp`, `tests/ReleaseTruthTest.cpp`]

**Primary recommendation:** Execute the three-wave plan already drafted (11-01 defaults/presets → 11-02 SAFE tests → 11-03 docs/tooltip) with zero DSP file changes; prove safety via `[release][safe]` assertions that mirror `HighFrequencyRingingDiagnosticsTest` config A thresholds at 48 kHz.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| APVTS `authentic_color` default | API / Backend (`ParameterLayout.cpp`) | — | JUCE `AudioParameterBool` third ctor arg sets fresh-instance default before any preset/state load |
| Factory preset `authentic_color` values | Database / Storage (embedded XML → BinaryData) | API (`FactoryPresets::applyEmbeddedXml`) | Presets overwrite APVTS on `setCurrentProgram`; XML source is `resources/presets/*.xml` |
| Wet reverb rate path selection | DSP / Backend (`SchroederTank32`) | API (parameter read in `PluginProcessor`) | `authenticColor` bool branches `processAuthentic()` vs `processHostRate()` — no Phase 11 DSP edits |
| Advanced drawer tooltip | Browser / Client (`AdvancedDrawer.cpp`) | — | User-facing experimental warning; label stays in `AdvancedDrawer.h` |
| SAFE automated proofs | CI / Validation (`ReleaseTruthTest.cpp`) | — | Release gate tests; chain-level HF proof via `GatedBloomChain` per D-06 |
| RC1 documentation | Static (`README.md`, `RELEASE_CHECKLIST.md`) | — | Honest product-facing default behavior |

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

- **D-01:** Phase 11 may only touch ParameterLayout.cpp, resources/presets/*.xml, ReleaseTruthTest.cpp, optional ParameterLayoutTest.cpp, AdvancedDrawer.cpp tooltip, README.md, docs/RELEASE_CHECKLIST.md
- **D-02:** All 8 factory presets ship authentic_color=0
- **D-03:** APVTS default authentic_color=false in ParameterLayout.cpp
- **D-04:** README and RELEASE_CHECKLIST get RC1 safety wording; no ProperSRC ship claims
- **D-05:** Keep "32k Color" label; tooltip adds experimental/off-by-default warning
- **D-06:** Extend ReleaseTruthTest.cpp with [release][safe] cases for SAFE-01/02/03
- **D-07:** Do not touch SchroederTank32, GatedBloomChain routing, WetOverdrive, r8brain/SRC, UI layout beyond tooltip

### D-01: Allowed file touch list

Phase 11 may modify **only** these paths:

| Path | Change |
|------|--------|
| `source/ParameterLayout.cpp` | `authentic_color` APVTS default `false` |
| `resources/presets/*.xml` (all 8) | `authentic_color` value `0` |
| `tests/ReleaseTruthTest.cpp` | New `[release][safe]` cases for SAFE-01/02/03 |
| `tests/ParameterLayoutTest.cpp` | Optional: assert `authentic_color` default off on fresh `PluginProcessor` |
| `source/ui/AdvancedDrawer.cpp` | Tooltip copy only — experimental/advanced warning |
| `README.md` | State 32k Color off by default for RC1 |
| `docs/RELEASE_CHECKLIST.md` | RC1 safety note + honest accumulator disclaimer when enabled |

**May read but not change:** `source/FactoryPresets.cpp` (loads embedded XML — presets fix is at XML source), `tests/PresetTest.cpp`, `tests/HighFrequencyRingingDiagnosticsTest.cpp` (reference for HF assertion patterns).

**Rebuild note:** After preset XML edits, BinaryData is regenerated on next CMake build — no manual BinaryData edits.

### D-02: Factory presets — all `authentic_color=0`

- **Decision:** All 8 factory presets ship `authentic_color=0`.
- **Current state:** 7 presets have `value="1"`; only `Hot_Clip.xml` is already `0`.
- **Rationale:** SAFE-02; presets are the primary user entry point after fresh load. Partial fix (APVTS only) still exposes whistle on preset recall.
- **Preserves:** All other preset parameter values unchanged — only flip `authentic_color`.

### D-03: APVTS default — `authentic_color=false`

- **Decision:** Change `ParameterLayout.cpp` line 56 from `true` → `false`.
- **Rationale:** SAFE-01; fresh plugin instance (no preset, no saved state) must use host-rate path.
- **Display name:** Keep APVTS name `"Authentic Color"` — UI label `"32k Color"` in `AdvancedDrawer.h` is the user-facing string (UI-04: no new controls).

### D-04: Documentation wording

**README.md:**
- Keep existing description of what 32k Color *does* when enabled.
- Add explicit RC1 line: **32k Color is off by default**; host-rate tank is the production path until ProperSRC passes acceptance gates (Phases 13–18).
- Do not claim ProperSRC is shipped or that accumulator path is fixed.

**docs/RELEASE_CHECKLIST.md:**
- Add **RC1 Safety Freeze** subsection under or beside existing 32k Color Truth (VERB-05).
- When enabled: keep honest accumulator-path description (current `processAuthentic` behavior).
- Add disclaimer: path is **experimental / not production-default** until TEST-11, DIAG-04, LAT-02, XFADE-01 pass (Phase 18 enablement).
- Do not update checklist to describe ProperSRC as shipped — that belongs to Phase 18.

**REQUIREMENTS.md:** Planner/executor may tick SAFE-01/02/03 on verification; no requirement text rewrite in Phase 11 unless acceptance wording is stale after implementation.

### D-05: UI label and tooltip

- **Label:** Keep `"32k Color"` on `colorToggle` — no rename, no new controls (INTEG-04).
- **Tooltip:** Update to experimental/advanced framing. Recommended copy (planner may tighten prose):
  - Lead: *Experimental — off by default until validated.*
  - Body: Retain existing technical description (32,768 Hz stepping, fixed delay table, 9-bit quantization, original software).
  - Close: *May exhibit HF artifacts at some host rates; host-rate reverb is the production default.*
- **No changes to:** `AdvancedDrawer.h` layout, `PluginEditor.cpp`, pedal layout, disabled Extended Stereo / Dirt OS toggles.

### D-06: Tests that prove the safety freeze

Extend `tests/ReleaseTruthTest.cpp` with `[release][safe]` cases:

1. **SAFE-01 — Fresh load default off:** Construct `PluginProcessor` with no `setStateInformation`; assert `authentic_color` APVTS raw value ≈ 0.
2. **SAFE-02 — All presets auth=0:** For each `setCurrentProgram(0..7)`, assert `authentic_color` ≈ 0. Complements existing preset/XML parity test — adds explicit SAFE assertion.
3. **SAFE-03 — Host-rate path no HF whistle at 48 kHz:** Render wet chain (or `GatedBloomChain` + default-off processor) with guitar-pluck or impulse fixture at 48 kHz, `authentic_color=false`, `distn=0`; assert tail metrics stay below existing host-rate ceilings (reuse thresholds from `HighFrequencyRingingDiagnosticsTest` config A / imaging band limits). Proves production default is clean — not a new tuning pass on authentic path.

**Regression gates:**
- Full Catch2 suite green (`ctest --test-dir Builds -C Release`).
- Existing `authentic_color produces distinct response` test **stays** — still validates paths differ when user explicitly enables 32k Color (diagnostic value for Phase 15+).
- `RealtimeStressTest` may continue forcing `authentic_color=1` for stress — that tests engine robustness when toggled, not RC1 default.

**No new test file** — keep SAFE cases in `ReleaseTruthTest.cpp` alongside release truth cases.

### D-07: Files and areas to avoid (conflict prevention)

| Avoid | Reason |
|-------|--------|
| `source/SchroederTank32.h` | No rewrite of `processAuthentic()` / accumulator — Phase 12–13 |
| `source/SchroederTank32DelayTable.h`, new `SchroederTankCore.*` | Phase 12 |
| Any r8brain / `FixedRateAdapter` / `Authentic32Mode` | Phase 13 |
| `source/GatedBloomChain.h`, `source/PluginProcessor.cpp` routing | Gate/send/dry/OD routing frozen |
| `source/WetOverdrive.*` | No tuning |
| `source/Fdn8Reverb.h`, `source/IReverbEngine.h` | Integration/SRC phases |
| `source/ui/*` except `AdvancedDrawer.cpp` tooltip | Avoid UI phase clash (Phase 9 shipped layout) |
| `CMakeLists.txt` (new deps), `.github/workflows/*` | No SRC dependency |
| Product name, version strings, git tags | User constraint |
| `HighFrequencyRingingDiagnosticsTest.cpp` threshold changes | Don't retune authentic-path acceptance — only add host-rate default proof |

### Claude's Discretion

- Exact tooltip prose and README/checklist sentence-level wording (within D-04/D-05 intent).
- Whether SAFE-01 lives in `ParameterLayoutTest.cpp` or only `ReleaseTruthTest.cpp` (at least one must cover it).
- HF assertion fixture choice (guitar pluck vs impulse) as long as it matches host-rate config A semantics at 48 kHz.

### Deferred Ideas (OUT OF SCOPE)

- **ProperSRC / r8brain integration** — Phase 13
- **SchroederTankCore extraction** — Phase 12
- **Engine crossfade on toggle** — Phase 16 (XFADE-01)
- **PDC / ADR-003 latency policy** — Phase 17
- **Re-enable `authentic_color=1` in factory presets** — Phase 18 after acceptance gates
- **Remove or demote legacy accumulator path** — post-ProperSRC validation (Phase 15+)
- **Multi-DAW smoke (TEST-07)** — still deferred from v1.0
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SAFE-01 | `authentic_color` (32k Color) defaults to off in parameter layout and factory presets | D-03: `ParameterLayout.cpp:56` flip `true`→`false`; D-02: all 8 XML presets to `value="0"`; SAFE-01 test in `ReleaseTruthTest.cpp` |
| SAFE-02 | All 8 factory presets ship with `authentic_color=0` until enablement phase passes acceptance gates | 7 presets currently `value="1"` [VERIFIED: grep]; flip XML only; SAFE-02 loops `setCurrentProgram(0..7)` |
| SAFE-03 | Production path uses host-rate Schroeder tank; broken accumulator path is not user-facing default | Default-off APVTS + presets route to `processHostRate()`; SAFE-03 chain render at 48 kHz with config A thresholds |
</phase_requirements>

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8 (submodule) | APVTS, `AudioParameterBool`, BinaryData presets | Project stack [VERIFIED: `CMakeLists.txt`, `.claude/.cursor/rules/gsd-project.md`] |
| Catch2 | 3.8.1 | `[release][safe]` assertions | Existing test infra via CPM [VERIFIED: `cmake/Tests.cmake`] |
| CMake / ctest | 3.25+ | Build, preset embed, test discovery | `catch_discover_tests(Tests)` [VERIFIED: `cmake/Tests.cmake`] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| `ChainTestHelpers.h` | in-repo | `goertzelPower`, `rms` | SAFE-03 tail spectral metrics |
| `GatedBloomChain` | in-repo | Wet chain render without full processor | SAFE-03 per D-06 (config A semantics) |
| `FactoryPresets` | in-repo | `kNumPresets`, `setCurrentProgram` | SAFE-02 preset recall loop |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| GatedBloomChain chain render (SAFE-03) | Full `PluginProcessor` wet path | Chain-level matches HF test config A; CONTEXT explicitly allows chain proof; faster, isolated |
| `ParameterLayoutTest.cpp` for SAFE-01 | `ReleaseTruthTest.cpp` only | Either satisfies D-06; release truth tag `[release][safe]` preferred for CI gate |

**Installation:** None — no new packages this phase.

**Version verification:** Catch2 3.8.1 pinned in `cmake/Tests.cmake` via `CPMAddPackage("gh:catchorg/Catch2@3.8.1")`. [VERIFIED: codebase]

## Package Legitimacy Audit

> Phase 11 installs **no external packages**. Existing dependencies only.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| — | — | — | — | — | — | N/A |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

## Architecture Patterns

### System Architecture Diagram

```
User loads plugin / recalls preset
         │
         ▼
┌─────────────────────┐
│  APVTS state        │◄── ParameterLayout default (authentic_color=false)
│  (PluginProcessor)  │◄── Factory preset XML (authentic_color=0)
└─────────┬───────────┘
          │ smoothedBank.getNextAuthenticColorTarget()
          ▼
┌─────────────────────┐
│  GatedBloomChain    │  (read-only routing — no Phase 11 edits)
└─────────┬───────────┘
          │ authenticColor bool
          ▼
     ┌────┴────┐
     │ branch  │
     ▼         ▼
processHostRate()   processAuthentic()
(PRODUCTION         (EXPERIMENTAL —
 default OFF)        accumulator+hold,
                     HF whistle risk)
```

### Recommended Project Structure

No structural changes. Edits confined to:

```
source/ParameterLayout.cpp          # SAFE-01 APVTS default
resources/presets/*.xml             # SAFE-02 preset values (8 files)
tests/ReleaseTruthTest.cpp          # SAFE-01/02/03 automated proofs
source/ui/AdvancedDrawer.cpp        # D-05 tooltip only
README.md                           # D-04 RC1 default wording
docs/RELEASE_CHECKLIST.md           # D-04 RC1 Safety Freeze subsection
```

### Pattern 1: JUCE AudioParameterBool default

**What:** Third constructor argument sets APVTS default for fresh instances.
**When to use:** SAFE-01 — single-line change at registration site.
**Example:**

```cpp
// Source: source/ParameterLayout.cpp (current — must change)
layout.add (std::make_unique<juce::AudioParameterBool> (
    juce::ParameterID { authenticColor, 1 }, "Authentic Color", false)); // was true
```

[VERIFIED: codebase — `source/ParameterLayout.cpp:55-56`]

### Pattern 2: Factory preset XML → BinaryData embed

**What:** CMake `juce_add_binary_data(SendBloomPresets SOURCES resources/presets/*.xml)` embeds XML at build time; `FactoryPresets::applyEmbeddedXml` loads via `apvts.replaceState`.
**When to use:** SAFE-02 — edit source XML, rebuild Tests target to refresh embedded blobs for `setCurrentProgram` tests.
**Example:**

```xml
<!-- Source: resources/presets/Sparkle_Verb.xml (representative — 7 files need flip) -->
<PARAM id="authentic_color" value="0"/>
```

[VERIFIED: `CMakeLists.txt:67-77`, `source/FactoryPresets.cpp`]

### Pattern 3: Release truth test tagging

**What:** Catch2 `TEST_CASE` with tags `[release][safe]` for RC1 safety gate; co-located with `[release][verb]`, `[release][preset]` cases.
**When to use:** All three SAFE requirements in one file per D-06.
**Example:**

```cpp
// Source: tests/ReleaseTruthTest.cpp (pattern from existing cases)
TEST_CASE ("fresh plugin load defaults authentic_color off", "[release][safe]")
{
    using namespace sendbloom::ParameterIDs;
    sendbloom::PluginProcessor plugin;
    REQUIRE (*plugin.getAPVTS().getRawParameterValue (authenticColor)
             == Catch::Approx (0.0f).margin (1e-4f));
}
```

[VERIFIED: `tests/ReleaseTruthTest.cpp` existing patterns]

### Pattern 4: SAFE-03 HF proof via GatedBloomChain config A

**What:** Render 24000 samples at 48 kHz with guitar-pluck fixture; `authenticColor=false`, `distn=0`, `darkMix=0`; measure tail Goertzel at 14825 Hz and narrowband dominance.
**When to use:** SAFE-03 — proves production default path clean without modifying HF diagnostic thresholds.
**Constants to reuse from `HighFrequencyRingingDiagnosticsTest.cpp`:**

| Constant | Value | Purpose |
|----------|-------|---------|
| `kSampleRate` | 48000.0 | Host rate |
| `kRenderSamples` | 24000 | Render length |
| `kTailStart` | 19200 | Tail slice start |
| `kTailCount` | 2400 | Tail slice length |
| `kImagingBandPeakRmsMax` | 0.0022f | 14825 Hz RMS ceiling |
| `kNarrowbandDominanceMaxRatio` | 10.0f | Peak vs neighbor ratio ceiling |

[VERIFIED: `tests/HighFrequencyRingingDiagnosticsTest.cpp:17-28`, `11-02-PLAN.md`]

### Anti-Patterns to Avoid

- **APVTS-only fix without presets:** Users selecting factory presets would still get `authentic_color=1` from embedded XML — partial fix fails SAFE-02.
- **Modifying `SchroederTank32::processAuthentic()`:** Out of scope (D-07); ProperSRC fix is Phases 12–13.
- **Retuning HF thresholds in `HighFrequencyRingingDiagnosticsTest.cpp`:** Masking authentic-path failures; SAFE-03 reuses ceilings, host path should pass with margin.
- **Removing `authentic_color produces distinct response` test:** Still needed to prove paths differ when explicitly enabled (Phase 15+ diagnostics).
- **Manual BinaryData edits:** Always rebuild after XML changes.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Tail HF spectral measurement | Custom FFT harness | `sendbloom::test::goertzelPower` in `ChainTestHelpers.h` | Already used by HF ringing suite; deterministic, fast |
| Preset loading in tests | Raw XML parse loops | `PluginProcessor::setCurrentProgram` + `FactoryPresets::kNumPresets` | Matches production recall path |
| Parameter default assertion | Custom layout introspection | `getAPVTS().getRawParameterValue(authenticColor)` | Standard JUCE APVTS read pattern in existing tests |
| Imaging whistle detection | New metric inventing | Reuse 14825 Hz Goertzel + 0.0022f ceiling from config A | Established acceptance contract |

**Key insight:** Phase 11 changes *which path is default*, not *how paths work*. All DSP behavior already exists and is tested — research confirms the host-rate path already passes HF gates when `authenticColor=false`.

## Common Pitfalls

### Pitfall 1: Stale BinaryData after preset XML edit

**What goes wrong:** Tests pass locally against old embedded `value="1"` blobs if Tests target not rebuilt.
**Why it happens:** `juce_add_binary_data` generates C++ at configure/build time.
**How to avoid:** Always `cmake --build Builds --config Release --target Tests` after XML edits before running SAFE-02.
**Warning signs:** SAFE-02 fails on presets but grep shows XML already `value="0"`.

### Pitfall 2: Partial preset fix (7 of 8)

**What goes wrong:** One preset still ships `authentic_color=1`; user hits whistle on that preset recall.
**Why it happens:** `Hot_Clip.xml` already `0`; easy to miss the other seven identical lines.
**How to avoid:** Grep `authentic_color` across `resources/presets/` — expect 8× `value="0"`, 0× `value="1"`.
**Warning signs:** SAFE-02 fails on specific preset index.

### Pitfall 3: Breaking existing authentic-path tests

**What goes wrong:** Changing defaults could cause tests that assume `authentic_color=1` on fresh load to fail.
**Why it happens:** Most tests explicitly set parameters; `RealtimeStressTest` sets `authentic_color=1` itself.
**How to avoid:** Run full `ctest --test-dir Builds -C Release`; do not change `RealtimeStressTest.cpp`.
**Warning signs:** Only failures in tests that don't set `authenticColor` explicitly — audit those cases.

### Pitfall 4: Over-scoping SAFE-03 to full PluginProcessor

**What goes wrong:** Unnecessary coupling to input gain, dry/wet mix, gate state — harder to debug HF failures.
**Why it happens:** Misreading SAFE-03 as requiring end-to-end plugin render.
**How to avoid:** Use `GatedBloomChain::processSample` render matching HF test `renderChain` for config A per D-06.
**Warning signs:** Test flakiness from gate envelope or dry path bleed.

### Pitfall 5: Claiming ProperSRC is fixed in docs

**What goes wrong:** User/legal expectation mismatch — accumulator path still broken when enabled.
**Why it happens:** Conflating "off by default" with "path fixed".
**How to avoid:** D-04 wording: experimental when enabled; production default is host-rate until Phase 18 gates pass.
**Warning signs:** README/checklist mentions r8brain or bandlimited SRC as shipped.

## Code Examples

### SAFE-02 preset loop (extends existing pattern)

```cpp
// Source: tests/ReleaseTruthTest.cpp — extends setCurrentProgram loads embedded XML preset values pattern
TEST_CASE ("all factory presets recall authentic_color off", "[release][safe]")
{
    using namespace sendbloom::ParameterIDs;

    for (int preset = 0; preset < sendbloom::FactoryPresets::kNumPresets; ++preset)
    {
        sendbloom::PluginProcessor plugin;
        plugin.setCurrentProgram (preset);
        INFO ("preset index " << preset);
        REQUIRE (*plugin.getAPVTS().getRawParameterValue (authenticColor)
                 == Catch::Approx (0.0f).margin (1e-4f));
    }
}
```

[VERIFIED: pattern from `tests/ReleaseTruthTest.cpp:297-344`]

### SAFE-03 render chain (from HF diagnostics)

```cpp
// Source: tests/HighFrequencyRingingDiagnosticsTest.cpp renderChain + config A
GatedBloomChain chain;
chain.prepare (48000.0, 512);
const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
// ... guitar pluck input ...
wet[i] = chain.processSample (input[i], env, rt60, 0.0f, false, // authenticColor=false
                              0.0f, 1.0f, true, -40.0f);
```

[VERIFIED: `tests/HighFrequencyRingingDiagnosticsTest.cpp:105-124`, config `kMatrixConfigs[0]`]

### Current broken default (must change)

```cpp
// source/ParameterLayout.cpp:55-56 — CURRENT STATE
layout.add (std::make_unique<juce::AudioParameterBool> (
    juce::ParameterID { authenticColor, 1 }, "Authentic Color", true));
```

[VERIFIED: codebase read]

### Preset XML current state (7 need flip)

| Preset file | Current `authentic_color` |
|-------------|---------------------------|
| Sparkle_Verb.xml | 1 |
| Cut_Sample_Gate.xml | 1 |
| Spacerock_Burn.xml | 1 |
| Dry_Dub_Sends.xml | 1 |
| Dark_Bloom.xml | 1 |
| Firm_Pressure.xml | 1 |
| Gated_Room.xml | 1 |
| Hot_Clip.xml | 0 |

[VERIFIED: grep `resources/presets/*.xml`]

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `authentic_color=true` APVTS default | Must become `false` for RC1 | Phase 11 | Fresh load uses host-rate path |
| 7/8 presets `authentic_color=1` | All 8 → `0` | Phase 11 | Preset recall safe |
| Tooltip describes feature positively | Experimental/off-by-default warning | Phase 11 | Honest UX for Advanced drawer |
| No `[release][safe]` tests | Three SAFE cases in ReleaseTruthTest | Phase 11 | Automated RC1 gate |

**Deprecated/outdated:**
- Treating accumulator `processAuthentic()` as production-ready SRC — superseded by v2.0 ProperSRC plan (Phases 13–18); RC1 ships with path disabled by default only.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| — | (none) | — | All critical claims verified against codebase this session |

**If this table is empty:** All claims in this research were verified or cited — no user confirmation needed.

## Open Questions

1. **Should SAFE-01 also live in `ParameterLayoutTest.cpp`?**
   - What we know: D-06 allows either location; `ReleaseTruthTest` is required.
   - What's unclear: Redundant coverage vs. layout-unit isolation.
   - Recommendation: Minimum — `ReleaseTruthTest` only; optional duplicate in `ParameterLayoutTest` if executor wants layout-file-local assertion.

2. **Will SAFE-03 pass with authentic ceilings on host path?**
   - What we know: HF diagnostic test shows host path has lower HF energy than authentic; ceilings (`0.0022f`, `10.0f`) were tuned for authentic acceptance.
   - What's unclear: Margin at 14825 Hz for config A guitar pluck — not explicitly asserted today.
   - Recommendation: Run SAFE-03 during 11-02 execution; if host path margin is tight, do not retune ceilings — investigate render parity with HF test instead.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | Build + ctest | ✓ | 4.3.3 | — |
| ctest | Test discovery/run | ✓ | 4.3.3 | Run `./Builds/Tests` directly |
| C++20 compiler | Plugin + Tests build | ✓ (assumed — Builds dir exists) | — | — |
| Catch2 3.8.1 | Test framework | ✓ (via CPM at configure) | 3.8.1 | — |
| JUCE 8 submodule | Plugin target | ✓ | 8 | — |
| Builds/ directory | Incremental build | ✓ | — | `cmake -B Builds -DCMAKE_BUILD_TYPE=Release` |

**Missing dependencies with no fallback:** none identified

**Missing dependencies with fallback:** none

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 |
| Config file | `cmake/Tests.cmake` (CPM + `catch_discover_tests`) |
| Quick run command | `ctest --test-dir Builds -C Release -R 'release.*safe' --output-on-failure` |
| Full suite command | `ctest --test-dir Builds -C Release --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SAFE-01 | Fresh load `authentic_color` ≈ 0 | unit/integration | `ctest --test-dir Builds -C Release -R 'fresh plugin load defaults authentic_color off' --output-on-failure` | ❌ Wave 0 (add in 11-02) |
| SAFE-02 | All 8 presets recall `authentic_color` ≈ 0 | integration | `ctest --test-dir Builds -C Release -R 'all factory presets recall authentic_color off' --output-on-failure` | ❌ Wave 0 (add in 11-02) |
| SAFE-03 | Host-rate path no HF imaging at 48 kHz | integration/DSP | `ctest --test-dir Builds -C Release -R 'host-rate default path no HF imaging' --output-on-failure` | ❌ Wave 0 (add in 11-02) |
| Regression | Full suite green (132+ tests) | regression | `ctest --test-dir Builds -C Release --output-on-failure` | ✅ |

### Sampling Rate

- **Per task commit:** `ctest --test-dir Builds -C Release -R 'release.*safe' --output-on-failure`
- **Per wave merge:** `ctest --test-dir Builds -C Release --output-on-failure`
- **Phase gate:** Full suite green before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] `tests/ReleaseTruthTest.cpp` — three `[release][safe]` TEST_CASE blocks for SAFE-01/02/03
- [ ] Rebuild Tests target after preset XML edits (refreshes BinaryData for SAFE-02)
- [ ] No new framework install needed

## Security Domain

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|------------------|
| V2 Authentication | no | — |
| V3 Session Management | no | — |
| V4 Access Control | no | — |
| V5 Input Validation | no (parameter bounds unchanged) | Existing JUCE `AudioParameterBool` 0/1 range |
| V6 Cryptography | no | — |

Phase 11 is a local audio plugin default-behavior change with no network, auth, or secrets surface. `security_enforcement` applies at project level but no new threat vectors are introduced.

### Known Threat Patterns for JUCE/C++ plugin

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malicious preset XML injection | Tampering | JUCE `parseXML` + APVTS `replaceState` with typed params — unchanged |
| Supply-chain package install | Spoofing | No new packages this phase |

## Project Constraints (from .cursor/rules/)

From `.claude/.cursor/rules/gsd-project.md`:

- **Tech stack:** JUCE 8, C++20, CMake, Catch2, pluginval — pamplejuce-style repository
- **Realtime safety:** Zero heap allocation, locks, logging, file I/O, or UI access in `processBlock()` after `prepare()` — Phase 11 does not touch `processBlock`
- **Parameter stability:** APVTS IDs immutable after release — do not rename `authentic_color` ID or display name
- **GSD workflow:** Execute via planned phase workflow; direct edits outside GSD discouraged

## Sources

### Primary (HIGH confidence)

- `source/ParameterLayout.cpp` — current `authentic_color` default `true` at line 56
- `resources/presets/*.xml` — 7× `value="1"`, 1× `value="0"`
- `tests/ReleaseTruthTest.cpp` — release truth patterns, preset parity test
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — config A constants, renderChain, HF thresholds
- `source/SchroederTank32.h` — `processAuthentic()` vs `processHostRate()` branch
- `CMakeLists.txt` — BinaryData preset embedding
- `.planning/phases/11-rc1-safety-freeze/11-CONTEXT.md` — locked decisions D-01 through D-07

### Secondary (MEDIUM confidence)

- `.planning/phases/11-rc1-safety-freeze/11-02-PLAN.md` — SAFE-03 implementation constants and acceptance criteria
- `.planning/PROJECT.md` — v1 diagnosis of 14–15 kHz imaging root cause

### Tertiary (LOW confidence)

- None used for implementation decisions

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — no new packages; existing Catch2/JUCE patterns verified in repo
- Architecture: HIGH — parameter → chain → tank branch traced in source
- Pitfalls: HIGH — current defaults and preset state confirmed by grep; BinaryData rebuild requirement documented in CONTEXT

**Research date:** 2026-07-08
**Valid until:** 2026-08-08 (stable config-only phase)
