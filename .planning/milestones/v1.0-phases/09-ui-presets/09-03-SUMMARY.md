---
phase: 09-ui-presets
plan: 03
subsystem: presets
tags: [factory-presets, round-trip, pluginval, verification]
requires:
  - phase: 09-02
    provides: Full pedal UI
provides:
  - 8 factory XML presets in bundle
  - Preset round-trip test suite
  - Phase 9 VERIFICATION.md
affects: [phase-10]
tech-stack:
  added: [juce_add_binary_data SendBloomPresets]
  patterns: [denormalised preset apply via convertTo0to1]
key-files:
  created: [source/FactoryPresets.h, source/FactoryPresets.cpp, resources/presets/*.xml, tests/PresetTest.cpp, tests/PluginEditorTest.cpp, .planning/phases/09-ui-presets/VERIFICATION.md]
  modified: [source/PluginProcessor.cpp, CMakeLists.txt, README.md]
key-decisions:
  - "Preset defs use denormalised values with setDenorm for skewed params"
  - "APVTS PARAM child XML format for embedded resources"
requirements-completed: [PRST-01, PRST-02]
coverage:
  - id: D1
    description: "96/96 Catch2 tests pass"
    verification:
      - kind: unit
        ref: "ctest --test-dir Builds"
        status: pass
    human_judgment: false
  - id: D2
    description: "Interactive DAW UI + preset audition"
    verification: []
    human_judgment: true
    rationale: "Executor cannot run host UI smoke"
duration: 30min
completed: 2026-07-06
status: partial
---

# Phase 9 Plan 03: Factory Presets + Verification Summary

**Eight factory presets with round-trip tests; pluginval 7 pass; human DAW smoke pending**

## Performance

- **Duration:** ~30 min
- **Tasks:** 2/3 (human checkpoint pending)
- **Files modified:** 14+

## Accomplishments

- `FactoryPresets`: 8 named programs via `getNumPrograms` / `setCurrentProgram`
- 8 XML preset resources embedded via `SendBloomPresets` binary data
- `PresetTest.cpp`: 6 tests (PRST-01, PRST-02)
- `PluginEditorTest.cpp`: headless editor smoke
- pluginval strictness 7 PASS (local)
- VERIFICATION.md + README Phase 9 smoke checklist

## Test Counts

| Suite | Count |
|-------|-------|
| Total ctest | **96/96** |
| Preset | 6 |
| PluginEditor | 2 |
| Delta from Phase 8 | **+8** |

## Deviations from Plan

### Auto-fixed Issues

**1. [Rule 1 - Bug] Preset apply used normalized values for skewed parameters**
- **Found during:** PresetTest PRST-02
- **Issue:** `setValueNotifyingHost` with denormalised values broke size/threshold on skewed ranges
- **Fix:** `setDenorm` via `convertTo0to1` for all float params
- **Files modified:** `source/FactoryPresets.cpp`

**2. [Rule 2 - Missing] APVTS XML uses PARAM children not root attributes**
- **Fix:** Updated all `resources/presets/*.xml` to JUCE PARAM child format

## Auth Gates

None.

## Self-Check: PASSED
