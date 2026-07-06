# SendBloom

Clean-room JUCE 8 audio plugin — gated dirty ambience guitar effect (AU + VST3).

## Build

```bash
git submodule update --init --recursive
cmake -B Builds -DCMAKE_BUILD_TYPE=Release
cmake --build Builds --config Release
ctest --test-dir Builds --output-on-failure -C Release
```

Artifacts:

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

## Legal / Metadata

Product: **SendBloom** by **Niko Audio Labs**  
Manufacturer code: `NkMo` | Plugin code: `SbLm`

Placeholder codes must be verified unique before any commercial release (SCAF-05).

Run the legal audit locally:

```bash
bash scripts/check-legal-metadata.sh
```

## DAW Smoke Test

Phase 1 completion gate — verify passthrough in a real host:

1. Build Release and locate artifacts under `Builds/SendBloom_artefacts/Release/`.
2. Rescan plugins in your DAW (Logic, REAPER, Cubase, etc.).
3. Insert SendBloom on an audio track; play guitar or a sine source.
4. Confirm output matches bypass — toggle plugin off/on; level unchanged (passthrough).
5. Verify plugin manager shows **SendBloom** by **Niko Audio Labs**.

**macOS note:** Unsigned local builds may require ad-hoc codesign:

```bash
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/AU/SendBloom.component"
```

Document results in phase verification notes when completing Phase 1.
