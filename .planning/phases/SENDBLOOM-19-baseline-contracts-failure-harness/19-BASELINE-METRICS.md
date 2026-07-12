# Baseline Factory Preset Metrics (BASE-07)

Frozen offline peak/RMS for all factory presets **before** any Phase 20–25 production DSP/UI fixes.

## Method (reproducible)

| Field | Value |
|-------|-------|
| Sample rate | 48 kHz |
| Block size | 512 |
| Blocks rendered | 24 (12288 samples total) |
| Excitation | Deterministic sine: `0.2 * sin(0.03 * sampleIndex)` into L and R |
| Host path | `PluginProcessor::setCurrentProgram` → `prepareToPlay` → `processBlock` |
| Capture test | `tests/BaselinePresetMetricsTest.cpp` |
| Filter command | `Builds/Tests "[baseline][metrics]"` |
| Alternate | `ctest --test-dir Builds -R BaselinePreset --output-on-failure` |
| Build | `BUILD_DIR=Builds`, `CMAKE_BUILD_TYPE=Release` |
| Captured against commit | `9024ccf` (test harness present; metrics values from this method) |
| Capture host | macOS Darwin arm64 (local) |

Assertions in the Catch2 test are limited to **finite / non-NaN** peak and RMS so the suite stays green under current (pre-fix) DSP. Values below are current truth, not v1 contracts.

## Per-preset peak / RMS (L/R)

| Index | Preset | Peak L | Peak R | RMS L | RMS R |
|------:|--------|-------:|-------:|------:|------:|
| 0 | Sparkle Verb | 0.304878 | 0.304878 | 0.176642 | 0.176642 |
| 1 | Cut Sample Gate | 0.268345 | 0.268345 | 0.165891 | 0.165891 |
| 2 | Spacerock Burn | 0.263437 | 0.263437 | 0.149479 | 0.149479 |
| 3 | Dry Dub Sends | 0.241470 | 0.241470 | 0.145304 | 0.145304 |
| 4 | Dark Bloom | 0.236392 | 0.236392 | 0.133107 | 0.133107 |
| 5 | Firm Pressure | 0.309594 | 0.309594 | 0.176031 | 0.176031 |
| 6 | Gated Room | 0.425616 | 0.425616 | 0.213968 | 0.213968 |
| 7 | Hot Clip | 0.353986 | 0.353986 | 0.209555 | 0.209555 |

## Factory pressure-preset truth (document only — do not fix here)

| Preset | `send_connected` | `send_amount` |
|--------|------------------|---------------|
| Firm Pressure | 1 | 0.85 |
| Hot Clip | 1 | 1.0 |

These presets load send engaged with non-zero amount today; Pressure Mode v1 contracts land in later phases.

## Regeneration

```bash
cmake -B Builds -DCMAKE_BUILD_TYPE=Release .
cmake --build Builds --config Release --target Tests
Builds/Tests "[baseline][metrics]"
```

Stdout lines prefixed with `BASELINE_METRICS` print the live table for re-copy into this file after intentional DSP changes (not during Phase 19 harness work).
