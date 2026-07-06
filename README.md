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

### Phase 3 — Ugly Signature Chain DAW Smoke

Automated gates (`ctest` 36 tests, pluginval strictness 5) must pass before human verification.

**Artifacts:**

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

**Human checklist:**

1. Load SendBloom from the artefact paths above on a guitar or DI track.
2. Set level ~50%, size ~50%, distn ~30%, send connected on, gate Post, defaults elsewhere.
3. Play a palm-muted riff — confirm dry attack stays clean while wash blooms in parallel.
4. Raise distn to 100% — confirm grind on the wash only; dry pick attack unchanged.
5. Stop playing — confirm wet chops hard within ~15 ms ("edited sample" feel) with gate Post active.
6. Hold send pad or automate send 1→0 during ring — confirm tail decays naturally, not instant mute.
7. Toggle gate Pre — confirm hum-suppression character changes on the wet feed (crude stub OK).
8. Note tone is intentionally ugly; routing feel is the pass criteria.

Reply `approved` in the GSD session when complete, or describe routing issues found.

Document results in phase verification notes when completing Phase 3.

### Phase 4 — IO + Gate Correctness DAW Smoke

Automated gates (`ctest` 53 tests, pluginval strictness 5) must pass before human verification.

**Artifacts:**

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

**Human checklist:**

1. Load SendBloom on a guitar or DI track.
2. Raise **In** — level increases; push hot input — soft clip (no harsh digital clip).
3. **Out** knob trims overall level.
4. Gate **Post** — stop playing — wet chops within ~15 ms ("edited sample" feel).
5. Gate **Pre** — quiet passages show hum suppression on wet feed only.
6. Dry pick attack stays clean when gate closes (dry never gated).
7. Toggle Pre/Post — gate position changes on wet path only.

Reply `approved` when complete, or describe IO/gate issues found.

### Phase 5 — SchroederTank32 Reverb DAW Smoke

Automated gates (`ctest` 62 tests, pluginval strictness 5, RT60 impulse tests) must pass before human verification.

**Artifacts:**

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

**Human checklist:**

1. Load SendBloom on a guitar or DI track.
2. Raise **Size** — reverb tail lengthens audibly.
3. Toggle **Dark** — predelay + darker tail vs Bright (immediate, brighter).
4. **authentic_color** ON — subtle lo-fi FV-style coloration.
5. Bloom-then-chop routing still feels correct (dry clean, wet gated).
6. Send release — tail decays naturally without hard cut.

Reply `approved` when complete, or describe reverb/routing issues found.

### Phase 6 — Wet Overdrive + Dry Integrity DAW Smoke

Automated gates (`ctest` 69 tests, pluginval strictness 5, dry-path THD test) must pass before human verification.

**Artifacts:**

- **VST3:** `Builds/SendBloom_artefacts/Release/VST3/SendBloom.vst3`
- **AU (macOS):** `Builds/SendBloom_artefacts/Release/AU/SendBloom.component`

**Human checklist:**

1. Load SendBloom on a guitar or DI track.
2. Set level ~50%, size ~50%, distn ~30%, send on, gate Post, defaults elsewhere.
3. Play — confirm wash has subtle asymmetric grind on the wet path.
4. Raise **distn** to 100% — confirm grind increases on wash only; dry pick attack unchanged.
5. Level at 100% with distn max — dry guitar stays pristine (no added grit on direct signal).
6. Bloom-then-chop routing still feels correct.

Reply `approved` when complete, or describe overdrive/routing issues found.

