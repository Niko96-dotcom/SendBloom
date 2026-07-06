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

### Phase 1 — Passthrough

Phase 1 completion gate — verify passthrough in a real host:

1. Build Release and locate artifacts under `Builds/SendBloom_artefacts/Release/`.
2. Rescan plugins in your DAW (Logic, REAPER, Cubase, etc.).
3. Insert SendBloom on an audio track; play guitar or a sine source.
4. Confirm output matches bypass — toggle plugin off/on; level unchanged (passthrough).
5. Verify plugin manager shows **SendBloom** by **Niko Audio Labs**.

### Phase 2 — Parameter Automation Smoke

Automated gates (`ctest`, 22 tests) must pass before human verification:

```bash
cmake --build Builds --config Release
ctest --test-dir Builds --output-on-failure -C Release
```

**Artifacts:**

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

**Human checklist:**

1. Load SendBloom in REAPER, Logic, or Cubase from the artefact paths above.
2. Open the host parameter/automation view — confirm **15 parameters** including split `input_gain` and `input_threshold`.
3. Play guitar or a sine test tone; automate **Size** for 10 seconds — listen for smooth tone change without stepped zipper noise.
4. Automate **input_gain** — confirm smooth level ride without blocky steps.
5. Toggle **bypass** five times quickly — confirm no audible clicks (5 ms crossfade).
6. Automate **distn** from 0 to 100% — confirm audible grind increase on the dummy wet path.
7. Save the project, reload — confirm parameter values restore (host state round-trip).

**Assumptions to spot-check** (see `02-RESEARCH.md` Assumptions Log A1–A8): default `input_gain` 0.5, `input_threshold` 0.35, main knob ramps 20–50 ms, soft send exponent 1.2.

Reply `approved` in the GSD session when complete, or describe issues found.

**macOS note:** Unsigned local builds may require ad-hoc codesign:

```bash
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
codesign --force --deep -s - "Builds/SendBloom_artefacts/Release/AU/SendBloom.component"
```

Document results in phase verification notes when completing Phase 1.
