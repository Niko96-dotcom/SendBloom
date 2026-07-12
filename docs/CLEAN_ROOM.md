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

- `scripts/check-legal-metadata.sh` scans product-facing source, resources, tests, README, CMake files and modules, and CI workflows. It covers both file contents and normalized repo-relative filenames, including asset names, generated BinaryData symbol references, and CMake resource references.
- Matching is normalized by lowercasing and stripping non-alphanumeric characters (including spaces, hyphens, underscores, and punctuation), so spelling and punctuation variants collapse to the same policy token.
- Internal citation records are deliberately outside the product-facing scan surface: `.planning/` plus clean-room and release-tracking documents under `docs/` are allowlisted per milestone specification section 14.4 because they must preserve research and audit context.
- CI runs the legal audit on every build matrix leg.
- Reference work accepts only user-created audio captures with explicit provenance. The protocol and analysis tools are documented in [`reference-capture-protocol.md`](reference-capture-protocol.md); the sole current ADR-V1-17 classification is recorded in [`../CLAIM_STATUS.md`](../CLAIM_STATUS.md).

## Commercial Considerations (Post-v1)

If SendBloom is distributed commercially:

- Obtain appropriate JUCE license (GPL vs commercial).
- Conduct freedom-to-operate review before wide release (see REQUIREMENTS COM-02).

## Contact

Niko Audio Labs — SendBloom is a personal/portfolio release unless otherwise stated.
