---
phase: 25
slug: presets-ui-branding-release-truth
status: approved
path: B  # procedural chassis is the production faceplate; Path A deferred post-RC0
canvas: 420x780
shadcn_initialized: false
preset: none
created: 2026-07-12
---

# Phase 25 — UI Design Contract (Procedural Faceplate = Production)

> Locks the procedural pedal chassis in `source/ui/PedalFaceplatePaint.cpp` as the
> **production faceplate** (Path B, per `25-CONTEXT.md`). The third-party reference
> faceplate asset (`resources/ui/reverbx-faceplate.png`) is removed from the shipping
> binary. This is a **branding truth + asset-swap contract**, not a visual redesign:
> canvas, layout grid, hotspot coordinates, overlay coordinates, and the original
> SendBloom palette are all preserved. Only third-party-derived text/art is replaced
> with original SendBloom content.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | JUCE 8 `juce::Graphics` procedural paint (NOT a web/UI framework) |
| Canvas | 420 × 780 px (`kEditorWidth=420`, `kEditorHeight=780`, `PluginEditor.cpp`) |
| Paint entry | `sendbloom::ui::paintPedalFaceplate()` → `paintProceduralChassis()` (sole path after Path B) |
| Component library | existing `PedalKnob`, `AdvancedDrawer`, `PressureSendPad`, `TransparentControls` |
| Icon library | none (all glyphs are hand-painted `juce::Path` / `drawFittedText`) |
| Font | `juce::FontOptions(size, juce::Font::bold)` — JUCE default typeface, bold weights only |

---

## Scope Fence

**In scope (this contract locks)**
- Title wordmark becomes `SENDBLOOM` (UX-09)
- Removal of all third-party-derived text/art from the procedural chassis (UX-07)
- Removal of `reverbx-faceplate.png` from disk, BinaryData, and CMake (UX-08, UX-10)
- Confirmation that the existing original palette, logo, hotspots, and overlays remain aligned (UX-14)
- Canonical Pressure Mode tooltip copy (UX-06)
- Copywriting rule: no third-party product/brand/controller names anywhere user-facing

**Out of scope (do NOT touch in this phase)**
- Canvas size, layout grid, hotspot coordinates, overlay coordinates (locked — see §6, §7)
- Original SendBloom color palette (already original — locked in §4)
- The `Niko` / `FX` logo plate (already SendBloom-original — see §3.2)
- Parameter IDs, knob ranges, Advanced drawer control set
- Visual redesign beyond removing third-party references and supplying the `SENDBLOOM` wordmark
- Path A Niko-approved `sendbloom-faceplate.png` asset (deferred post-RC0, `human_needed`)

---

## 1. Wordmark & Branding Contract

### 1.1 Title wordmark — `SENDBLOOM` (UX-09)

| Property | Locked value |
|----------|--------------|
| Text | `"SENDBLOOM"` (uppercase, no spaces, one word) |
| Font | `juce::FontOptions(34.0f, juce::Font::bold)` |
| Colour | `juce::Colours::black` |
| Rect | `(104, 90, 218, 42)` — **unchanged** from current `REVERB X` rect |
| Justification | `juce::Justification::centred`, single line, max scale `0.86f` |
| Cyan underline | two segments retained: `(48,113)→(112,113)` and `(312,113)→(372,113)`, stroke `5.0f`, colour `cyan` (`0xff5fc0d2`) |

**Replacement site:** `PedalFaceplatePaint.cpp` line ~303.
The literal `"REVERB X"` string is replaced 1:1 with `"SENDBLOOM"`. The 218-px-wide centred rect comfortably fits 9 uppercase glyphs at 34 px bold; no rect change required.

### 1.2 Publisher logo — `Niko` / `FX` (unchanged, already original)

`drawLogo(g, {60.0f, 24.0f, 300.0f, 54.0f})` is **SendBloom-original** and stays as-is:
- Black rounded plate, orange (`0xffff8f25`) parallelogram
- `"Niko"` in black bold 55 px inside the orange field
- `"FX"` in black bold 58 px on the orange sub-plate

`Niko Audio Labs` is the SendBloom publisher (`CMakeLists.txt` `COMPANY_NAME`). This is not a third-party reference. **No change.**

### 1.3 Central art panel — replace third-party-derived content with original

The panel rect `(58, 195, 154, 219)` labelled `"OVERLOAD / CLIP"` is original and stays. The art **inside** it currently spells the third-party product name and must be replaced:

| Current (REMOVE) | Lines | Reason |
|------------------|-------|--------|
| Vertical `"REVERB"` letters (char-by-char) | 320–323 | Third-party product name |
| Standalone giant `"X"` glyph, 116 px | 325–326 | Third-party product name (UX-09 explicit) |
| Four concentric ellipses around the `X` | 328–332 | Trade-dress-derived decorative frame around the name |

**Replacement contract (locked constraints, executor chooses final art within them):**
- Region: confined to the existing leftPanel rect `(58, 195, 154, 219)` — **do not change panel coords** (UX-14).
- Content: original SendBloom abstract geometry only. No letterforms that spell or abbreviate any product/brand name. Permitted motifs: the existing `drawChevronRail` chevron shape, concentric rounded rects, or an abstract `SB` monogram rendered as geometry (not as a wordmark string).
- Palette: restricted to the locked palette in §4 (`cyan`, `black`, white gradient, optional `accent` orange). No new colours.
- The `"OVERLOAD / CLIP"` header label at the top of the panel is retained verbatim.
- No hotspot, knob, or overlay lives in this region (verified — see §7), so the art rework cannot affect hittability.

### 1.4 Copywriting rule (UX-07, binding on the whole phase)

> No user-facing string, painted glyph, asset filename, BinaryData symbol, or visual
> element in the shipping binary may contain a third-party product, brand, or
> controller name — in any spelling variant (`Reverb-X`, `REVERB X`, `reverbx`,
> `Reverb X`, etc.). This rule applies to source contents AND filenames (UX-08).
> Internal research/milestone docs under `.planning/` and `docs/` that *cite* the
> third party for clean-room record are explicitly out of the product-facing scan
> (spec §14.4 allowlist).

---

## 2. Asset Removal Contract (Path B)

`paintPedalFaceplate()` currently tries to load the PNG from disk and BinaryData before falling back to procedural (lines 422–435). After Path B, the procedural chassis is the **only** paint path.

| Action | File | Detail |
|--------|------|--------|
| Delete file | `resources/ui/reverbx-faceplate.png` | Third-party faceplate — clean-room policy prohibits shipping |
| Remove from BinaryData | `CMakeLists.txt` line 82 | Delete the `resources/ui/reverbx-faceplate.png` entry from `juce_add_binary_data(SendBloomPresets SOURCES ...)` |
| Remove load path | `source/ui/PedalFaceplatePaint.cpp` `paintPedalFaceplate()` | Delete the `ImageFileFormat::loadFrom(...)` block (lines ~422–433); call `paintProceduralChassis()` + `drawStateOverlays()` directly |
| Keep | `resources/ui/knob.png` | Orthogonal knob asset, not in scope |

After removal, `paintPedalFaceplate()` body becomes effectively:
```cpp
paintProceduralChassis(g, bounds, cyan, advancedExpanded);
drawStateOverlays(g, apvts, clipActive, advancedExpanded, padPressed, padDisplayAmount);
```

> **Deferred (not blocking):** a future Niko-approved `resources/ui/sendbloom-faceplate.png` (Path A) may replace the procedural paint post-RC0. That swap is `human_needed` for asset approval and does not affect this contract's coordinates — Path A must reuse the identical canvas, hotspot grid, and overlay coords locked here.

---

## 3. Color Palette (LOCKED — already SendBloom-original)

The procedural chassis palette defined in `SendBloomLookAndFeel.cpp` is original SendBloom trade dress, **not** derived from the third-party product. It is frozen as-is for v1.

| Role | Hex (ARGB) | `juce::Colour` source | Used for |
|------|-----------|------------------------|----------|
| Background fill | `0xff0a0a0a` | literal in `paintProceduralChassis` | Outside the chassis plate |
| Chassis plate gradient top | `0xffffffff` (white) | `juce::Colours::white` | Plate fill start |
| Chassis plate gradient bottom | `0xffeeeeeb` | literal | Plate fill end |
| Plate outer border | `0xff253035` | literal | 2 px rounded-rect stroke |
| Plate inner highlight | white @ 0.5 alpha | `juce::Colours::white.withAlpha(0.5f)` | 2 px inner stroke |
| **Accent cyan** (primary) | `0xff5fc0d2` | `lookAndFeel.cyanColour()` | Frame, chevrons, knob ring/arc, Advanced outline, underlines |
| Accent orange (logo only) | `0xffff8f25` | `lookAndFeel.accentColour()` | `Niko`/`FX` plate |
| Label text | `0xff050505` / `juce::Colours::black` | `lookAndFeel.labelColour()` | All faceplate labels |
| Toggle tick / disabled | `0xff444444` | literal | Advanced controls |
| Green LED ring | `0xff295a34` | literal | Dark-mode LED surround |
| Green LED fill gradient | `0xff98ff56` → `0xff2f9b39` | literal | Dark-mode LED |
| Red overload arrow | `0xffa52829` | literal | Overload arrow fill |

**No new colours may be introduced** by the central-art rework (§1.3) or any other Phase 25 paint change. The cyan/orange/green/black palette is the SendBloom original and is part of the locked contract.

---

## 4. Typography Contract

All faceplate type uses `juce::FontOptions(size, juce::Font::bold)` with JUCE's default typeface. No custom font file is loaded. Lock the existing sizes:

| Element | Font spec | Colour |
|---------|-----------|--------|
| Title wordmark `SENDBLOOM` | `FontOptions(34.0f, bold)` | black |
| Logo `Niko` | `FontOptions(55.0f, bold)` | black |
| Logo `FX` | `FontOptions(58.0f, bold)` | black |
| `OVERLOAD / CLIP` label | `FontOptions(14.0f, bold)` | black |
| `OVERLOAD` chip label | `FontOptions(18.0f, bold)` | black on cyan |
| `DARK MODE` / `GATE` / `PRE` / `POST` | `FontOptions(14.0f–16.0f, bold)` | black |
| Advanced panel header `ADVANCED` | `FontOptions(17.0f, bold)` | cyan (fan-out) / `18.0f` cyan (drawer) |
| Advanced sub-labels (`GATE SENS`, `SEND FEEL`, `FIRM`, `32K COLOR`, …) | `FontOptions(12.0f–13.0f, bold)` | cyan / black |
| Control icons caption (`SAVE`, `NEW`, `DELETE`) | `FontOptions(10.0f, bold)` | black |
| Preset name (editor-painted) | `FontOptions(12.0f, bold)` | black on white |

---

## 5. Layout & Hotspot Alignment (UX-14)

The 420 × 780 procedural chassis **already defines** the layout that every hotspot is measured against. Path B does not move any paint region. The table below confirms each hardcoded `setBounds(...)` in `PluginEditor::resized()` (lines 184–216) lands on a painted target.

### 5.1 Hotspot → paint alignment table

| Control | `setBounds` (x, y, w, h) | Painted target it sits on | Aligned? |
|---------|--------------------------|---------------------------|----------|
| `presetBox` | `(64, 148, 210, 16)` | Preset name field at top of chassis (cyan frame starts y=128) | ✅ no change |
| `saveButton` | `(300, 148, 30, 44)` | `SAVE` icon, `drawControlIcons` region `(300, 148, 92, 44)` | ✅ no change |
| `newButton` | `(330, 148, 30, 44)` | `NEW` icon | ✅ no change |
| `deleteButton` | `(360, 148, 36, 44)` | `DELETE` icon | ✅ no change |
| `lvlKnob` centre | `(265, 213)`, 50×76 | Right-column knob site (cyan frame interior) | ✅ no change |
| `sizeKnob` centre | `(265, 303)`, 50×76 | Right-column knob site | ✅ no change |
| `distnKnob` centre | `(265, 393)`, 50×76 | Right-column knob site | ✅ no change |
| `inKnob` centre | `(265, 479)`, 50×76 | Right-column knob site | ✅ no change |
| `outKnob` centre | `(265, 564)`, 50×76 | Right-column knob site | ✅ no change |
| `darkToggle` | `(64, 562, 40, 40)` | Dark disk painted at `(64, 562, 40, 40)` | ✅ exact match |
| `gateToggle` | `(148, 568, 22, 52)` | Gate slot painted at `(148, 568, 22, 52)` | ✅ exact match |
| `pressurePad` | `(90, 655, 95, 90)` | Footswitch well (centre `137, 696`, inside pad rect) | ✅ no change |
| `advancedButton` | `(250, 646, 132, 104)` | `advancedClosed` path `(250–382, 646–750)` | ✅ no change |
| `advancedDrawer` | `(232, 560, 166, h)` | `drawAdvancedFanout` region `(230–398, 560–754)` | ✅ no change |

**Verdict: zero hotspot coordinate changes required by Path B.** Every hotspot was measured against the procedural chassis, not the PNG, so removing the PNG leaves all hit targets aligned.

### 5.2 KNOWN GAP — default preset name field (must fix in this phase)

`PluginEditor::paint()` lines 170–181 currently skip repainting the preset name when `presetBox.getSelectedId() == 1`, with the comment:
> `// Default "INITIAL PATCH" is baked into the faceplate.`

That assumption held when the third-party PNG baked `INITIAL PATCH` into its pixels. The procedural chassis **does not** paint `INITIAL PATCH`. After Path B, the default preset's name field would be blank.

**Fix contract (locked):**
- Option A (preferred): the procedural chassis paints `"INITIAL PATCH"` at `(70, 148, 200, 14)` in `FontOptions(12.0f, bold)` black-on-chassis, matching the editor's redraw style. The editor's `if (presetBox.getSelectedId() != 1)` conditional then stays correct.
- Option B: remove the conditional in `PluginEditor::paint()` so the editor always paints the preset name (white background `70,148,200,14` + black text). The chassis paints nothing there.

Either is acceptable; both preserve the existing `(64, 148, 210, 16)` hotspot and the visual style. The planner picks one. `"INITIAL PATCH"` is original SendBloom copy (not third-party) and is safe to bake.

### 5.3 State overlay coordinates (UX-14 — all confirmed aligned)

All overlays in `drawStateOverlays()` are painted against procedural-chassis coordinates (not the PNG), so they remain aligned under Path B:

| Overlay | Painted coords | Aligns to | Status |
|---------|----------------|-----------|--------|
| `drawDarkPressedOverlay` | body `(64, 562, 40, 40)` | `darkToggle` exact rect | ✅ |
| `drawGatePositionOverlay` | slot `(148, 568, 22, 52)`, thumb `(151, 571, 16, 22)` | `gateToggle` exact rect; redrawn only when PRE | ✅ |
| `drawFootswitchPressedOverlay` | centre `(137, 696)`, ring `(91–183, 657–745)` | inside `pressurePad (90, 655, 95, 90)` | ✅ |
| `drawClipReadOverlay` | lamp `(174, 191, 19, 19)` | inside `OVERLOAD / CLIP` panel `(58, 195, 154, 219)` | ✅ |
| `drawAdvancedFanout` | `(230–398, 560–754)` | `advancedDrawer (232, 560, 166, h)` | ✅ |

The `shouldDrawFootswitchPressedOverlay()` rule (SEND-06) is unchanged: overlay fires on `padPressed || displayAmount > ε || sendAmount > ε`, never on `send_connected` alone.

---

## 6. Pressure Mode Copy (UX-06 — canonical, already clean)

The Pressure Mode tooltip in `AdvancedDrawer.cpp` lines 34–35 is **already free of third-party controller naming** and is locked as the canonical explanation:

> **`Pressure Mode: when on, wet feed follows pressure; when off, reverb stays always-on.`**

Rules:
1. This string stays verbatim. It is the single source of truth for the Pressure Mode explanation in the UI.
2. The word "controller" is intentionally absent. Do NOT reintroduce any third-party controller product name here or anywhere else.
3. The spec's longer form (§14.5: "Off: reverb input is always active. / On: the signal stays dry until the pressure pad, automation, or MIDI CC1 sends audio into the reverb. Existing tails continue after release.") is an **acceptable alternative** if the planner wants more detail — both are clean. Pick one; do not ship both.
4. The 32k Color tooltip ("Original software — not firmware-derived.") is clean and stays. **Note:** spec §14.5 flags the "May exhibit HF artifacts at some host rates" warning as potentially stale post-ProperSRC-validation; the planner should verify that line against current evidence and trim if unsupported. This is a copy-truth item, not a branding item.

---

## 7. Verification

| Check | Evidence |
|-------|----------|
| Title is `SENDBLOOM` | source scan: `PedalFaceplatePaint.cpp` contains `"SENDBLOOM"`, not `"REVERB X"` or `"X"` as a standalone glyph |
| No third-party strings in product-facing source | `scripts/check-legal-metadata.sh` normalized scan green (catches `Reverb-X`, `REVERB X`, `reverbx` via one normalized token) |
| No third-party filenames | scanner walks `source`, `resources`, `tests`, `README`, `CMake`, `docs/release`; BinaryData symbol list contains no `reverbx*` |
| Asset removed | `resources/ui/reverbx-faceplate.png` absent; `CMakeLists.txt` BinaryData list free of it; `BinaryData::reverbxfaceplate_png` symbol gone |
| Hotspots aligned | `PluginEditor::resized()` bounds unchanged from the table in §5.1; editor screenshot QA shows each control over its painted target |
| Overlays aligned | overlay coords table in §5.3 unchanged |
| Pressure Mode copy clean | tooltip contains no banned tokens; `human_needed` visual confirm |
| `[v1][contract][shipping-policy]` green | `V1ContractShippingPolicyTest` UX-07/UX-08 assertions flip to pass |
| Default preset name visible | procedural chassis shows `INITIAL PATCH` at preset field (or editor always paints it) — §5.2 fix applied |
| Path A deferral documented | `25-SUMMARY.md` or `docs/CLEAN_ROOM.md` notes Path A is post-RC0, `human_needed` |

---

## 8. Anti-Goals

- Do NOT redesign the pedal UI, palette, logo, layout grid, or any coordinate in §5.
- Do NOT introduce a new font, new colour, or new component library.
- Do NOT change the canvas size (420 × 780).
- Do NOT rename parameter IDs, knob ranges, or the Advanced drawer control set.
- Do NOT ship `reverbx-faceplate.png` under any circumstance (clean-room policy).
- Do NOT green `[shipping-policy]` by weakening the scanner or allowlisting the banned product-facing paths.
- Do NOT implement Path A (`sendbloom-faceplate.png`) in this phase — it is `human_needed` and deferred post-RC0.
- Do NOT bake any third-party name into the procedural chassis art even temporarily "for testing".
