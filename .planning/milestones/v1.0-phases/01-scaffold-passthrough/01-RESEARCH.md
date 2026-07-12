# Phase 1: Scaffold & Passthrough - Research

**Researched:** 2026-07-06
**Domain:** JUCE 8 audio plugin scaffold (CMake, pamplejuce pattern, CI, pluginval)
**Confidence:** HIGH

## Summary

Phase 1 bootstraps SendBloom as a greenfield pamplejuce-style JUCE 8 repository with AU + VST3 passthrough, Catch2 smoke tests, and a three-platform GitHub Actions matrix running pluginval at strictness 5. ADR-001 locks the base to [sudara/pamplejuce](https://github.com/sudara/pamplejuce) (MIT); the local shallow clone at `.planning/repo-samples/sudara-pamplejuce/` confirms CMake structure, workflow layout, and test patterns even though submodules (`JUCE/`, `cmake/`, `modules/`) are not populated in the sample.

The planner should **copy pamplejuce structure into the repo root** (not maintain a git fork relationship), then customize metadata for SendBloom / Niko Audio Labs / NkMo / SbLm, restrict `FORMATS` to `AU VST3`, implement explicit passthrough in `processBlock`, and adapt CI: strictness 5, skip code-signing/notarization until secrets exist, disable optional IPP/CLAP/Standalone/AUv3 for this phase. Passthrough DSP is trivial (buffer unchanged or `copyFrom` self); pamplejuce's default empty `processBlock` already passes audio but explicit passthrough + a Catch2 identity test makes SCAF-02 verifiable.

**Primary recommendation:** Initialize from pamplejuce template files, pin JUCE 8 via submodule, set `FORMATS AU VST3`, metadata codes `NkMo`/`SbLm`, override C++ standard to **C++20** per PROJECT.md, and ship a slimmed CI workflow validating VST3 with pluginval strictness 5 on macOS/Windows/Linux.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions

All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP success criteria, PROJECT.md constraints (JUCE 8, C++20, CMake, Catch2, pamplejuce pattern), ADR-001 (pamplejuce base), and `.planning/repo-samples/` for reference patterns. Namespace: `sendbloom`. Manufacturer placeholder: Niko Audio Labs.

### Claude's Discretion

All implementation choices are at Claude's discretion — pure infrastructure phase. Use ROADMAP success criteria, PROJECT.md constraints (JUCE 8, C++20, CMake, Catch2, pamplejuce pattern), ADR-001 (pamplejuce base), and `.planning/repo-samples/` for reference patterns. Namespace: `sendbloom`. Manufacturer placeholder: Niko Audio Labs.

### Deferred Ideas (OUT OF SCOPE)

None — infrastructure phase.
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| SCAF-01 | Repository initialized from pamplejuce-style scaffold (JUCE 8, CMake, C++20) | ADR-001 + pamplejuce `CMakeLists.txt`, `cmake-includes` submodules, CPM Catch2 3.8.1 pattern [CITED: sudara/cmake-includes/Tests.cmake] |
| SCAF-02 | Empty passthrough plugin builds as AU and VST3 | `juce_add_plugin` with `FORMATS AU VST3`; passthrough `processBlock`; mono/stereo bus layout [CITED: JUCE CMake API] |
| SCAF-03 | GitHub Actions build matrix (macOS, Windows, Linux) passes on passthrough | pamplejuce `build_and_test.yml` matrix pattern; Linux Xvfb + JUCE deps [VERIFIED: repo-samples] |
| SCAF-04 | pluginval runs in CI at strictness 5 initially, raised to 10 before release | `--strictness-level 5` in workflow env; VST3 validation primary [CITED: Tracktion/pluginval README] |
| SCAF-05 | Plugin metadata uses SendBloom / NkMo / SbLm codes (placeholders verified before ship) | `PRODUCT_NAME`, `PLUGIN_MANUFACTURER_CODE NkMo`, `PLUGIN_CODE SbLm`, `COMPANY_NAME` [CITED: JUCE CMake API] |
</phase_requirements>

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Plugin DSP (passthrough) | API / Backend (audio thread) | — | `processBlock` runs in host audio callback; no browser/SSR involvement |
| Plugin metadata (name, codes) | Build system (CMake) | Host runtime | Codes baked at compile via `juce_add_plugin`; DAW reads bundle/plist |
| Unit tests (Catch2) | Build / CI | — | Offline test executable links SharedCode; not shipped |
| pluginval validation | CI runner | — | Headless binary exercises built VST3/AU artifacts post-build |
| GitHub Actions matrix | CI/CD | — | Cross-platform compile + test orchestration |
| DAW load smoke test | Developer workstation | — | Manual gate documented in ROADMAP; not automatable in CI without DAW license |

## Standard Stack

### Core

| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| JUCE | 8.x (pin tag in submodule) | Audio plugin framework, AU/VST3 targets | Industry standard; ADR-001 + PROJECT.md mandate JUCE 8 [CITED: JUCE CMake API] |
| CMake | ≥ 3.25 | Cross-platform build | pamplejuce minimum; verified locally 4.3.3 [VERIFIED: local env] |
| Catch2 | 3.8.1 (via CPM) | Unit test framework | pamplejuce `Tests.cmake` pin [CITED: sudara/cmake-includes/Tests.cmake] |
| CPM.cmake | via cmake-includes submodule | Fetch Catch2 at configure time | pamplejuce standard dependency manager [VERIFIED: repo-samples] |
| pluginval | 1.0.3+ (CI download) | Plugin binary validation | Tracktion standard; pamplejuce CI uses v1.0.3 zip [VERIFIED: repo-samples] |

### Supporting

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| Ninja | latest (CI) | Fast builds | pamplejuce CI matrix; optional locally |
| sccache | via GHA action | CI compile cache | pamplejuce workflow; optional for Phase 1 |
| melatonin_inspector | submodule (optional) | UI debug | Keep from pamplejuce for dev; strip from minimal scaffold if desired |
| Intel IPP | optional | SIMD DSP | **Disable for Phase 1** — adds CI complexity; not needed for passthrough |

### Alternatives Considered

| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| pamplejuce base | anthonyalfimov/JUCE-CMake-Plugin-Template | Lacks Catch2 + pluginval CI wiring; ADR-001 rejected as base |
| pamplejuce base | ChowDSP JUCEPluginTemplate | Vendor workflow overhead; ADR-001 rejected |
| CPM Catch2 | Git submodule Catch2 | pamplejuce pattern is CPM; less submodule maintenance |
| pluginval strictness 5 | strictness 10 in Phase 1 | SCAF-04 explicitly requires 5 now, 10 before release (Phase 10) |

**Installation:**

```bash
# After copying pamplejuce scaffold into repo root:
git submodule update --init --recursive   # JUCE, cmake-includes, optional modules
cmake -B Builds -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --config Release
ctest --test-dir Builds --verbose --output-on-failure
```

**Version verification:**

```bash
# Catch2 pin (from upstream Tests.cmake):
# CPMAddPackage("gh:catchorg/Catch2@3.8.1")

# JUCE: pin explicitly in .gitmodules branch or checkout tag after submodule init
# anthonyalfimov template uses LIB_JUCE_TAG "8.0.12" as reference pin [VERIFIED: repo-samples]

# pluginval release (pamplejuce CI):
# https://github.com/Tracktion/pluginval/releases/download/v1.0.3/pluginval_${{ runner.os }}.zip
```

## Package Legitimacy Audit

> Phase installs no npm/PyPI packages. Dependencies are git submodules and CPM-fetched CMake packages.

| Package | Registry | Age | Downloads | Source Repo | Verdict | Disposition |
|---------|----------|-----|-----------|-------------|---------|-------------|
| catchorg/Catch2 | GitHub (CPM) | mature | N/A | github.com/catchorg/Catch2 | OK | Approved — official Catch2 org |
| juce-framework/JUCE | GitHub (submodule) | mature | N/A | github.com/juce-framework/JUCE | OK | Approved — official JUCE |
| sudara/cmake-includes | GitHub (submodule) | mature | N/A | github.com/sudara/cmake-includes | OK | Approved — pamplejuce maintainer |
| Tracktion/pluginval | GitHub releases (binary) | mature | N/A | github.com/Tracktion/pluginval | OK | Approved — official Tracktion tool |

**Packages removed due to [SLOP] verdict:** none
**Packages flagged as suspicious [SUS]:** none

*Catch2 on npm does not exist (`catch2` npm package is SLOP); use CPM `gh:catchorg/Catch2@3.8.1` only.*

## Architecture Patterns

### System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                        Developer / CI                            │
│  git push → GitHub Actions (macOS | Windows | Linux)              │
└────────────────────────────┬────────────────────────────────────┘
                             │
         ┌───────────────────┼───────────────────┐
         ▼                   ▼                   ▼
   ┌───────────┐      ┌───────────┐      ┌───────────┐
   │  cmake    │      │  cmake    │      │  cmake    │
   │  configure│      │  configure│      │  configure│
   └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
         │                   │                   │
         ▼                   ▼                   ▼
   ┌───────────┐      ┌───────────┐      ┌───────────┐
   │ JUCE 8    │      │ JUCE 8    │      │ JUCE 8    │
   │ SharedCode│      │ SharedCode│      │ SharedCode│
   │ + AU/VST3 │      │ + VST3    │      │ + VST3    │
   └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
         │                   │                   │
         ▼                   ▼                   ▼
   ┌───────────┐      ┌───────────┐      ┌───────────┐
   │ ctest     │      │ ctest     │      │ ctest     │
   │ (Catch2)  │      │ (Catch2)  │      │ (Catch2)  │
   └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
         │                   │                   │
         ▼                   ▼                   ▼
   ┌───────────┐      ┌───────────┐      ┌───────────┐
   │ pluginval │      │ pluginval │      │ pluginval │
   │ VST3 + AU │      │ VST3      │      │ VST3      │
   │ strict 5  │      │ strict 5  │      │ strict 5  │
   └───────────┘      └───────────┘      └───────────┘

┌─────────────────────────────────────────────────────────────────┐
│                     Host DAW (manual smoke)                      │
│  Audio In → SendBloom AU/VST3 → processBlock (passthrough) → Out │
└─────────────────────────────────────────────────────────────────┘
```

### Recommended Project Structure

```
SendBloom/                          # repo root (greenfield — copy from pamplejuce)
├── CMakeLists.txt                  # PROJECT_NAME, juce_add_plugin, SharedCode
├── VERSION                         # semver propagated to plugin
├── cmake/                          # submodule: sudara/cmake-includes
│   ├── Tests.cmake                 # Catch2 + ctest
│   ├── SharedCodeDefaults.cmake    # override cxx_std_20 for SendBloom
│   └── GitHubENV.cmake             # exports PRODUCT_NAME to CI .env
├── JUCE/                           # submodule: juce-framework/JUCE
├── source/
│   ├── PluginProcessor.h/.cpp      # namespace sendbloom; passthrough DSP
│   └── PluginEditor.h/.cpp         # minimal UI (version label or blank)
├── tests/
│   ├── Catch2Main.cpp
│   ├── PluginBasics.cpp            # name + passthrough identity test
│   └── helpers/test_helpers.h
├── .github/workflows/
│   └── build_and_test.yml          # matrix; strictness 5; no signing Phase 1
├── .clang-format
└── .gitmodules
```

### Pattern 1: pamplejuce SharedCode + juce_add_plugin

**What:** INTERFACE library holds all plugin sources; `juce_add_plugin` creates format targets; Tests links SharedCode.
**When to use:** Always — this is the pamplejuce ODR-safe pattern.
**Example:**

```cmake
# Source: https://github.com/sudara/pamplejuce/blob/main/CMakeLists.txt
# SendBloom customizations shown inline

set(PROJECT_NAME "SendBloom")
set(PRODUCT_NAME "SendBloom")
set(COMPANY_NAME "Niko Audio Labs")
set(BUNDLE_ID "com.nikoaudiolabs.sendbloom")
set(FORMATS AU VST3)   # Phase 1: no Standalone, AUv3, CLAP

add_subdirectory(JUCE)

juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    PLUGIN_MANUFACTURER_CODE NkMo
    PLUGIN_CODE SbLm
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PRODUCT_NAME}"
    COPY_PLUGIN_AFTER_BUILD TRUE)

add_library(SharedCode INTERFACE)
file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})
target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)
include(Tests)
```

[CITED: https://github.com/juce-framework/JUCE/blob/develop/docs/CMake%20API.md]

### Pattern 2: Explicit Passthrough processBlock

**What:** Copy input channels to output unchanged; clear extra outputs; no allocation.
**When to use:** SCAF-02; baseline before any DSP in later phases.
**Example:**

```cpp
// Source: JUCE AudioProcessor pattern + SendBloom requirement
namespace sendbloom {

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto numInput = getTotalNumInputChannels();
    const auto numOutput = getTotalNumOutputChannels();

    for (int ch = numInput; ch < numOutput; ++ch)
        buffer.clear(ch, 0, buffer.getNumSamples());

    // Passthrough: samples already in buffer; no in-place mutation needed.
    // Optional explicit copy for clarity when input/output channel counts match:
    // for (int ch = 0; ch < numInput; ++ch)
    //     buffer.copyFrom(ch, 0, buffer, ch, 0, buffer.getNumSamples());
}

} // namespace sendbloom
```

### Pattern 3: pluginval in CI (strictness 5)

**What:** Download release binary, validate VST3 artifact, fail build on exit code 1.
**When to use:** SCAF-03, SCAF-04.
**Example:**

```yaml
# Source: pamplejuce build_and_test.yml + SCAF-04 override
env:
  STRICTNESS_LEVEL: 5

- name: Pluginval
  run: |
    curl -LO "https://github.com/Tracktion/pluginval/releases/download/v1.0.3/pluginval_${{ runner.os }}.zip"
    7z x pluginval_${{ runner.os }}.zip
    ${{ matrix.pluginval-binary }} \
      --strictness-level ${{ env.STRICTNESS_LEVEL }} \
      --verbose \
      --validate "${{ env.VST3_PATH }}"
```

[CITED: https://github.com/Tracktion/pluginval/blob/develop/README.md]

### Pattern 4: Catch2 Passthrough Identity Test

**What:** Feed known samples through offline `processBlock`, assert bit-identical output.
**When to use:** Phase completion gate per ROADMAP.
**Example:**

```cpp
// Source: pamplejuce test pattern + DSP verification
#include <catch2/catch_test_macros.hpp>
#include <PluginProcessor.h>

TEST_CASE("Passthrough preserves audio", "[dsp][passthrough]")
{
    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay(48000.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            buffer.setSample(ch, i, std::sin(0.01f * static_cast<float>(i)));

    auto expected = buffer;
    juce::MidiBuffer midi;
    plugin.processBlock(buffer, midi);

    for (int ch = 0; ch < 2; ++ch)
        for (int i = 0; i < 512; ++i)
            REQUIRE(buffer.getSample(ch, i) == expected.getSample(ch, i));
}
```

### Anti-Patterns to Avoid

- **Forking pamplejuce as git remote:** ADR-001 means copy template into SendBloom repo; avoid ongoing fork sync burden unless desired later.
- **Leaving pamplejuce demo names:** Fails SCAF-05 and legal guardrails; grep for Pamplejuce, Pamp, P001, Rainger, Reverb-X, Igor after scaffold.
- **pluginval strictness 10 in Phase 1:** Violates SCAF-04; pamplejuce defaults to 10 — must lower.
- **Requiring code signing in CI before secrets exist:** pamplejuce macOS packaging steps fail without Apple certs; gate with `if: secrets.DEV_ID_APP_CERT != ''` or comment out for Phase 1.
- **AU-only pluginval on Linux/Windows CI:** AU builds only on macOS; validate VST3 on all platforms, AU on macOS only [CITED: anthonyalfimov Validation.yml comment on pluginval issue #39].
- **C++23 from SharedCodeDefaults:** PROJECT.md requires C++20; override `target_compile_features(SharedCode INTERFACE cxx_std_20)`.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| CMake plugin target wiring | Custom Xcode/Projucer project | `juce_add_plugin` + pamplejuce SharedCode | AU/VST3 bundle layout, plist, module linking are error-prone |
| Test runner infrastructure | Custom test main | Catch2 v3 + `catch_discover_tests` | pamplejuce `Tests.cmake` already wires ctest/Xcode schemes |
| Plugin binary validation | Ad-hoc load scripts | pluginval CLI | Industry standard; strictness levels map to host compatibility |
| Dependency fetching for Catch2 | Vendor copy in repo | CPM `gh:catchorg/Catch2@3.8.1` | Pin version, cache-friendly, matches pamplejuce |
| CI cross-platform matrix | Per-OS custom scripts | Adapt pamplejuce `build_and_test.yml` | Linux deps, Xvfb, sccache already solved |
| Four-char manufacturer/plugin IDs | Random codes each build | Fixed `NkMo` / `SbLm` in CMake | JUCE defaults random PLUGIN_CODE each configure — breaks AU stability |

**Key insight:** Audio plugin scaffolding looks simple but AU bundle structure, VST3 factory exports, and cross-platform CI are deceptively complex — pamplejuce exists precisely to avoid relearning these failures.

## Common Pitfalls

### Pitfall 1: CI Fails Without Code Signing Secrets

**What goes wrong:** pamplejuce macOS workflow runs codesign, notarize, pkgbuild — all fail without GitHub secrets.
**Why it happens:** Template assumes production release pipeline.
**How to avoid:** Disable or `if:`-guard signing/notarization/packaging steps for Phase 1; keep build + test + pluginval only.
**Warning signs:** `codesign: no identity found` in GHA logs.

### Pitfall 2: pluginval AU Validation on CI Runners

**What goes wrong:** pluginval AU tests fail on headless GHA macOS even when plugin is valid locally.
**Why it happens:** AU hosting environment limitations on CI [CITED: pluginval issue #39].
**How to avoid:** CI pluginval targets VST3 on all OSes; AU validation deferred to local DAW smoke + later phases.
**Warning signs:** AU pass locally, fail only in CI.

### Pitfall 3: Linux pluginval Needs Display

**What goes wrong:** pluginval hangs or crashes on Linux CI without X server.
**Why it happens:** GUI subsystem initialization even in headless mode.
**How to avoid:** Set `DISPLAY: :0`, start `Xvfb` before pluginval (pamplejuce pattern).
**Warning signs:** Linux-only CI failure at Pluginval step.

### Pitfall 4: Submodule Not Initialized

**What goes wrong:** `add_subdirectory(JUCE)` fails; missing `cmake/` modules.
**Why it happens:** Shallow clone without `git submodule update --init --recursive`.
**How to avoid:** Document in README; GHA checkout with `submodules: recursive`.
**Warning signs:** CMake error "JUCE/CMakeLists.txt not found".

### Pitfall 5: GarageBand-Incompatible Plugin Codes

**What goes wrong:** AU rejected by GarageBand/Logic scan.
**Why it happens:** `PLUGIN_MANUFACTURER_CODE` / `PLUGIN_CODE` violate Apple four-char rules.
**How to avoid:** `NkMo` and `SbLm` follow first-uppercase-rest-lowercase pattern [CITED: JUCE CMake API].
**Warning signs:** Plugin builds but AU validation fails code-format tests.

### Pitfall 6: Trademark Strings in Metadata

**What goes wrong:** Legal guardrail violation (LEG-01).
**Why it happens:** Copy-paste from research docs referencing Rainger/Reverb-X/Igor.
**How to avoid:** Grep CI + source for banned strings; use SendBloom / Niko Audio Labs only.
**Warning signs:** `PRODUCT_NAME`, preset paths, or README contain banned terms.

### Pitfall 7: C++ Standard Mismatch

**What goes wrong:** Toolchain confusion; pamplejuce uses C++23 default, PROJECT.md says C++20.
**Why it happens:** `SharedCodeDefaults.cmake` sets `cxx_std_23`.
**How to avoid:** Override to `cxx_std_20` in SendBloom `CMakeLists.txt` after including defaults.
**Warning signs:** CI compiler errors on C++23-only features if compiler older.

## Code Examples

### SendBloom CMake Metadata Block

```cmake
# Source: JUCE CMake API + PROJECT.md + SCAF-05
set(PROJECT_NAME "SendBloom")
set(PRODUCT_NAME "SendBloom")
set(COMPANY_NAME "Niko Audio Labs")
set(BUNDLE_ID "com.nikoaudiolabs.sendbloom")
set(FORMATS AU VST3)

juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    PLUGIN_MANUFACTURER_CODE NkMo
    PLUGIN_CODE SbLm
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PRODUCT_NAME}"
    COPY_PLUGIN_AFTER_BUILD TRUE)
```

[CITED: https://github.com/juce-framework/JUCE/blob/develop/docs/CMake%20API.md]

### C++20 Override After SharedCodeDefaults

```cmake
# Source: sudara/cmake-includes + PROJECT.md constraint
include(SharedCodeDefaults)
# Override pamplejuce C++23 default:
target_compile_features(SharedCode INTERFACE cxx_std_20)
```

### Minimal CI Matrix (Phase 1 Slim)

```yaml
# Source: pamplejuce build_and_test.yml simplified
strategy:
  matrix:
    include:
      - name: Linux
        os: ubuntu-22.04
        pluginval-binary: ./pluginval
      - name: macOS
        os: macos-14
        pluginval-binary: pluginval.app/Contents/MacOS/pluginval
      - name: Windows
        os: windows-latest
        pluginval-binary: ./pluginval.exe
env:
  BUILD_TYPE: Release
  BUILD_DIR: Builds
  STRICTNESS_LEVEL: 5
  DISPLAY: :0
```

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Projucer `.jucer` projects | CMake `juce_add_plugin` | JUCE 6+ | SendBloom uses CMake-only per ADR-001 |
| Manual Catch2 submodule | CPM `gh:catchorg/Catch2@3.x` | pamplejuce 2024+ | Simpler version pinning |
| pluginval GUI-only | Headless CLI with strictness levels | pluginval 0.x→1.x | CI integration standard |
| VST2 + VST3 | VST3 + AU only | Industry 2020+ | SendBloom skips VST2 entirely |

**Deprecated/outdated:**
- **Projucer as primary build:** JUCE 8 CMake API is authoritative for new projects.
- **CLAP in Phase 1:** Deferred per ROADMAP/ADR-001; remove `clap-juce-extensions` submodule for minimal Phase 1 scaffold.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | JUCE submodule can track `develop` until release pin | Standard Stack | Breaking JUCE API changes mid-phase |
| A2 | `NkMo` / `SbLm` codes are unique enough for dev (verify before ship per SCAF-05) | Standard Stack | AU/VST3 scan conflicts with existing plugins |
| A3 | pluginval v1.0.3 zip URLs remain stable | CI Pattern | CI download 404; pin to specific release tag |
| A4 | Disabling code signing in CI is acceptable for Phase 1 | Pitfalls | macOS local load may require ad-hoc sign for Gatekeeper |
| A5 | `com.nikoaudiolabs.sendbloom` bundle ID is acceptable placeholder | Architecture | Must change if org domain differs |

## Open Questions

1. **Exact JUCE 8 git tag pin**
   - What we know: anthonyalfimov sample uses `8.0.12`; pamplejuce tracks `develop`.
   - What's unclear: Whether to pin stable tag now or track develop until Phase 10.
   - Recommendation: Pin `8.0.12` or latest `8.0.x` tag at scaffold time for reproducibility; document upgrade path.

2. **Keep melatonin_inspector in Phase 1?**
   - What we know: pamplejuce includes it by default; not required for passthrough.
   - What's unclear: User preference for UI debug tooling in early phases.
   - Recommendation: Keep (low cost, useful Phase 9); planner discretion.

3. **IPP in CI**
   - What we know: pamplejuce enables Intel IPP; adds install steps and failures.
   - What's unclear: N/A for passthrough.
   - Recommendation: Disable IPP (`PamplejuceIPP` commented out) for Phase 1.

## Environment Availability

| Dependency | Required By | Available | Version | Fallback |
|------------|------------|-----------|---------|----------|
| CMake | SCAF-01 build | ✓ | 4.3.3 | Install via brew/cmake.org |
| Clang (Apple) | macOS native build | ✓ | 21.0.0 | Xcode CLI tools |
| Git | Submodule init | ✓ | 2.50.1 | Required — no fallback |
| Ninja | Fast local/CI builds | ✗ | — | Use `-G Unix Makefiles` or Xcode generator |
| pluginval | SCAF-04 local verify | ✗ | — | CI downloads binary; local: brew not available — download release zip |
| 7z | CI unzip pluginval | ✓ | homebrew | `unzip` on macOS/Linux |
| Python3 | ancillary tooling | ✓ | 3.14.5 | — |
| Node.js | gsd-tools only | ✓ | 22.22.3 | Not required for plugin build |
| Xcode | AU build (macOS) | ✓ | 26.6 | Required for AU on macOS |
| Linux JUCE deps | Linux CI | CI-only | ubuntu packages | pamplejuce workflow apt install list |
| GitHub Actions | SCAF-03 | ✓ (when pushed) | — | Manual local build if no remote |

**Missing dependencies with no fallback:**
- Git (submodule init)
- macOS + Xcode for local AU builds

**Missing dependencies with fallback:**
- Ninja → Unix Makefiles / Xcode generator
- Local pluginval → rely on CI artifact validation

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 (CPM) |
| Config file | `cmake/Tests.cmake` (pamplejuce submodule) |
| Quick run command | `ctest --test-dir Builds -R passthrough --output-on-failure` |
| Full suite command | `ctest --test-dir Builds --verbose --output-on-failure` |

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|-------------|
| SCAF-01 | CMake configures, Tests target builds | integration | `cmake -B Builds && cmake --build Builds --target Tests` | ❌ Wave 0 |
| SCAF-02 | Passthrough preserves samples | unit | `ctest --test-dir Builds -R "Passthrough" -x` | ❌ Wave 0 |
| SCAF-02 | Plugin name is SendBloom | unit | `ctest --test-dir Builds -R "Plugin instance" -x` | ❌ Wave 0 |
| SCAF-03 | Cross-platform build passes | CI | GitHub Actions `build_and_test` workflow | ❌ Wave 0 |
| SCAF-04 | pluginval strictness 5 passes | CI | pluginval step in workflow | ❌ Wave 0 |
| SCAF-05 | Metadata codes in binary | manual grep + unit | `ctest -R "name"` asserts `getName() == "SendBloom"` | ❌ Wave 0 |
| SCAF-02 | DAW load passthrough | manual | Document Logic/REAPER load smoke | manual-only |

### Sampling Rate

- **Per task commit:** `ctest --test-dir Builds -R "passthrough|Plugin instance" --output-on-failure`
- **Per wave merge:** `ctest --test-dir Builds --verbose --output-on-failure`
- **Phase gate:** CI green + pluginval strictness 5 + documented DAW smoke before `/gsd-verify-work`

### Wave 0 Gaps

- [ ] Entire pamplejuce scaffold copied to repo root — covers SCAF-01
- [ ] `tests/PluginBasics.cpp` passthrough identity test — covers SCAF-02
- [ ] `.github/workflows/build_and_test.yml` (slimmed, strictness 5) — covers SCAF-03/04
- [ ] `source/PluginProcessor.cpp` namespace `sendbloom` + metadata — covers SCAF-05
- [ ] `git submodule update --init --recursive` documented in README
- [ ] Legal string audit script or grep checklist — covers LEG-01 precursor

## Security Domain

> `security_enforcement: true`, ASVS Level 1. Scaffold phase has minimal attack surface.

### Applicable ASVS Categories

| ASVS Category | Applies | Standard Control |
|---------------|---------|-----------------|
| V2 Authentication | no | N/A — no auth in plugin |
| V3 Session Management | no | N/A |
| V4 Access Control | no | N/A |
| V5 Input Validation | partial | Host drives buffer sizes; trust JUCE `processBlock` contract; no user file input in Phase 1 |
| V6 Cryptography | no | Code signing deferred Phase 1 |

### Known Threat Patterns for {stack}

| Pattern | STRIDE | Standard Mitigation |
|---------|--------|---------------------|
| Malicious host buffer sizes | DoS | JUCE `AudioBuffer` bounds; don't index outside `getNumSamples()` |
| Supply-chain compromised dependency | Tampering | Pin JUCE submodule tag; CPM Catch2 version pin; verify pluginval release URL |
| Secrets in CI logs | Information disclosure | Never echo signing passwords; use GitHub secrets when signing enabled |
| Trademark/legal exposure in metadata | Repudiation | SCAF-05 grep for banned strings |

## Project Constraints (from .cursor/rules/)

No `.cursor/rules/` directory exists in the workspace. No additional project-specific Cursor rules beyond user rules and planning artifacts (PROJECT.md, ADR-001).

## Sources

### Primary (HIGH confidence)

- `.planning/repo-samples/sudara-pamplejuce/` — CMakeLists.txt, build_and_test.yml, PluginProcessor.cpp, tests (local verified)
- `.planning/ADR-001-fork-decision.md` — pamplejuce base decision
- `.planning/phases/01-scaffold-passthrough/01-CONTEXT.md` — phase boundary and constraints

### Secondary (MEDIUM confidence)

- [JUCE CMake API](https://github.com/juce-framework/JUCE/blob/develop/docs/CMake%20API.md) — `juce_add_plugin`, FORMATS, plugin codes
- [pamplejuce setup guide](https://melatonin.dev/manuals/pamplejuce/getting-started/setting-your-project-up/) — customization checklist
- [pluginval README](https://github.com/Tracktion/pluginval/blob/develop/README.md) — strictness levels, CLI usage
- [pluginval CI guide](https://github.com/Tracktion/pluginval/blob/develop/docs/Adding%20pluginval%20to%20CI.md) — download and validate pattern
- [sudara/cmake-includes Tests.cmake](https://raw.githubusercontent.com/sudara/cmake-includes/main/Tests.cmake) — Catch2 3.8.1 pin

### Tertiary (LOW confidence)

- anthonyalfimov JUCE-CMake-Plugin-Template — JUCE 8.0.12 pin reference only; not primary scaffold

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH — verified against local repo-samples + official JUCE/pluginval docs
- Architecture: HIGH — pamplejuce pattern well-documented; SendBloom customizations are straightforward deltas
- Pitfalls: HIGH — code signing and AU-on-CI issues documented in upstream templates

**Research date:** 2026-07-06
**Valid until:** 2026-08-06 (30 days — stable ecosystem)
