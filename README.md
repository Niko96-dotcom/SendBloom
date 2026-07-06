# SendBloom

SendBloom is a JUCE 8 audio effect plugin (AU + VST3) built on a pamplejuce-style CMake scaffold. Phase 1 delivers a passthrough processor with Catch2 tests and CI validation.

**Manufacturer:** Niko Audio Labs  
**Plugin codes:** NkMo / SbLm  
**Bundle ID:** com.nikoaudiolabs.sendbloom

## Prerequisites

- CMake 3.25 or newer
- Git with submodule support
- macOS: Xcode (for AU builds)
- Ninja (recommended)

## Submodule setup

```bash
git submodule update --init --recursive
```

Pin JUCE to a stable 8.0.x release after init:

```bash
cd JUCE && git checkout 8.0.12 && cd ..
```

## Configure and build

```bash
cmake -B Builds -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --config Release
```

AU builds are macOS-only. VST3 builds on macOS, Windows, and Linux.

## Run tests

```bash
ctest --test-dir Builds --verbose --output-on-failure
```

## Artifacts

Release plugin binaries are written under:

```
Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3
Builds/SendBloom_artefacts/Release/AU/SendBloom.component   # macOS only
```

## Legal / metadata

Manufacturer and four-character plugin codes are placeholders. Verify uniqueness with the plugin registry before commercial distribution.
