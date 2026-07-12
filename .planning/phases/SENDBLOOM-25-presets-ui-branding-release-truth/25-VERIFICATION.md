---
phase: 25-presets-ui-branding-release-truth
verified: 2026-07-12
status: passed
score: 4/4-roadmap-criteria-supported
requirements: [UX-06, UX-07, UX-08, UX-09, UX-10, UX-11, UX-12, UX-13, UX-14, UX-15, UX-16]
---

# Phase 25 Verification

## Verdict

**`passed`** — all Phase 25 automated gates pass and the implementation supports all four ROADMAP success criteria. On 2026-07-12, Niko explicitly approved the procedural faceplate and host alignment, the Pressure Mode and 32k Color wording, and the decision to defer any Path A asset until post-RC0.

## Goal Verification

The shipping implementation, preset metadata, scanner, and documentation now match SendBloom's procedural clean-room release position:

- `source/ui/PedalFaceplatePaint.cpp` is the sole production faceplate paint path, draws `SENDBLOOM`, and contains original abstract geometry rather than the removed reference faceplate.
- `resources/ui/reverbx-faceplate.png` is absent; its BinaryData/CMake inclusion and runtime image fallback were removed.
- `scripts/check-legal-metadata.sh` lowercases and strips non-alphanumeric characters before comparing banned tokens, applies the same normalized policy to contents and repo-relative filenames, and passes on the current tree.
- All eight factory preset XML roots declare `preset_class="pre-v1-dev"`; preset XML parity and safety gates pass.
- Pressure Mode copy is controller-neutral, and README/clean-room copy is bounded to the implemented behavior and clean-room position.

## ROADMAP Success Criteria

### 1. Clean product-facing names and shipping assets — automated pass

- `Builds/Tests "[v1][contract][shipping-policy]"`: PASS, 8 assertions / 3 cases.
- `Builds/Tests "[release][legal]"`: PASS, 2 assertions / 1 case; it also executes the legal audit script.
- Direct inspection confirms the procedural title `SENDBLOOM`, no runtime image fallback, no CMake/BinaryData reference to the removed faceplate, and no removed asset on disk.
- The legal audit scans normalized content and normalized filenames across shipping-facing source, tests, build metadata, CI, presets, and UI resources.

### 2. Original procedural faceplate and aligned interaction surface — automated pass plus human_needed

- Path B, the original procedural chassis, is the only production faceplate path. Path A is explicitly deferred post-RC0 and remains subject to Niko asset approval.
- The editor remains fixed at 420x780. `PluginEditor::resized()` retains the established preset, knob, toggle, pad, and Advanced Drawer hit rectangles; the phase diff does not alter those coordinates.
- State overlays are still drawn after the procedural chassis from the same APVTS and pressure-state inputs.
- `Builds/Tests "[ui][editor]"`: PASS, 12 assertions / 2 cases, including editor dimensions and hittable/painted hotspots.
- `cmake --build Builds --config Release --target EditorSnapshot`: PASS.
- **Human needed:** final pixel-level visual sign-off and rendered host confirmation that every overlay/hit target feels correctly aligned. Automated geometry/source checks do not substitute for human visual approval.

### 3. Normalized legal scan and portable design evidence — automated pass

- The scanner lowercases and removes punctuation, whitespace, underscores, and hyphens through `tr -cd '[:alnum:]'`, then scans both file content and repo-relative filenames.
- `design-qa.md` contains repository-relative paths only; direct checks find no `/Users/` or `file://` paths.
- Its current production reference is `source/ui/PedalFaceplatePaint.cpp`, with reproducible `Tests` and `EditorSnapshot` commands and explicit human boundaries.
- Both the legal test and direct `EditorSnapshot` build pass on 2026-07-12.

### 4. Truthful Pressure Mode, preset, README, and clean-room copy — automated pass plus human_needed

- `source/ui/AdvancedDrawer.cpp` uses: `Pressure Mode: when on, wet feed follows pressure; when off, reverb stays always-on.` It contains no third-party controller name.
- All eight shipping preset XML roots carry `preset_class="pre-v1-dev"`; no migration promise was added.
- `Builds/Tests "[release][preset][xml]"`: PASS, 128 assertions / 1 case.
- `Builds/Tests "[release][verb][authentic]"`: PASS, 10 assertions / 2 cases.
- README keeps 32k Color off by default and describes the host-rate production path; clean-room docs describe original software behavior and explicitly deny firmware/bytecode/schematic cloning.
- **Human needed:** final editorial approval of Pressure Mode and 32k Color wording.

## Requirement Disposition

| Requirement | Disposition | Evidence / boundary |
|---|---|---|
| UX-06 | automated pass; editorial human_needed | Exact controller-neutral tooltip in `AdvancedDrawer.cpp`. |
| UX-07 | automated pass | Normalized legal audit and shipping-policy tests pass. |
| UX-08 | automated pass | Filename scan passes; legacy faceplate asset is absent. |
| UX-09 | automated pass | Procedural paint draws `SENDBLOOM`. |
| UX-10 | automated pass | Asset, BinaryData inclusion, CMake reference, and runtime fallback removed. |
| UX-11 | satisfied by procedural Path B; Path A approval human_needed | Procedural chassis is production; future asset approval explicitly deferred. |
| UX-12 | automated pass | Scanner normalizes case/punctuation/spacing and scans filenames. |
| UX-13 | automated pass | `design-qa.md` uses repo-relative current source/build evidence. |
| UX-14 | automated geometry pass; rendered visual human_needed | Coordinates unchanged; UI tests and snapshot build pass. |
| UX-15 | automated pass | Eight preset roots classified `pre-v1-dev`; XML parity passes. |
| UX-16 | automated/source pass; editorial human_needed | Release copy tests pass; docs remain clean-room/evidence bounded. |

## Independent Regression Evidence

Built `Tests` successfully at verification HEAD, then reran the complete Phase 25 matrix:

- `[release][safe]`: PASS — 24,020 assertions / 3 cases.
- `[pressure-release]`: PASS — 8 / 1.
- `[oversized-block]`: PASS — 2 / 1.
- `[true-bypass]`: PASS — 4 / 1.
- `[midi-apvts]`: PASS — 5 / 2.
- `[input-anchors]`: PASS — 3 / 1.
- `[posthard]`: PASS — 7 / 1.
- `[per-sample]`: PASS — 18 / 3.
- `[realtime]`: PASS — 26,150,428 / 12.
- `[authentic]`: PASS — 48,063 / 6.
- `[wet-dirt]`: PASS — 4,111 / 7.

## Human Gate Resolution

1. Procedural production faceplate and rendered host alignment: **approved by Niko on 2026-07-12**.
2. Pressure Mode and 32k Color copy: **approved by Niko on 2026-07-12**.
3. Path A original faceplate asset: **explicitly deferred until post-RC0 by Niko on 2026-07-12**.

No automated failure or implementation gap was found. The previously explicit human gates were resolved by the user's approval and are recorded here rather than inferred from automated evidence.
