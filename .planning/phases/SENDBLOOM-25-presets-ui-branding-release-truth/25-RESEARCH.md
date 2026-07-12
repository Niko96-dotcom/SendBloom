# Phase 25: Presets, UI, Branding & Release Truth - Research

**Researched:** 2026-07-12
**Domain:** Procedural faceplate rebrand, legal scanner rewrite, preset pre-v1 classification, docs truth, Pressure Mode copy (UX-06…16)
**Confidence:** HIGH

## Summary

Phase 25 makes the procedural pedal chassis the production faceplate (Path B), deletes the third-party reference PNG from the shipping binary, strengthens the legal scanner to catch all spelling variants (`Reverb-X`, `REVERB X`, `reverbx`) via one normalized token plus filename coverage, classifies the 8 factory presets as explicit pre-v1 development state with no migration promise, and rewrites `design-qa.md` plus clean-room/release docs to repo-relative verified evidence. It greens the two intentionally-red `[v1][contract][shipping-policy]` assertions in `tests/V1ContractShippingPolicyTest.cpp` (UX-07 faceplate string ban, UX-08 filename ban) by removing the offending material — never by weakening the scanner.

All five defect loci are localized and verified in source this session:

1. **Faceplate strings + asset** — `source/ui/PedalFaceplatePaint.cpp` lines 303 (`"REVERB X"`), 320-332 (vertical letters, giant `X`, concentric ellipses), 422-433 (PNG loadFrom block); `CMakeLists.txt` line 82 (BinaryData entry); `resources/ui/reverbx-faceplate.png` (379944-byte PNG on disk); plus the `INITIAL PATCH` preset-name gap at `source/PluginEditor.cpp` lines 170-181.
2. **Scanner blind spots** — `scripts/check-legal-metadata.sh` uses literal `grep -qi "Reverb-X"` (line 9, 36) which misses `REVERB X`/`reverbx`; scans file contents only (no filename scan); extension filter omits `*.png`/`*.cmake`.
3. **Preset classification** — all 8 XMLs in `resources/presets/` have root `<SendBloomParams>` with no classification field; `FactoryPresets.cpp` `applyEmbeddedXml` (lines 29-38) does a pure `replaceState` with no version logic.
4. **Stale docs** — `design-qa.md` has absolute `/Users/nikolay/Documents/RAINGER FX/...` paths (lines 8-19) and a stale `135/135` count (line 26); `docs/RELEASE_CHECKLIST.md` line 36 cites the third-party names (acceptable for an internal release doc but must be outside the product-facing scan surface).
5. **Pressure Mode copy** — `source/ui/AdvancedDrawer.cpp` lines 34-35 tooltip is already clean; the 32k Color tooltip (lines 38-43) carries a potentially stale "HF artifacts" warning that needs evidence verification.

**Primary recommendation:** Three bounded plans — (1) faceplate rebrand + asset removal + INITIAL PATCH fix + scanner rewrite (UX-07/08/09/10/12/14 are tightly coupled by the shipping-policy test), (2) preset pre-v1 classification + docs rewrite (UX-13/15/16), (3) copy verification + human gate documentation (UX-06/11). Each plan flips its `[v1][contract][shipping-policy]` assertions green and preserves all Phase 19-24 green contracts.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Procedural faceplate paint (title, central art) | UI Paint (`PedalFaceplatePaint.cpp`) | — | `paintProceduralChassis()` (line 283) is sole paint path after Path B; all third-party-derived art lives here |
| Faceplate asset lifecycle | Build / Packaging (`CMakeLists.txt` BinaryData) | Disk (`resources/ui/`) | `juce_add_binary_data` generates `BinaryData::reverbxfaceplate_png` from the CMake entry; removing the entry removes the symbol |
| Editor hotspot alignment | UI Components (`PluginEditor.cpp` `resized()`) | — | Hardcoded `setBounds` coords (lines 184-216) measured against procedural chassis; Path B moves nothing |
| Preset name rendering | UI Paint (`PluginEditor.cpp` `paint()`) | `PedalFaceplatePaint.cpp` | The `getSelectedId() != 1` conditional (line 171) assumes the faceplate bakes `INITIAL PATCH`; procedural chassis does not |
| Legal scanner | Tooling (`scripts/check-legal-metadata.sh`) | Tests (`ReleaseTruthTest.cpp`) | Bash script invoked by CTest via `std::system`; normalization + filename scan live here |
| Preset classification | Storage (`resources/presets/*.xml`) | Loader (`FactoryPresets.cpp`) | XML carries the declared classification; loader must not rewrite or migrate |
| Docs truth | Docs (`design-qa.md`, `README.md`, `docs/CLEAN_ROOM.md`) | — | Repo-relative paths, runtime test discovery, verified-behavior-only claims |

<user_constraints>
## User Constraints (from CONTEXT.md + UI-SPEC.md)

**CRITICAL:** Locked decisions are NON-NEGOTIABLE for planning/execution.

### Locked Decisions

#### D-01 — Path B: procedural chassis is the production faceplate (UX-09, UX-10, UX-11, UX-14)
- Ship `paintProceduralChassis()` as the sole production paint path; no PNG load
- Delete `resources/ui/reverbx-faceplate.png` from disk, BinaryData, and CMake
- Title wordmark becomes `"SENDBLOOM"` (uppercase, one word) at the unchanged rect `(104, 90, 218, 42)`
- Remove all third-party-derived central art (vertical `REVERB` letters, giant `X`, concentric ellipses)
- Replace central art with original SendBloom abstract geometry confined to the existing `(58, 195, 154, 219)` panel
- Preserve the original SendBloom palette, `Niko`/`FX` logo plate, canvas (420x780), all hotspot coords, all overlay coords
- Path A (Niko-approved `sendbloom-faceplate.png`) is explicitly deferred post-RC0 (`human_needed`)

#### D-02 — Legal scanner: normalized matching + filename coverage (UX-07, UX-08, UX-12)
- Rewrite around normalization: lowercase → strip spaces/hyphens/underscores/punctuation → compare
- ONE normalized banned token (`reverbx`) catches all three variants (`Reverb-X`, `REVERB X`, `reverbx`)
- Extend scanning to filenames (asset names, BinaryData symbols, CMake references), not just file contents
- Normalize both banned terms AND required terms on both sides (content + path)
- Keep the `.planning/` and internal docs allowlist (spec §14.4: cited research is out of product-facing scan)
- Do not green `[shipping-policy]` by weakening the scanner or allowlisting product-facing paths

#### D-03 — Presets: explicit pre-v1 classification, no migration promise (UX-15)
- Add an explicit `pre-v1-dev` classification to each of the 8 factory preset XMLs
- Classification must be machine-readable and declared (root attribute or metadata element)
- No silent rewriting / no state-version wrapper / no migration logic in `FactoryPresets.cpp`
- Loading remains best-effort; `applyEmbeddedXml` keeps its pure `replaceState` semantics
- All presets keep `authentic_color="0"` (DSP-14/15, already true — preserved)

#### D-04 — Docs: repo-relative paths, runtime discovery, verified behavior only (UX-13, UX-16)
- Rewrite `design-qa.md`: no absolute paths, no `RAINGER FX` references, no fixed `135/135` count
- Follow `docs/RELEASE_CHECKLIST.md` pattern: runtime test-count discovery, no hard-coded N/N
- `README.md` / `docs/CLEAN_ROOM.md`: describe only verified behavior; no circuit-emulation/exact-fidelity claims
- Screenshot QA references updated to the procedural faceplate

#### D-05 — Pressure Mode copy stays verbatim (UX-06)
- `AdvancedDrawer.cpp` tooltip (lines 34-35) is canonical and already clean — do not change
- The word "controller" is intentionally absent; no third-party controller name anywhere user-facing
- 32k Color "HF artifacts" warning (line 41) must be verified against Phase 24 ProperSRC evidence; trim if unsupported

### Claude's Discretion
- Exact normalized-token algorithm (regex strip vs. `tr` transliteration) as long as all three variants match
- Exact preset classification field name/placement (root attribute vs. metadata element) as long as it is explicit, machine-readable, and declares `pre-v1-dev` with no migration promise
- Whether `design-qa.md` keeps a live test-count or drops counts entirely (RELEASE_CHECKLIST prefers runtime discovery — prefer that pattern)
- Final central-art motif within §1.3 constraints (chevron rail, concentric rounded rects, abstract geometry)
- Option A vs. Option B for the INITIAL PATCH fix (UI-SPEC §5.2)

### Deferred Ideas (OUT OF SCOPE)
- Path A faceplate asset (Niko-approved `sendbloom-faceplate.png`) — post-RC0, `human_needed`
- New preset-browser architecture / cloud licensing / storefront / telemetry — post-v1
- Visual redesign beyond required original branding — branding only this release
- Fidelity/claim wording tied to hardware comparison — Phase 26 `CLAIM_STATUS.md`
- Extended Stereo / Dirt OS enablement — explicitly deferred
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| UX-06 | UI explains Pressure Mode without third-party controller naming | `AdvancedDrawer.cpp` lines 34-35 tooltip already clean; verify only [VERIFIED: source read] |
| UX-07 | No product-facing source string contains third-party product/brand/controller names | Remove `"REVERB X"` (line 303), vertical letters (320-323), `"X"` glyph (325-326); scanner normalized content scan [CITED: spec §14.4] |
| UX-08 | No shipping resource filename contains those names | Delete `reverbx-faceplate.png`, remove CMake entry (line 82); scanner filename scan [CITED: spec §14.4] |
| UX-09 | Procedural fallback says `SENDBLOOM`, not the referenced product name | 1:1 string swap at PedalFaceplatePaint.cpp line 303 [CITED: UI-SPEC §1.1] |
| UX-10 | Exact reference faceplate asset removed from shipping binary | Delete file + CMake entry + loadFrom block (422-433) [CITED: spec §14.3 Path B] |
| UX-11 | Niko-approved original faceplate ships, or procedural ships (`human_needed`) | Path B satisfies; Path A deferred post-RC0 [CITED: UI-SPEC §2] |
| UX-12 | Legal scan normalizes punctuation/spacing/case and scans filenames | Rewrite check-legal-metadata.sh: normalize both sides, add filename walk [CITED: spec §14.4] |
| UX-13 | `design-qa.md` uses portable repo-relative paths with current evidence | Rewrite lines 8-19 (absolute paths), line 26 (stale count) [CITED: spec §14.8] |
| UX-14 | Editor hotspots/overlays remain hittable and aligned after asset replacement | UI-SPEC §5.1 table confirms zero hotspot changes; verify no coord drift [VERIFIED: PluginEditor.cpp 184-216 vs paint] |
| UX-15 | Existing preset sessions explicitly classified pre-v1; no hidden migration promise | Add classification to 8 XMLs; keep `applyEmbeddedXml` pure `replaceState` [CITED: spec §14.7] |
| UX-16 | README and clean-room docs describe only verified behavior | Verify README compliant (ReleaseTruthTest assertions); update CLEAN_ROOM scanner description [CITED: spec §14.5] |
</phase_requirements>

## Current State (Exact Bug Loci)

### 1. Procedural faceplate — third-party naming + asset (UX-09, UX-10, UX-14)

**CURRENT (wrong) — `source/ui/PedalFaceplatePaint.cpp`:**

Title wordmark (line 303):
```cpp
g.drawFittedText ("REVERB X", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
```

Central art panel — vertical letters (lines 320-323):
```cpp
constexpr const char* reverb = "REVERB";
g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
for (int i = 0; i < 6; ++i)
    g.drawText (juce::String::charToString (reverb[i]), 58, 248 + i * 25, 34, 28, juce::Justification::centred);
```

Standalone giant X glyph (lines 325-326):
```cpp
g.setFont (juce::FontOptions (116.0f, juce::Font::bold));
g.drawFittedText ("X", 98, 240, 96, 120, juce::Justification::centred, 1, 0.8f);
```

Concentric ellipses around the X (lines 327-332):
```cpp
g.setColour (cyan);
for (int i = 0; i < 4; ++i)
{
    const auto f = static_cast<float> (i);
    g.drawEllipse (138.0f + f * 8.0f, 258.0f + f * 7.0f, 56.0f + f * 12.0f, 78.0f - f * 4.0f, 2.0f);
}
```

PNG load path in `paintPedalFaceplate()` (lines 422-433):
```cpp
auto faceplate = juce::ImageFileFormat::loadFrom (
    juce::File::getCurrentWorkingDirectory().getChildFile ("resources/ui/reverbx-faceplate.png"));
if (! faceplate.isValid())
    faceplate = juce::ImageFileFormat::loadFrom (BinaryData::reverbxfaceplate_png,
                                                 static_cast<size_t> (BinaryData::reverbxfaceplate_pngSize));

if (faceplate.isValid())
{
    g.drawImage (faceplate, bounds);
    drawStateOverlays (g, apvts, clipActive, advancedExpanded, padPressed, padDisplayAmount);
    return;
}

paintProceduralChassis (g, bounds, cyan, advancedExpanded);
```

**CURRENT (wrong) — `CMakeLists.txt` line 82:**
```cmake
        resources/ui/reverbx-faceplate.png
```
Inside `juce_add_binary_data(SendBloomPresets SOURCES ...)` (lines 72-83). This generates the `BinaryData::reverbxfaceplate_png` + `BinaryData::reverbxfaceplate_pngSize` symbols.

**CURRENT (wrong) — disk:** `resources/ui/reverbx-faceplate.png` exists (379944 bytes).

**CURRENT (gap) — `source/PluginEditor.cpp` lines 170-181:**
```cpp
// Default "INITIAL PATCH" is baked into the faceplate. Only redraw for other presets.
if (presetBox.getSelectedId() != 1)
{
    g.setColour (juce::Colours::white);
    g.fillRect (70, 148, 200, 14);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (presetBox.getText(),
                76, 148, 184, 10,
                juce::Justification::centredLeft,
                false);
}
```
The comment is stale: the procedural chassis does NOT bake `INITIAL PATCH`. After Path B, the default preset's name field is blank when `getSelectedId() == 1`.

**What stays (verified aligned):**
- Cyan underline segments (lines 305-306): `(48,113)→(112,113)` and `(312,113)→(372,113)` — UI-SPEC §1.1 locks these unchanged.
- `drawLogo` (lines 13-38): `Niko`/`FX` plate — SendBloom-original, unchanged.
- `drawCyanFrame`, `drawChevronRail`, `drawControlIcons`, `drawAdvancedFanout` — all original, unchanged.
- `drawStateOverlays` (lines 257-281): all overlay coords align to procedural chassis, not the PNG.
- All `PluginEditor::resized()` coords (lines 184-216): UI-SPEC §5.1 confirms every hotspot maps to a procedural-chassis painted target with zero changes.

### 2. Legal scanner blind spots (UX-07, UX-08, UX-12)

**CURRENT (wrong) — `scripts/check-legal-metadata.sh`:**

Banned term list (lines 7-14):
```bash
BANNED_TERMS=(
  "Rainger"
  "Reverb-X"
  "Igor"
  "Pamplejuce"
  "Pamp"
  "P001"
)
```

Content scan (lines 33-42):
```bash
scan_file() {
  local file="$1"
  for term in "${BANNED_TERMS[@]}"; do
    if grep -qi "$term" "$file"; then
      echo "ERROR: Banned term '$term' found in $file" >&2
      ...
```

**Three concrete failures of the current scanner:**

1. **Literal `Reverb-X` misses variants.** `grep -qi "Reverb-X"` matches only the hyphenated spelling. It does NOT match:
   - `"REVERB X"` (line 303 of PedalFaceplatePaint.cpp — spaces, not hyphens) — actually caught by the test at V1ContractShippingPolicyTest.cpp:100, NOT by the scanner.
   - `reverbx-faceplate.png` (CMakeLists.txt line 82 — `reverbx` as one word) — NOT caught: `grep -qi "Reverb-X"` against `reverbx` fails because the hyphen is absent and there is no normalization.
   - `BinaryData::reverbxfaceplate_png` (PedalFaceplatePaint.cpp line 425) — NOT caught for the same reason.

2. **No filename scanning.** The scanner walks file *contents* only. Even if content were clean, a file named `reverbx-faceplate.png` on disk and in CMake would not be flagged by its name. UX-08 explicitly requires filename coverage.

3. **Extension filter omits binary/build files.** The `find` (line 62) restricts to `*.cpp *.h *.md *.yml *.sh *.xml *.txt`. No `*.png`, no `*.cmake`, and `CMakeLists.txt` is scanned separately (lines 45-53) but only for content, not for the filename strings inside it (actually the CMake content scan WOULD catch "reverbx" in the BinaryData SOURCES line if the banned term were "reverbx" — but it is "Reverb-X", so it misses).

**Paths currently scanned (line 55):**
```bash
for path in source tests docs/THIRD_PARTY_LICENSES.md README.md .github/workflows resources/presets resources; do
```
Note: `design-qa.md` at repo root is NOT in this list — it is currently unscanned. `docs/RELEASE_CHECKLIST.md` and `docs/CLEAN_ROOM.md` are also NOT scanned (only `docs/THIRD_PARTY_LICENSES.md` is). This is acceptable per spec §14.4 if those docs are treated as internal/release docs, but the rewrite should decide the scan surface deliberately.

**Pamplejuce allowlist (line 46):**
```bash
[[ "$line" =~ include\(Pamplejuce ]] && continue
```
Pamplejuce is the upstream CMake template (`include(PamplejuceVersion)` at CMakeLists.txt line 5). It is a build-system dependency, not a third-party product reference. The allowlist must be preserved in the rewrite.

**How the test invokes the script — `tests/ReleaseTruthTest.cpp` lines 271-278:**
```cpp
TEST_CASE ("legal metadata audit script passes on product-facing files", "[release][legal]")
{
    const auto script = findRepoRoot().getChildFile ("scripts/check-legal-metadata.sh");
    REQUIRE (script.existsAsFile());
    const auto command = "bash \"" + script.getFullPathName() + "\"";
    REQUIRE (std::system (command.toRawUTF8()) == 0);
}
```
Simple: exit 0 = pass. The scanner is also invoked by `scripts/verify-v1.sh` (gate [1/6]) and by CI. The rewrite must keep exit-code semantics identical (0 = clean, non-zero = violation).

### 3. Preset pre-v1 classification (UX-15)

**CURRENT — `resources/presets/*.xml` (8 files):**

All share identical structure. Example (`Sparkle_Verb.xml`):
```xml
<?xml version="1.0" encoding="UTF-8"?>

<SendBloomParams>
  <PARAM id="input_gain" value="0.55"/>
  <PARAM id="input_threshold" value="0.3"/>
  ...
  <PARAM id="bypass" value="0"/>
</SendBloomParams>
```

No classification field, no version attribute, no metadata element exists in any of the 8 files (verified by grep for `preset_class`, `preset-class`, `dev_state`, `pre-v1` — all empty).

**CURRENT — `source/FactoryPresets.cpp` lines 29-38:**
```cpp
bool applyEmbeddedXml (juce::AudioProcessorValueTreeState& apvts, const PresetResource& preset)
{
    const auto xml = juce::parseXML (juce::String (preset.xml, static_cast<size_t> (preset.xmlSize)));
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    return true;
}
```

This is a pure `replaceState` — no version check, no migration, no transformation. It must stay this way (UX-15: "no hidden migration promise"). The classification is *declared* in XML, not *enforced* by loader code.

**Preset name source — `FactoryPresets.cpp` lines 18-27:** names are in the `kPresetResources` array, not in the XML. The editor's `upperPresetName(0)` returns `"INITIAL PATCH"` hardcoded at `PluginEditor.cpp` line 20.

**Test that validates XML/program parity — `tests/ReleaseTruthTest.cpp` lines 441-488:** loops all 8 presets, parses XML via `juce::parseXML`, calls `setStateInformation`, and asserts every parameter matches `setCurrentProgram`. This test reads the XML root tag (`SendBloomParams`) and all `<PARAM>` entries. Adding a root attribute will not break it (it only checks `hasTagName` + param values). Adding a non-APVTS `<PARAM>` entry would also not break it (it only iterates known param IDs). But the cleanest approach is a root attribute or a non-`<PARAM>` metadata element.

### 4. Docs rewrite (UX-13, UX-16)

**CURRENT (wrong) — `design-qa.md`:**

Absolute paths with third-party name (lines 8-19):
```
- Source visual truth path: `/Users/niko/Documents/RAINGER FX/resources/ui/reverbx-faceplate.png`
- Implementation screenshot path: `/Users/niko/Documents/RAINGER FX/artifacts/state-default-final.png`
...
- Full-view comparison evidence: `/Users/niko/Documents/RAINGER FX/artifacts/design-comparison-final-state.png`
...
  - Dark mode pressed/on: `/Users/niko/Documents/RAINGER FX/artifacts/state-dark-final.png`
  ... (7 artifact paths, all absolute, all under `RAINGER FX`)
```

Stale test count (line 26):
```
- Test gate: `ctest --test-dir Builds --output-on-failure` passed, `135/135`.
```
The suite is larger now (Phases 19-24 added many `[v1][contract]` tests) and several are intentionally red until later phases. The `135/135` count is both numerically stale and conceptually wrong (BASE-06: do not hard-code totals).

Stale follow-up (line 30):
```
- None blocking. Future production packaging can remove the local development faceplate fallback because BinaryData has now been verified.
```
This is backwards now — Path B removes the fallback entirely.

`final result: passed` (line 32) — stale claim given the current red `[shipping-policy]` gates.

**CURRENT — `README.md` (verified mostly compliant):**

The `ReleaseTruthTest.cpp` "32k Color docs" test (lines 302-321) asserts:
- `readme.find("firmware-derived") != npos` — README line 14 says "not firmware-derived" ✓
- `readme.find("32,768 Hz") != npos` — README line 14 contains it ✓
- `readme.find("EEPROM") == npos` ✓
- `readme.find("bytecode") == npos` ✓

These four assertions must stay green. README must keep the strings "firmware-derived" and "32,768 Hz". No trimming of those lines.

README has no absolute paths and no third-party product names. It is compliant. Verify only; trim nothing load-bearing.

**CURRENT — `docs/CLEAN_ROOM.md`:**

Line 36: "`scripts/check-legal-metadata.sh` scans sources, tests, CI, README, and `resources/presets/*.xml` for banned third-party identifiers."

This description is stale after the scanner rewrite (which adds filename scanning and normalization). Update to reflect normalized matching + filename coverage.

No absolute paths. No third-party product names in product-facing copy (the doc describes the clean-room *position*, which is inherently about not referencing third parties). Compliant except for the scanner description.

**CURRENT — `docs/RELEASE_CHECKLIST.md` line 36:**
```
- [x] No Rainger / Reverb-X / Igor in product name, CMake metadata, sources, presets, or README
```
This line cites the third-party names literally. It is a release-tracking doc, not a product-facing surface. Spec §14.4 permits cited research in internal docs. The scanner rewrite must keep `docs/RELEASE_CHECKLIST.md` OUT of the product-facing scan surface (it currently is out — only `docs/THIRD_PARTY_LICENSES.md` is scanned). Do NOT add `docs/RELEASE_CHECKLIST.md` to the scan walk, or this line will fail the scanner.

**Truth model — `docs/RELEASE_CHECKLIST.md` lines 7, 11, 15:** already follows the runtime-discovery pattern: "Discovers ctest suite size at runtime", "do not claim a fixed N/N total", "Run `ctest --test-dir Builds -N` ... and record the discovered count". `design-qa.md` should follow this same pattern.

### 5. Pressure Mode copy (UX-06)

**CURRENT (clean) — `source/ui/AdvancedDrawer.cpp` lines 34-35:**
```cpp
pressureModeToggle.setTooltip ("Pressure Mode: when on, wet feed follows pressure; "
                               "when off, reverb stays always-on.");
```
No third-party controller name. UI-SPEC §6 locks this as the single source of truth. **No change required.**

**CURRENT (needs verification) — `source/ui/AdvancedDrawer.cpp` lines 38-43:**
```cpp
colorToggle.setTooltip ("Experimental — off by default until validated. "
                        "Steps the tank at 32,768 Hz with fixed delay-table lengths, "
                        "per-comb feedback, damping, and 9-bit quantization. "
                        "Original software — not firmware-derived. "
                        "May exhibit HF artifacts at some host rates; "
                        "host-rate reverb is the production default.");
```
The "May exhibit HF artifacts at some host rates" line is flagged by UI-SPEC §6 note 4 as potentially stale post-ProperSRC-validation (Phase 24). This is a copy-truth item, not a branding item. The planner should verify against Phase 24 DSP-06/07/08 evidence (ProperSRC pre-clear + underfill zero-fill) and trim the warning if the current ProperSRC path no longer exhibits HF artifacts at supported rates.

**No third-party controller name anywhere else in user-facing strings:** verified by reading `AdvancedDrawer.cpp`, `PedalFaceplatePaint.cpp`, `PluginEditor.cpp`. The only user-facing strings are SendBloom-original labels (`ADVANCED`, `GATE SENS`, `SEND FEEL`, `PRESSURE MODE`, `32K COLOR`, `EXTENDED STEREO`, `DIRT OS`, `DARK MODE`, `GATE`, `PRE`, `POST`, `OVERLOAD / CLIP`, `SAVE`, `NEW`, `DELETE`, `Niko`, `FX`).

## Required Changes & Approach

### Area 1: Procedural faceplate rebrand + asset removal

**Step 1 — Title wordmark swap (`PedalFaceplatePaint.cpp` line 303):**
```cpp
// BEFORE
g.drawFittedText ("REVERB X", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
// AFTER
g.drawFittedText ("SENDBLOOM", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
```
The rect `(104, 90, 218, 42)` and max scale `0.86f` stay unchanged. 9 uppercase glyphs at 34 px bold fit comfortably in 218 px (UI-SPEC §1.1).

**Step 2 — Remove third-party central art (lines 320-332):**
Delete the vertical `REVERB` letters loop (320-323), the giant `X` glyph (325-326), and the concentric ellipses loop (327-332). Replace with original SendBloom abstract geometry confined to the `(58, 195, 154, 219)` leftPanel. Permitted motifs (UI-SPEC §1.3): existing `drawChevronRail` chevron shape, concentric rounded rects, or abstract geometry. The `"OVERLOAD / CLIP"` header label (line 318) is retained verbatim.

**Step 3 — Gut the PNG load path (lines 422-433):**
Replace the entire `paintPedalFaceplate()` body with:
```cpp
void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour cyan,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded,
                          bool padPressed,
                          float padDisplayAmount)
{
    paintProceduralChassis (g, bounds, cyan, advancedExpanded);
    drawStateOverlays (g, apvts, clipActive, advancedExpanded, padPressed, padDisplayAmount);
}
```
This removes the `#include <BinaryData.h>` dependency for the PNG symbol (but BinaryData.h is still needed if other code in this TU uses it — check: only the PNG symbol was used here, so the include can stay harmlessly or be removed; leaving it is safe since BinaryData still has preset XMLs + knob.png).

**Step 4 — Remove from CMake (`CMakeLists.txt` line 82):**
Delete the `resources/ui/reverbx-faceplate.png` line from `juce_add_binary_data(SendBloomPresets SOURCES ...)`. The block becomes 9 entries (8 XMLs + knob.png). After this, `BinaryData::reverbxfaceplate_png` and `BinaryData::reverbxfaceplate_pngSize` symbols are no longer generated — Step 3 already removed all references.

**Step 5 — Delete the asset file:**
```bash
git rm resources/ui/reverbx-faceplate.png
```

**Step 6 — Fix INITIAL PATCH gap (`PluginEditor.cpp` lines 170-181):**

Option A (UI-SPEC preferred — chassis paints the default name): add to `paintProceduralChassis()` after the title/underline block, before `drawCyanFrame`:
```cpp
g.setColour (juce::Colours::black);
g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
g.drawText ("INITIAL PATCH", 70, 148, 200, 14, juce::Justification::centredLeft);
```
The editor's `if (presetBox.getSelectedId() != 1)` conditional then stays correct — the chassis paints the default, the editor paints overrides.

Option B — remove the conditional in `PluginEditor::paint()` so the editor always paints the preset name (white background + black text). The chassis paints nothing there.

Either preserves the `(64, 148, 210, 16)` hotspot and visual style.

**Step 7 — Verify hotspot alignment (UX-14):**
No coordinate changes. The UI-SPEC §5.1 table confirms every `setBounds` in `resized()` (lines 184-216) maps to a procedural-chassis painted target. Run the editor snapshot tool (`tools/EditorSnapshot.cpp`) and visually confirm each control sits on its painted target. This is `human_needed` for final visual sign-off but the coordinate parity is provable from source.

### Area 2: Legal scanner rewrite

**Design constraint:** ONE normalized banned token catches all three variants. Normalization function: lowercase, then delete all non-alphanumeric characters (spaces, hyphens, underscores, punctuation).

| Raw variant | Lowercased | Normalized (alnum only) |
|-------------|-----------|------------------------|
| `Reverb-X` | `reverb-x` | `reverbx` |
| `REVERB X` | `reverb x` | `reverbx` |
| `reverbx` | `reverbx` | `reverbx` |
| `Rainger` | `rainger` | `rainger` |

**Scanner rewrite approach:**

```bash
#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

# Normalization: lowercase, strip everything non-alphanumeric
normalize() {
  tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]'
}

# Banned concepts as normalized tokens (order irrelevant after normalization)
BANNED_TOKENS=( "reverbx" "rainger" "igor" )
# Pamplejuce is a build-template dependency, not a product reference.
# Keep it banned in product-facing content but allowlist the include() directive.
BANNED_TOKENS_PAMPLEJUICE=( "pamplejuce" "pamp" )

REQUIRED_TOKENS=( "sendbloom" "nkmo" "sblm" "nikoaudiolabs" )
```

**Content scan** — for each product-facing text file, normalize its full content and grep for each banned token:
```bash
scan_content() {
  local file="$1"
  local normalized
  normalized="$(normalize < "$file")"
  for token in "${BANNED_TOKENS[@]}"; do
    if [[ "$normalized" == *"$token"* ]]; then
      echo "ERROR: banned token '$token' in $file" >&2; exit 1
    fi
  done
}
```

**Filename scan** — for each file in the scan surface, normalize the relative path and grep for banned tokens:
```bash
scan_filename() {
  local relpath="$1"
  local normalized
  normalized="$(echo "$relpath" | normalize)"
  for token in "${BANNED_TOKENS[@]}"; do
    if [[ "$normalized" == *"$token"* ]]; then
      echo "ERROR: banned token '$token' in filename $relpath" >&2; exit 1
    fi
  done
}
```

**Scan surface (spec §14.4):** `source`, `resources`, `tests`, `README.md`, `CMakeLists.txt`, `cmake`, `cmake-local`, `.github/workflows`. Explicitly EXCLUDE `.planning/` (research/citation docs) and `docs/` internal docs that cite third parties for clean-room record (CLEAN_ROOM.md, RELEASE_CHECKLIST.md, THIRD_PARTY_LICENSES.md). If `docs/release/` is added later, include it.

**Pamplejuce allowlist:** the only legitimate occurrence is `include(PamplejuceVersion)` / `include(PamplejuceMacOS)` / `include(PamplejuceLog)` in CMakeLists.txt and the `cmake/` modules. Preserve an allowlist for CMake `include(...)` directives (regex `include\(pamplejuce` after normalization). Do NOT blanket-allowlist Pamplejuce everywhere.

**Required terms:** normalize CMakeLists.txt content and assert each required token is present:
```bash
cmake_normalized="$(normalize < CMakeLists.txt)"
for token in "${REQUIRED_TOKENS[@]}"; do
  [[ "$cmake_normalized" == *"$token"* ]] || { echo "ERROR: required '$token' missing from CMakeLists.txt"; exit 1; }
done
```

**Keep the r8brain / MIT citation check** (lines 66-71) — this is an orthogonal dependency-license audit, not a banned-term scan.

### Area 3: Preset pre-v1 classification

**Approach — root attribute on each XML (cleanest, no PARAM pollution):**
```xml
<SendBloomParams preset_class="pre-v1-dev">
  <PARAM id="input_gain" value="0.55"/>
  ...
</SendBloomParams>
```

Why a root attribute rather than a `<PARAM>`:
- `applyEmbeddedXml` does `apvts.replaceState(ValueTree::fromXml(*xml))`. A `<PARAM id="preset_class">` would enter the ValueTree as a child but map to no registered APVTS parameter — harmless but misleading (it looks like a parameter). A root attribute is clearly metadata, not a parameter.
- The parity test (`ReleaseTruthTest.cpp` lines 441-488) iterates known param IDs only, so a root attribute is invisible to it. Safe.
- `FactoryPresets.cpp` `applyEmbeddedXml` checks `xml->hasTagName(apvts.state.getType())` — the tag name `SendBloomParams` is unchanged; adding an attribute does not affect `hasTagName`. Safe.

**No loader change.** `applyEmbeddedXml` stays a pure `replaceState`. No version check, no migration, no conditional. The classification is declared in the file, not enforced at load time. This satisfies "no hidden migration promise" (UX-15, spec §14.7).

**Optional: read-only accessor for docs/tests.** If a test or doc wants to verify the classification, it can parse the XML root attribute directly. This does not require `FactoryPresets.cpp` changes — the XML is accessible via `BinaryData::*_xml`.

### Area 4: Docs rewrite

**`design-qa.md` — full rewrite:**
- Remove all `/Users/niko/...` absolute paths.
- Remove all `RAINGER FX` references.
- Replace source-of-truth path with repo-relative `source/ui/PedalFaceplatePaint.cpp` (procedural chassis is now the truth, not an external PNG).
- Replace artifact paths with repo-relative `artifacts/` or drop them (screenshots are generated, not committed — reference the generation command).
- Drop `135/135` — follow RELEASE_CHECKLIST pattern: "Run `ctest --test-dir Builds -N` and record the discovered count + date."
- Update screenshot QA references to the procedural faceplate.
- Update `final result: passed` to reflect current state truthfully (shipping-policy gates flip green after this phase).
- Reference the Path B decision and Path A deferral.

**`docs/CLEAN_ROOM.md` — scanner description update:**
Line 36: update from "scans sources, tests, CI, README, and `resources/presets/*.xml`" to reflect normalized matching + filename coverage + the full scan surface. Add a note that internal citation docs (`.planning/`, release tracking) are allowlisted per spec §14.4.

**`README.md` — verify only:**
Already compliant. The `ReleaseTruthTest` "32k Color docs" assertions (lines 302-321) pin four strings that must remain. Do not trim "firmware-derived" or "32,768 Hz". No absolute paths, no third-party names.

**`docs/RELEASE_CHECKLIST.md` — no third-party-name change needed:**
Line 36 cites the names in a release-tracking context. Keep it out of the scanner surface. Do not add `docs/RELEASE_CHECKLIST.md` to the product-facing scan walk.

### Area 5: Pressure Mode copy

**Pressure Mode tooltip — no change.** `AdvancedDrawer.cpp` lines 34-35 are canonical and clean.

**32k Color tooltip — verify and possibly trim:**
The "May exhibit HF artifacts at some host rates" line (line 41) should be checked against Phase 24 evidence. Phase 24 DSP-06/07 implemented ProperSRC output pre-clear (`std::fill` before downsample) and confirmed unwritten samples remain zero. If the ProperSRC path no longer produces HF artifacts at supported rates post-Phase-24, trim the warning. If it still does (e.g., at extreme non-standard rates), keep it. This is a copy-truth decision, not a branding decision — the planner verifies against current test evidence.

## Test Posture

### What flips green this phase

| Test | File:Line | Current | After |
|------|-----------|---------|-------|
| UX-07 faceplate string ban | `V1ContractShippingPolicyTest.cpp:100` | RED (`"REVERB X"` found) | GREEN (`"SENDBLOOM"` swap) |
| UX-08 filename ban | `V1ContractShippingPolicyTest.cpp:111` | RED (`reverbx` in CMake) | GREEN (CMake entry removed) |

### What must stay green

| Test | Tag | Verification |
|------|-----|--------------|
| Legal metadata audit | `[release][legal]` | `ReleaseTruthTest.cpp:271` — scanner rewrite must exit 0 on clean repo |
| processBlock no `setValueNotifyingHost` | `[release][realtime]` | `ReleaseTruthTest.cpp:280` — untouched this phase |
| 32k Color docs truth | `[release][verb][authentic]` | `ReleaseTruthTest.cpp:302` — README strings preserved |
| Preset XML/program parity | `[release][preset][xml]` | `ReleaseTruthTest.cpp:441` — root attribute does not break param iteration |
| Fresh load + presets `authentic_color=0` | `[release][safe]` | `ReleaseTruthTest.cpp:490,501` — no preset param values change |
| RT-01 processBlock bans | `[v1][contract][shipping-policy][RT-01]` | `V1ContractShippingPolicyTest.cpp:114` — already green from Phase 21/22 |

### Scanner invocation chain

```
ReleaseTruthTest.cpp:271  →  std::system("bash scripts/check-legal-metadata.sh")  →  exit 0
verify-v1.sh gate [1/6]   →  bash scripts/check-legal-metadata.sh                  →  exit 0
CI (.github/workflows)    →  bash scripts/check-legal-metadata.sh                  →  exit 0
```
The rewrite must preserve exit-code semantics exactly. Any false positive (e.g., the scanner flagging `docs/RELEASE_CHECKLIST.md` line 36, or the Pamplejuce `include()`) will break all three.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Variant matching | Enumerate every spelling (`Reverb-X`, `REVERB X`, `reverbx`, `Reverb X`, `REVERB-X`, ...) | Normalize both sides to alnum-only, match one token `reverbx` | Spec §14.4 mandates normalization; enumeration is brittle |
| Filename scanning | Separate content vs. filename codepaths with different term lists | One normalization function, applied to both file content and relative path | Single source of truth for banned tokens |
| Preset classification | APVTS parameter or state-version wrapper | Root XML attribute `preset_class="pre-v1-dev"` | Spec §14.7: "Do not add a complex state wrapper unless a real released-session requirement is discovered" |
| INITIAL PATCH rendering | New label component or ComboBox text style change | Paint text in chassis (Option A) or always paint in editor (Option B) | Existing pattern; no new component |

## Common Pitfalls

### Pitfall 1: Removing CMake entry before removing source references
**What goes wrong:** Deleting `resources/ui/reverbx-faceplate.png` from CMakeLists.txt line 82 removes the `BinaryData::reverbxfaceplate_png` symbol. If `PedalFaceplatePaint.cpp` lines 422-433 still reference it, compilation fails with an undefined symbol.
**How to avoid:** Gut the `paintPedalFaceplate()` loadFrom block (Step 3) in the same commit or before the CMake edit (Step 4). Order: source edit → CMake edit → file deletion.

### Pitfall 2: Scanner flags its own allowlisted build template
**What goes wrong:** The normalized scanner flags `include(PamplejuceVersion)` in CMakeLists.txt line 5 because "pamplejuce" is a banned token.
**How to avoid:** Preserve the CMake `include()` allowlist. Normalize the allowlist pattern the same way (`includepamplejuce`), and skip CMake lines matching it. Alternatively, drop Pamplejuce from the product-facing banned list and rely on it being absent from user-facing strings by convention — but the safer path is the allowlist.

### Pitfall 3: Scanner flags release-tracking docs that cite third parties
**What goes wrong:** `docs/RELEASE_CHECKLIST.md` line 36 ("No Rainger / Reverb-X / Igor ...") and potentially `docs/CLEAN_ROOM.md` describe the clean-room position by naming what is banned. A broad `docs/` scan walk flags these.
**How to avoid:** Keep the scan surface to product-facing paths only (`source`, `resources`, `tests`, `README.md`, `CMakeLists.txt`, `cmake*`, `.github/workflows`). Explicitly exclude `docs/` internal docs and `.planning/` per spec §14.4. If a `docs/release/` surface is added later, scan it; do not scan `docs/` wholesale.

### Pitfall 4: Normalization breaks required-term check
**What goes wrong:** Required term "Niko Audio Labs" contains spaces. If the scanner normalizes content but not the required-term list, "Niko Audio Labs" (with space) won't match "nikoaudiolabs" (normalized content).
**How to avoid:** Normalize both sides. The required token becomes `nikoaudiolabs`; the CMake content `COMPANY_NAME "Niko Audio Labs"` normalizes to contain `nikoaudiolabs`. Same for `sendbloom`, `nkmo`, `sblm`.

### Pitfall 5: INITIAL PATCH double-render or blank field
**What goes wrong:** If Option A (chassis paints `INITIAL PATCH`) and Option B (editor always paints) are both partially implemented, the name field either double-renders (chassis text + editor white-box overlay) or the editor's `getSelectedId() != 1` conditional leaves the default blank.
**How to avoid:** Pick one option. If Option A: chassis paints `"INITIAL PATCH"`, editor conditional stays. If Option B: chassis paints nothing there, editor conditional is removed. Do not mix.

### Pitfall 6: Preset root attribute breaks XML parsing
**What goes wrong:** Adding `<SendBloomParams preset_class="pre-v1-dev">` changes the root element. `juce::XmlDocument::parseXML` handles attributes fine, but if any code does a strict root-element comparison (not just `hasTagName`), it could break.
**How to avoid:** Verified: `FactoryPresets.cpp` line 33 uses `xml->hasTagName(apvts.state.getType())` which checks the tag name only, not attributes. `ReleaseTruthTest.cpp` line 468 uses `juce::parseXML` then `copyXmlToBinary` / `setStateInformation` — JUCE's APVTS state restore ignores unknown root attributes. Safe. But run the full preset parity test after the edit to confirm.

### Pitfall 7: Stale 32k Color "HF artifacts" warning
**What goes wrong:** Leaving the "May exhibit HF artifacts" tooltip line when Phase 24 ProperSRC pre-clear (DSP-06/07) eliminated the underfill cause makes the UI lie — a copy-truth violation in the same spirit as UX-16.
**How to avoid:** Check Phase 24 `V1ContractSrcUnderfillTest` and HF diagnostics. If ProperSRC is clean at all supported rates, trim the warning. If artifacts persist at some edge rate, keep it with evidence.

## Claude's Discretion

- **Normalization implementation:** `tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]'` (portable POSIX) vs. `sed` regex vs. bash `${var,,}` + pattern substitution. Any is fine as long as all three banned variants collapse to `reverbx` and the required terms normalize correctly.
- **Central art motif:** chevron rail extension, concentric rounded rects, or abstract geometry — all permitted by UI-SPEC §1.3. The executor picks the final look within the locked palette and panel rect.
- **INITIAL PATCH fix:** Option A (chassis paints it) or Option B (editor always paints it). UI-SPEC expresses a preference for Option A but both are acceptable.
- **Preset classification field name:** `preset_class`, `dev_class`, `session_class`, or similar — as long as the value is `pre-v1-dev` (or equivalent explicit string) and it is a root attribute or non-PARAM metadata element.
- **design-qa.md count handling:** keep a live runtime-discovered count with a verification date, or drop counts entirely and reference the RELEASE_CHECKLIST runtime-discovery command. Both satisfy UX-13; the latter is simpler.

## Deferred Ideas

- **Path A faceplate asset** (`resources/ui/sendbloom-faceplate.png`) — post-RC0, `human_needed` for Niko's asset approval. Must reuse the identical canvas (420x780), hotspot grid, and overlay coords locked in UI-SPEC §5.
- **Preset browser architecture / cloud licensing / storefront / telemetry** — post-v1 (PROJECT non-negotiable).
- **Visual redesign beyond branding** — palette, logo, layout grid are locked this release.
- **Fidelity/claim wording** (circuit emulation, exact hardware fidelity) — Phase 26 `CLAIM_STATUS.md` / ADR-V1-17.
- **Extended Stereo / Dirt OS enablement** — explicitly deferred; UI stays disabled.
- **Separate clean-room allowlist file** — if the number of cited-research docs grows, factor the allowlist into a manifest file instead of hardcoded path exclusions. Not needed at v1 scale.

## Code Examples

### Title wordmark swap (UX-09) [CITED: PedalFaceplatePaint.cpp line 303, UI-SPEC §1.1]
```cpp
// Before
g.drawFittedText ("REVERB X", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
// After — rect and scale unchanged
g.drawFittedText ("SENDBLOOM", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
```

### paintPedalFaceplate after Path B (UX-10) [CITED: PedalFaceplatePaint.cpp lines 422-436, UI-SPEC §2]
```cpp
void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour cyan,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded,
                          bool padPressed,
                          float padDisplayAmount)
{
    paintProceduralChassis (g, bounds, cyan, advancedExpanded);
    drawStateOverlays (g, apvts, clipActive, advancedExpanded, padPressed, padDisplayAmount);
}
```

### Scanner normalization core (UX-12) [CITED: spec §14.4]
```bash
normalize() { tr '[:upper:]' '[:lower:]' | tr -cd '[:alnum:]'; }

# Content check
content_norm="$(normalize < "$file")"
for token in "${BANNED_TOKENS[@]}"; do
  [[ "$content_norm" == *"$token"* ]] && { echo "ERROR: $token in $file" >&2; exit 1; }
done

# Filename check
path_norm="$(echo "$relpath" | normalize)"
for token in "${BANNED_TOKENS[@]}"; do
  [[ "$path_norm" == *"$token"* ]] && { echo "ERROR: $token in filename $relpath" >&2; exit 1; }
done
```

### Preset classification (UX-15) [CITED: spec §14.7]
```xml
<?xml version="1.0" encoding="UTF-8"?>

<SendBloomParams preset_class="pre-v1-dev">
  <PARAM id="input_gain" value="0.55"/>
  ...
</SendBloomParams>
```

## Environment Availability

No new external dependencies. CMake + Catch2 + JUCE 8 already required by repo. The scanner rewrite uses only POSIX `tr` / `grep` / `find` — no new bash dependencies.

## Security Domain

No new attack surface. No secrets, no network I/O, no new user input paths. The scanner is a build-time/dev tool, not shipped in the plugin binary. Preset XMLs are embedded read-only resources.

## Package Legitimacy Audit

No new external packages. The `reverbx-faceplate.png` deletion removes a third-party-derived asset (clean-room policy gain, not loss).

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| A1 | `BinaryData::reverbxfaceplate_png` is referenced only at PedalFaceplatePaint.cpp:425 | §1 | Compile error if referenced elsewhere — verified by grep this session (only 2 hits, both in the loadFrom block) |
| A2 | Root XML attribute does not break APVTS `replaceState` / `setStateInformation` | §3 | Preset parity test fails — mitigated by running ReleaseTruthTest:441 after edit |
| A3 | `design-qa.md` is not currently in the scanner walk | §4 | Scanner rewrite would flag its `RAINGER FX` paths — verified: line 55 path list does not include root-level .md files except README.md |
| A4 | `docs/RELEASE_CHECKLIST.md` is not in the scanner walk | §4 | Scanner flags line 36 — verified: only `docs/THIRD_PARTY_LICENSES.md` is scanned from docs/ |
| A5 | Pamplejuce is the only build-template that needs allowlisting | §2 | Additional `include()` lines flagged — verified: CMakeLists.txt has 3 Pamplejuce includes (lines 5, 8, 22), all covered by the same allowlist pattern |

## Sources

### Primary (HIGH confidence)
- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` — §14.3 Path A/B, §14.4 scanner normalization, §14.5 UI truth, §14.7 state compatibility, §14.8 design QA [CITED]
- `.planning/phases/SENDBLOOM-25-presets-ui-branding-release-truth/25-CONTEXT.md` — locked decisions D-01…D-05 [CITED]
- `.planning/phases/SENDBLOOM-25-presets-ui-branding-release-truth/25-UI-SPEC.md` — wordmark contract §1.1, central art §1.3, hotspot alignment §5.1, INITIAL PATCH gap §5.2, Pressure Mode copy §6 [CITED]
- `source/ui/PedalFaceplatePaint.cpp` — lines 13-38 (logo), 283-403 (procedural chassis), 413-436 (paintPedalFaceplate) [VERIFIED: source read]
- `source/PluginEditor.cpp` — lines 14-15 (canvas), 17-23 (preset name), 159-182 (paint + INITIAL PATCH gap), 184-216 (hotspots) [VERIFIED: source read]
- `source/ui/AdvancedDrawer.cpp` — lines 34-35 (Pressure Mode tooltip), 38-43 (32k Color tooltip) [VERIFIED: source read]
- `source/FactoryPresets.cpp` — lines 18-27 (resources), 29-38 (applyEmbeddedXml), 64-70 (applyPreset) [VERIFIED: source read]
- `scripts/check-legal-metadata.sh` — lines 7-14 (banned terms), 33-42 (scan_file), 45-53 (CMake scan), 55-64 (path walk) [VERIFIED: source read]
- `tests/V1ContractShippingPolicyTest.cpp` — lines 91-101 (UX-07 assertion), 103-112 (UX-08 assertion), 114-127 (RT-01 assertions) [VERIFIED: source read]
- `tests/ReleaseTruthTest.cpp` — lines 271-278 (scanner invocation), 302-321 (32k Color docs), 441-488 (preset parity), 490-516 (authentic_color safe) [VERIFIED: source read]
- `CMakeLists.txt` — lines 72-83 (BinaryData SOURCES) [VERIFIED: source read]
- `design-qa.md` — lines 8-19 (absolute paths), 26 (stale count) [VERIFIED: source read]
- `docs/RELEASE_CHECKLIST.md` — lines 7, 15 (runtime discovery pattern), 36 (third-party citation) [VERIFIED: source read]
- `docs/CLEAN_ROOM.md` — line 36 (scanner description) [VERIFIED: source read]
- `README.md` — lines 14, 60-65 (compliance) [VERIFIED: source read]
- `resources/presets/Sparkle_Verb.xml` — root tag + PARAM structure [VERIFIED: source read]
- `resources/ui/` — `reverbx-faceplate.png` (379944 bytes), `knob.png` (2107 bytes) [VERIFIED: ls]

### Secondary
- `.planning/ROADMAP.md` — Phase 25 goal + success criteria, UX-06…16 coverage [CITED]
- `.planning/REQUIREMENTS.md` — UX-06…16 definitions [CITED]
- `source/ui/PedalFaceplatePaint.h` — public API (paintPedalFaceplate signature) [VERIFIED: source read]
- `source/ui/AdvancedDrawer.h` — toggle members [VERIFIED: source read]
- `scripts/verify-v1.sh` — gate [1/6] legal-metadata invocation [VERIFIED: source read]
- `docs/THIRD_PARTY_LICENSES.md` — r8brain MIT citation [VERIFIED: source read]
- `.github/workflows/build_and_test.yml` — CI runs legal audit (no reverbx refs) [VERIFIED: grep]

## Confidence

HIGH — all five defect loci are localized with exact line-level evidence verified in source this session. The UI-SPEC locks every coordinate, color, and font size, leaving no ambiguity in the faceplate rebrand. The scanner rewrite has one clear design constraint (normalize to alnum-only, one token catches three variants). Preset classification is a single root-attribute edit with no loader change. The two `[shipping-policy]` assertions flip green by material removal, not scanner weakening.

### Key Findings
- `PedalFaceplatePaint.cpp` has exactly 4 third-party-derived art blocks (lines 303, 320-323, 325-326, 328-332) plus the loadFrom block (422-433); all are cleanly removable without touching any hotspot or overlay coordinate.
- `BinaryData::reverbxfaceplate_png` is referenced only at PedalFaceplatePaint.cpp:425 (verified by grep) — gutting the loadFrom block makes the CMake removal compile-safe.
- The current scanner's literal `grep -qi "Reverb-X"` cannot catch `reverbx` (one word) or `REVERB X` (spaces) — normalization is the only fix that covers all variants with one token.
- `design-qa.md` is NOT in the current scanner walk (only README.md is scanned from root), so its `RAINGER FX` paths evade the current scanner entirely — the rewrite must decide to include it or rely on the doc rewrite alone.
- Preset XML root-attribute addition is safe: `FactoryPresets.cpp` uses `hasTagName` (not attribute comparison) and the parity test iterates known param IDs only.
- The `INITIAL PATCH` gap is a real post-Path-B bug: the procedural chassis does not paint it, so the default preset's name field would be blank unless Option A or B is applied.

### File Created
`.planning/phases/SENDBLOOM-25-presets-ui-branding-release-truth/25-RESEARCH.md`

### Plan Shape Summary
1. **Plan 01 — Faceplate rebrand + asset removal + scanner rewrite:** title swap, central art removal, loadFrom gut, CMake entry removal, PNG deletion, INITIAL PATCH fix (UX-09/10/14); scanner normalization + filename scan rewrite (UX-07/08/12). Flips both `[shipping-policy]` assertions green. Tightly coupled — the test at V1ContractShippingPolicyTest.cpp:100 requires the string swap AND the test at :111 requires the CMake removal; the scanner rewrite ensures the `[release][legal]` test stays green.
2. **Plan 02 — Preset classification + docs rewrite:** root attribute on 8 XMLs (UX-15); design-qa.md rewrite to repo-relative + runtime discovery (UX-13); CLEAN_ROOM.md scanner description update (UX-16); README verify (UX-16). Preserves preset parity and `[release][safe]` tests.
3. **Plan 03 — Copy verification + human gates:** confirm Pressure Mode tooltip verbatim (UX-06); verify/trim 32k Color "HF artifacts" warning against Phase 24 evidence; document Path A deferral and UX-11 `human_needed` status.

### Ready for Planning
