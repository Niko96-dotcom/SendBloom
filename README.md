# SendBloom

Gated dirty ambience guitar effect — AU and VST3 plugin built with JUCE 8.

SendBloom delivers parallel wet reverb with wet-only overdrive, dual gate placement, and a momentary pressure/send control. The dry guitar stays clean while the wet path blooms then chops hard when you stop playing — the signature "edited sample" ambience feel.

**Publisher:** Niko Audio Labs  
**Formats:** AU (macOS), VST3 (macOS, Windows, Linux)  
**Distributed builds:** macOS only — AU + VST3, universal (arm64 + x86_64). Windows and Linux VST3 builds run in CI but are not part of the signed public release.  
**License:** MIT

## Features

- Parallel dry/wet routing — dry path never gated or distorted
- One fixed-rate reverb engine — an **allpass ring** (four `allpass → delay → shelving` blocks, fed by four series input diffusers), built to the architecture the reference DSP chip class's manufacturer publishes, and sized to its real 32,768-word delay-RAM budget. Every host rate is bandlimited-converted to and from the **32,768 Hz** tank via r8brain ProperSRC (`FixedRateAdapter`). There is no sample-rate, fidelity, or color control. The rate, the delay-memory budget, and the loop topology come from the chip vendor's own published datasheet and design articles — this is a software model built to documented public constraints, not a verified match to any product and not a firmware-derived implementation. See [docs/fv1-reverb-architecture.md](docs/fv1-reverb-architecture.md) for the design rules, the measurements, and what changed from the previous Schroeder comb tank; the legacy comb, accumulator, and host-rate engines are retained only for diagnostics.
- Size spans **1.2 s to 6.0 s**. Public reference material specifies the 5–6 s maximum, not a minimum; 1.2 s is SendBloom's chosen useful floor because lower feedback targets collapse toward the current ring's measured ~0.9 s feed-forward tail.
- Wet-only overdrive blended independently via `distn`
- One gate circuit that moves: Pre (keeps hum out of the reverb and dirt, tail decays untouched) and Post (≤15 ms wet chop, tail and all). Same threshold, same close speed — only the placement changes, and the switch ramps rather than jumping.
- Pressure send pad and MIDI CC1 momentary control
- Optional Extended Stereo wet return in the Advanced drawer
- 8 factory presets with host save/load round-trip
- Pedal-style UI with clip LED and advanced drawer
- Zero reported latency, mono-first wet return
- Catch2 test suite + pluginval strictness 10 in CI

## Signal routing

SendBloom uses a parallel pedal topology:

- **Dry path:** Unity copy of the mono-summed input, taken **before** input gain. Never gated or distorted.
- **Wet path:** Mono sum → `InputStage` (input gain + soft clip) → gated reverb chain → wet return.
- **Output:** The engaged path writes dual-mono (identical L/R) unless Extended Stereo is enabled.

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

Unsigned local builds may need an ad-hoc codesign for development only:

```bash
codesign --force --sign - --timestamp=none "Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
codesign --force --sign - --timestamp=none "Builds/SendBloom_artefacts/Release/AU/SendBloom.component"
```

## Install (users)

Download the macOS disk image from the [Releases page](https://github.com/Niko96-dotcom/SendBloom/releases), verify it, and install:

```bash
shasum -a 256 -c SHA256SUMS.txt
```

Full instructions, upgrade, uninstall and troubleshooting: [docs/install.md](docs/install.md).

## Releasing (maintainers)

The release system is fail-closed and driven by a single canonical version source, the `VERSION` file. One command runs every gate:

```bash
RELEASE_MODE=public scripts/release/release-all.sh
```

- [docs/release.md](docs/release.md) — process, prerequisites, artifact contract, signing and notarization
- [docs/release-validation.md](docs/release-validation.md) — the validation checklist and exact commands
- [docs/rollback.md](docs/rollback.md) — withdrawing and re-releasing
- [CHANGELOG.md](CHANGELOG.md)

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
tests/           Catch2 unit and integration tests, plus release-script tests
resources/       Factory presets and the UI art the plugin embeds
tools/           Faceplate renderer, SVG tooling, snapshot and probe harnesses
scripts/         Legal metadata audit and the release pipeline
cmake/           CMake build helpers (submodule)
cmake-local/     Project-owned CMake modules, incl. the canonical version parser
docs/            Clean-room, architecture, UI-render and release documentation
.planning/       Roadmap, requirements, ADRs and per-phase planning records
.github/         CI workflows, issue and pull-request templates
```

`Builds/` is the canonical build tree and `JUCE/` and `cmake/` are submodules;
all three are untracked by design.
