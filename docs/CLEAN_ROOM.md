# SendBloom Clean-Room Positioning

**Product:** SendBloom v1.0  
**Publisher:** Niko Audio Labs  
**Last updated:** 2026-07-06

## Purpose

SendBloom is an original software implementation of a **gated dirty ambience** guitar effect inspired by publicly described behavior of hardware reverb pedals. It is a **clean-room** project: behavioral goals come from listening notes, published descriptions, and musician-facing documentation — not from reverse engineering proprietary firmware, EEPROM dumps, or bytecode.

## What We Did

- Designed original DSP in C++20 using JUCE 8: Schroeder tank reverb, wet-only overdrive, dual gate profiles, pressure/send control, and parallel dry/wet routing.
- Authored factory presets, UI copy, and parameter curves independently.
- Validated behavior with unit tests, integration tests, realtime stress harness, and pluginval.

## What We Did Not Do

- **No EEPROM or FV-1 bytecode decompilation** — we do not possess or use disassembled firmware from any hardware product.
- **No trademarked names in shipping metadata** — product name, presets, and user-visible strings use SendBloom / Niko Audio Labs branding only.
- **No schematic or BOM cloning** — analog hardware circuits are not replicated; this is a digital model of an audible effect category.

## Behavioral Inspiration (Allowed)

Public-domain effect concepts we intentionally evoke:

- Parallel dry + gated wet reverb
- Wet-path distortion blended independently of dry integrity
- Momentary send / pressure control
- Hard post-gate chop for "edited sample" ambience

These are common musician-described behaviors, not proprietary interfaces.

## Legal Metadata Controls

- `scripts/check-legal-metadata.sh` scans sources, tests, CI, README, and `resources/presets/*.xml` for banned third-party identifiers.
- CI runs the legal audit on every build matrix leg.

## Commercial Considerations (Post-v1)

If SendBloom is distributed commercially:

- Obtain appropriate JUCE license (GPL vs commercial).
- Conduct freedom-to-operate review before wide release (see REQUIREMENTS COM-02).

## Contact

Niko Audio Labs — SendBloom is a personal/portfolio release unless otherwise stated.
