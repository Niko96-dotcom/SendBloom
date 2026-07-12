# SendBloom

Gated dirty ambience guitar effect — AU and VST3 plugin built with JUCE 8.

SendBloom delivers parallel wet reverb with wet-only overdrive, dual gate placement, and a momentary pressure/send control. The dry guitar stays clean while the wet path blooms then chops hard when you stop playing — the signature "edited sample" ambience feel.

**Publisher:** Niko Audio Labs  
**Formats:** AU (macOS), VST3 (macOS, Windows, Linux)  
**License:** MIT

## Features

- Parallel dry/wet routing — dry path never gated or distorted
- Schroeder tank reverb with optional **32k Color** — when enabled, bandlimited host-rate ↔ 32,768 Hz conversion via r8brain ProperSRC (`FixedRateAdapter`); tank runs at fixed delay-table lengths with per-comb feedback calibration, damping, and optional 9-bit parameter quantization; original software, not firmware-derived. Legacy accumulator stepping exists only for diagnostics regression (`LegacyAccumulatorPath`). **For RC1, 32k Color is off by default** — the host-rate Schroeder tank is the production path
- Wet-only overdrive blended independently via `distn`
- Dual gate profiles: Pre (hum suppression) and Post (≤15 ms wet chop)
- Pressure send pad and MIDI CC1 momentary control
- 8 factory presets with host save/load round-trip
- Pedal-style UI with clip LED and advanced drawer
- Zero reported latency, mono-first authentic mode
- Catch2 test suite + pluginval strictness 10 in CI

## Signal routing

SendBloom uses a parallel pedal topology:

- **Dry path:** Unity copy of the mono-summed input, taken **before** input gain. Never gated or distorted.
- **Wet path:** Mono sum → `InputStage` (input gain + soft clip) → gated reverb chain → wet return.
- **Output:** Authentic mode writes dual-mono (identical L/R) unless Extended Stereo is enabled.

## Build

Requires CMake 3.25+, a C++20 compiler, and Git submodules initialized.

```bash
git submodule update --init --recursive
cmake -B Builds -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --config Release
ctest --test-dir Builds --output-on-failure -C Release
```

**Artifacts:**

| Format | Path |
|--------|------|
| VST3 | `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3` |
| AU (macOS) | `Builds/SendBloom_artefacts/Release/AU/SendBloom.component` |

### macOS local install

Unsigned local builds may need ad-hoc codesign:

```bash
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/AU/SendBloom.component"
```

## Legal & Clean-Room

SendBloom is an original software implementation inspired by publicly described gated ambience behavior — not reverse-engineered firmware or proprietary hardware.

- Product metadata uses **SendBloom** / **Niko Audio Labs** branding only
- Manufacturer code: `NkMo` | Plugin code: `SbLm`
- Legal metadata audit: `bash scripts/check-legal-metadata.sh`
- Fidelity classification: [`original-inspired`](CLAIM_STATUS.md); capture tooling exists, while hardware comparison and listening remain `human_needed`
- Reproducible clean-room capture procedure: [docs/reference-capture-protocol.md](docs/reference-capture-protocol.md)
- Third-party dependencies (including [r8brain-free-src](https://github.com/avaneev/r8brain-free-src), MIT): [docs/THIRD_PARTY_LICENSES.md](docs/THIRD_PARTY_LICENSES.md)

See [CLAIM_STATUS.md](CLAIM_STATUS.md), [docs/CLEAN_ROOM.md](docs/CLEAN_ROOM.md), and [docs/RELEASE_CHECKLIST.md](docs/RELEASE_CHECKLIST.md) for full positioning and release procedures.

## CI

GitHub Actions builds and tests on Linux, macOS, and Windows. Each matrix leg runs the legal metadata audit, full Catch2 suite, and **pluginval strictness 10 on the Release VST3** (AU is not validated by pluginval in CI). Local verification on this machine covers macOS Release build, tests, and VST3 pluginval only — Windows/Linux matrix legs and AU pluginval are **not verified** locally.

## Project Structure

```
source/          Plugin processor, DSP chain, UI
tests/           Catch2 unit and integration tests
resources/       Factory presets
cmake/           CMake build helpers
scripts/         Legal metadata checker
docs/            Clean-room and release documentation
```
