# Stack Research

**Domain:** Realtime bandlimited sample-rate conversion (host ↔ 32,768 Hz) for JUCE 8 audio plugin
**Researched:** 2026-07-08
**Confidence:** HIGH (r8brain + JUCE constraints); MEDIUM (latency numbers until measured on target ratios)

## Recommended Stack

### Core Technologies

| Technology | Version | Purpose | Why Recommended |
|------------|---------|---------|-----------------|
| **r8brain-free-src** | **7.1** (pin `e71c31bf320f84210bb4bdcb57e296c39ce940f9`, 2026-05-25) | Arbitrary-ratio bandlimited SRC in `FixedRateAdapter` | MIT license; handles non-integer ratios (44.1/48/88.2/96 kHz ↔ 32768 Hz); linear-phase; explicit `getLatency()` / `getLatencyFrac()` for PDC; used in REAPER and other pro audio; project already chose r8brain-first in ADR track |
| **JUCE 8 `juce_dsp`** | 8.x (in-tree) | Existing delay/comb/APF core, `ProcessSpec`, optional `dsp::DelayLine` for adapter ring buffers | No new framework; tank stays on fixed 32768 Hz after extraction |
| **C++20** | — | `SchroederTankCore`, `FixedRateAdapter`, block `processBlock()` | Matches repo standard; r8brain requires C++11+ (satisfied) |
| **CMake + CPM** | CPM 0.42.0 (existing) | Vendor r8brain as INTERFACE dependency | Matches pamplejuce pattern; upstream has no `CMakeLists.txt` |

### Supporting Libraries

| Library | Version | Purpose | When to Use |
|---------|---------|---------|-------------|
| **r8brain PFFFT double** (`fft/pffft_double.c`) | bundled in r8brain 7.x | Optional ~15–25% SRC CPU reduction vs default Ooura FFT | Enable after prototype passes HF gates; define `R8B_PFFFT_DOUBLE=1` and compile one C file |
| **Catch2** | 3.8.1 (existing) | Offline SRC impulse/HF imaging tests, latency regression | Three-path harness: host-rate vs legacy accumulator vs ProperSRC |
| **libsamplerate** | 0.2.2 (BSD-2-Clause) | A/B reference SRC in tests only | Benchmark against r8brain on 44.1/48/96 ↔ 32768; **do not ship** unless r8brain fails acceptance |
| **SpeexDSP resampler** | 1.2.1 (`SpeexDSP-1.2.1`) | Low-footprint fallback evaluation | Only if r8brain latency/CPU unacceptable after tuning; perceptual-quality design, not mastering-grade |

### Development Tools

| Tool | Purpose | Notes |
|------|---------|-------|
| **pluginval** | Regression after PDC/latency changes | Re-run level 10 when `setLatencySamples()` policy changes |
| **Existing diagnostics harness** (v2.0) | HF imaging metrics at 14–15 kHz | Validates fix for accumulator imaging called out in PROJECT.md |

## Installation

```bash
# No system packages required for the recommended path.
# Add cmake/R8brain.cmake (or inline in CMakeLists.txt) — see Integration below.

# Optional A/B test dependency (diagnostics only, not linked in release plugin):
# macOS: brew install libsamplerate
# Linux: apt install libsamplerate0-dev
```

### CMake integration (recommended)

r8brain 7.0+ is **header-only** for C++11+ (no `r8bbase.cpp`, no pthread/Windows.h). Create a thin INTERFACE target:

```cmake
# cmake/R8brain.cmake
include(CPM)

CPMAddPackage(
    NAME r8brain-free-src
    GITHUB_REPOSITORY avaneev/r8brain-free-src
    GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
    DOWNLOAD_ONLY YES
)

add_library(r8brain INTERFACE)
target_include_directories(r8brain INTERFACE "${r8brain-free-src_SOURCE_DIR}")

# Optional performance path (recommended after quality sign-off):
# target_compile_definitions(r8brain INTERFACE R8B_PFFFT_DOUBLE=1)
# target_sources(r8brain INTERFACE
#     "${r8brain-free-src_SOURCE_DIR}/fft/pffft_double.c")

# In root CMakeLists.txt:
# include(R8brain)
# target_link_libraries(SharedCode INTERFACE r8brain)
```

**Realtime contract:** construct `r8b::CDSPResampler` in `prepare()` only; `process()` uses internal buffers (no heap in steady state). One resampler instance per mono channel (SendBloom authentic path is mono-internal).

### Application integration points

| Component | Stack role |
|-----------|------------|
| `SchroederTankCore` | Runs **only** at `kInternalRate` (32768 Hz); no host-rate branches |
| `FixedRateAdapter` | Owns `r8b::CDSPResampler` up (host→32k) and down (32k→host); block `processBlock()` |
| `SchroederTank32` (wrapper) | Host-rate path unchanged; `authentic_color` routes through `FixedRateAdapter` + core |
| `Authentic32Mode` enum | `LegacyAccumulator` (no r8brain) vs `ProperSRC` (r8brain) for diagnostics |
| `PluginProcessor` | Sum up/down latency via `getLatency()`; ADR-003 decides `setLatencySamples()` vs v2 deferral |

**r8brain API surface (minimal):**

```cpp
#include "CDSPResampler.h"

// prepare(): max host block size drives aMaxInLen
r8b::CDSPResampler upsampler { hostRate, 32768.0, maxHostBlock };
r8b::CDSPResampler downsampler { 32768.0, hostRate, maxInternalBlock };

// processBlock(): "pull" until output buffer filled (async processor)
// double* in/out; query latency for PDC:
const int hostLatency = upsampler.getLatency() + downsampler.getLatency();
```

Filter knobs for latency tradeoff (after HF tests pass): `ReqTransBand` 2.0 (default, safest) → 5.0–8.0 (lower latency, still often inaudible on guitar wet path per upstream guidance).

## Alternatives Considered

| Recommended | Alternative | When to Use Alternative |
|-------------|-------------|-------------------------|
| **r8brain-free-src** | **libsamplerate 0.2.2** | If team prefers mature C API and accepts external link dependency; use `src_process()` streaming API, `SRC_SINC_BEST_QUALITY`; measure latency separately |
| **r8brain-free-src** | **SpeexDSP 1.2.1** | Ultra-low CPU budget after profiling; `speex_resampler_init_frac()` with quality 8–10; accept lower stopband vs r8brain |
| **r8brain in-tree (CPM)** | **Vendor copy under `third_party/r8brain/`** | Air-gapped CI or offline builds; pin same 7.1 commit |
| **Block `processBlock()` adapter** | **Per-sample SRC wrapper** | Only for bring-up; block path matches v2.0 plan and amortizes r8brain filter setup |

## What NOT to Use

| Avoid | Why | Use Instead |
|-------|-----|-------------|
| **`juce::dsp::Oversampling`** | Integer powers of 2 only (2/4/8/16×); designed for anti-aliasing around nonlinear DSP, not 48 kHz ↔ 32768 Hz | r8brain `CDSPResampler` |
| **Accumulator + linear hold** (`processAuthentic()` today) | Not bandlimited; stable 14–15 kHz imaging at 48 kHz host | Proper SRC bridge |
| **Downstream anti-image SVF alone** | Cleans up artifacts; does not fix SRC | Remove or demote to belt-and-suspenders after ProperSRC passes metrics |
| **`juce::WindowedSincInterpolator` / `LagrangeInterpolator`** | Fractional **delay** within one rate; not a full-ratio SRC engine with anti-imaging filters | r8brain |
| **`juce::ResamplingAudioSource`** | Playback-oriented; default Lagrange quality; not plugin-grade arbitrary SRC | `FixedRateAdapter` + r8brain |
| **Custom windowed-sinc from scratch** | Weeks of filter design, latency tuning, SIMD, and validation; r8brain already solves this MIT-clean | r8brain first; revisit only if dependency rejected |
| **`src_simple()` (libsamplerate)** | Stateless whole-buffer API; breaks streaming continuity | `src_process()` with persistent `SRC_STATE` |
| **GPL SoX / `resample` 1.8.x** | License incompatible with portfolio/MIT preference | r8brain or BSD libsamplerate |
| **Float I/O through r8brain `process()`** | Library processes `double`; extra conversion unless you use `oneshot()` offline only | `double` scratch buffers in adapter (prepare-time allocated) |
| **Linking SpeexDSP + libsamplerate in release** | Bloat and duplicate SRC stacks | r8brain only in shipping binary; others test-only |

## Stack Patterns by Variant

**If RC1 / zero-PDC must hold temporarily:**
- Keep host-rate `SchroederTank32` as default (`authentic_color=0`)
- Ship ProperSRC on branch behind `Authentic32Mode::ProperSRC` without reporting latency until ADR-003

**If HF acceptance passes but CPU is tight:**
- Enable `R8B_PFFFT_DOUBLE=1` + `pffft_double.c`
- Relax `ReqTransBand` to 5.0 on downsample leg first (wet guitar tolerates more than mastering SRC)
- Consider slightly smaller `aMaxInLen` on upsampler (memory/CPU tradeoff per r8brain docs)

**If r8brain latency fails product bar:**
- Benchmark SpeexDSP 1.2.1 at quality 10 and libsamplerate `SRC_SINC_MEDIUM_QUALITY` in harness
- Do **not** revert to accumulator; lower filter order or report PDC

**If Extended stereo reverb later:**
- Duplicate resampler pairs per channel (r8brain: one instance per concurrent stream/channel)

## Version Compatibility

| Package A | Compatible With | Notes |
|-----------|-----------------|-------|
| r8brain-free-src 7.1 | JUCE 8, C++20, CMake 3.25+ | Header-only; no conflict with JUCE modules |
| r8brain 7.1 | macOS / Windows / Linux plugin targets | SSE2/NEON paths inside library; no extra link libs on C++11+ |
| r8brain + `R8B_PFFFT_DOUBLE` | Clang/GCC/MSVC in SendBloom CI | Single `.c` file; compile as C, link to C++ |
| libsamplerate 0.2.2 | Catch2 offline tests | System or CPM package; **optional** |
| SpeexDSP 1.2.1 | Autotools/CMake subproject | Heavier build than r8brain; fallback only |
| JUCE `dsp::Oversampling` | Wet OD Extended mode (future) | Keep for dirt oversampling only — orthogonal to 32k bridge |

### Target conversion ratios (SendBloom)

| Host rate | Ratio to 32768 Hz | r8brain notes |
|-----------|-------------------|---------------|
| 44100 | 1024:140625 (non-power-of-2) | Full arbitrary SRC path |
| 48000 | 1024:1500 | Rational but not 2^N; r8brain still optimal |
| 88200 | 2× 44100 | Same as 44.1k with doubled host |
| 96000 | 2× 48000 | Same as 48k with doubled host |

## Sources

- [r8brain-free-src README](https://github.com/avaneev/r8brain-free-src/blob/master/README.md) — MIT license, v7.1 header-only, realtime pull model, latency API (HIGH)
- [r8brain CDSPResampler reference](https://www.voxengo.com/public/r8brain-free-src/Documentation/a00098.html) — `getLatency()`, `process()`, constructor params (HIGH)
- [r8brain realtime latency discussion (issue #6)](https://github.com/avaneev/r8brain-free-src/issues/6) — `ReqTransBand` latency tradeoffs (MEDIUM)
- [JUCE dsp::Oversampling](https://docs.juce.com/master/classjuce_1_1dsp_1_1Oversampling.html) — 2/4/8/16× only (HIGH)
- [libsamplerate API](https://libsndfile.github.io/libsamplerate/api_full.html) — `src_process()` streaming (HIGH)
- [libsamplerate FAQ](https://libsndfile.github.io/libsamplerate/faq.html) — do not use `src_simple()` on streams (HIGH)
- [libsamplerate 0.2.2 tag](https://github.com/libsndfile/libsamplerate/tree/0.2.2) — BSD-2-Clause (HIGH)
- [SpeexDSP resampler header](https://github.com/xiph/speexdsp/blob/master/include/speex/speex_resampler.h) — `speex_resampler_init_frac`, latency helpers (HIGH)
- [SpeexDSP 1.2.1 tag](https://github.com/xiph/speexdsp/tree/SpeexDSP-1.2.1) — BSD license (HIGH)
- [CCRMA resampling survey](https://ccrma.stanford.edu/~jos/resample/Free_Resampling_Software.html) — ecosystem context (MEDIUM)
- SendBloom `PROJECT.md`, `source/SchroederTank32.h` — broken accumulator path, v2.0 Option C scope (HIGH)

---
*Stack research for: SendBloom v2.0 proper 32k SRC (host ↔ 32768 Hz)*
*Researched: 2026-07-08*
