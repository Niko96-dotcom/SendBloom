# Phase 25: Presets, UI, Branding & Release Truth - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Smart discuss (autonomous)

<domain>
## Phase Boundary

Shipping surfaces, scanner, presets, and docs match SendBloom's clean-room position and truthful Pressure Mode copy. Covers UX-06…16. The phase removes the third-party reference faceplate from the shipping binary, makes the procedural chassis the production faceplate (title `SENDBLOOM`), strengthens the legal scanner with punctuation/spacing/case normalization and filename coverage, classifies existing preset sessions as pre-v1 development state, and rewrites docs to use repo-relative paths with current verified evidence.

This phase greens `[v1][contract][shipping-policy]` (currently red, exit 42). It does NOT introduce new DSP, retune the reverb, or add new features.

</domain>

<decisions>
## Implementation Decisions

### Faceplate asset — Path B (procedural, Path A deferred)
- No approved original asset exists yet. Ship the **procedural chassis as the production faceplate** (spec §14.3 Path B).
- Remove `resources/ui/reverbx-faceplate.png` from BinaryData / CMake (UX-10). Delete the asset file from the repo (it is a third-party product faceplate — clean-room policy prohibits shipping it).
- Procedural fallback title becomes `SENDBLOOM` (not `REVERB X`) — UX-09. Remove the standalone `X` and any reference-derived exact copy elements from `PedalFaceplatePaint.cpp`.
- Preserve control usability: editor hotspots/overlays must remain hittable and aligned against the procedural layout (UX-14). The procedural chassis already defines the layout; hardcoded hotspot coords in `PluginEditor.cpp` must be validated against it.
- Asset approval (a future Niko-approved `sendbloom-faceplate.png`, Path A) stays `human_needed` and is explicitly deferred to post-RC0. Document this intent.

### Legal scanner strengthening (spec §14.4)
- Rewrite `scripts/check-legal-metadata.sh` around **normalized matching**: lowercase, then strip spaces/hyphens/underscores/punctuation, then compare. This catches all three real-world variants (`Reverb-X`, `REVERB X`, `reverbx`) with one normalized token.
- Extend scanning to **filenames** (asset names, BinaryData symbols, CMake references) in addition to file contents (UX-08, UX-12).
- Keep banned-term and required-term lists; normalize both sides.
- `V1ContractShippingPolicyTest` green after the faceplate removal + scanner strengthening (the two failing UX-07/UX-08 assertions flip to pass).

### Presets — pre-v1 classification (UX-15)
- Existing factory presets are explicitly classified as **pre-v1 development state**. Add a classification field/attribute to each preset XML (e.g. a `<PARAM id="preset_class" value="pre-v1-dev"/>` or a metadata attribute on the root).
- **No hidden migration promise**: any preset-loading code must not silently rewrite or "migrate" sessions in a way that implies a compatibility guarantee. Loading is best-effort; classification is declared.
- All presets keep `authentic_color="0"` (already true — DSP-14/15, preserved).

### Docs — repo-relative paths, verified evidence only (UX-13, UX-16)
- Rewrite `design-qa.md` to use **portable repo-relative paths** (not `/Users/nikolay/...` absolute paths) and **current evidence**. Remove the stale `135/135` count (suite is larger and several `[v1][contract]` tests are intentionally red until later phases).
- `README.md` and `docs/CLEAN_ROOM.md`: describe **only verified behavior**. No firmware/circuit-emulation/exact-fidelity claims (those depend on Phase 26 `CLAIM_STATUS.md`). README already mostly compliant — verify and trim any over-claim.
- Screenshot QA references updated to the procedural faceplate.

### Claude's Discretion
- Exact normalized-token algorithm (regex strip vs. transliteration) as long as all three variants match.
- Exact preset classification field name/placement as long as it is explicit, machine-readable, and declares pre-v1-dev with no migration promise.
- Whether `design-qa.md` keeps a live test-count or drops counts entirely (RELEASE_CHECKLIST.md already prefers runtime discovery over fixed totals — prefer that pattern).

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- `source/ui/PedalFaceplatePaint.cpp` — `paintProceduralChassis()` (line 283) is the no-asset fallback; becomes the production paint path. `drawStateOverlays` (line 257) handles dark/gate/footswitch/clip overlays.
- `source/ui/AdvancedDrawer.cpp` / `.h` — Pressure Mode toggle + tooltip; copy is already clean (no third-party naming). UX-06 likely already satisfied — verify only.
- `scripts/check-legal-metadata.sh` — existing scanner to rewrite (content-only, hyphenated-term, no filename scan).
- `tests/V1ContractShippingPolicyTest.cpp` — the two UX-07/UX-08 assertions that are intentionally red; flip green after this phase.
- `tests/ReleaseTruthTest.cpp` — drives the legal script + docs-truth checks.
- `docs/RELEASE_CHECKLIST.md` — the truth model (runtime test-count discovery, no fixed N/N); use as the pattern for `design-qa.md`.

### Established Patterns
- V1 contract tests use `[v1][contract][...]` tags matching Phase 19 harness style.
- Docs use repo-relative markdown links (README already compliant).
- Presets are XML embedded via `juce_add_binary_data` (8 factory presets in `resources/presets/`).

### Integration Points
- `CMakeLists.txt` lines 72-83 — BinaryData asset list (remove `reverbx-faceplate.png`).
- `source/PluginEditor.cpp` `resized()` (lines 184-216) — hardcoded hotspot coords measured against the 420x780 faceplate; validate against procedural layout.
- `source/FactoryPresets.cpp` — preset loading; add classification read.

</code_context>

<specifics>
## Specific Ideas

- Path B now, Path A later: ship procedural as production this phase; an approved `sendbloom-faceplate.png` (Path A) can replace it post-RC0. Document this deferral explicitly so it is not mistaken for a permanent decision.
- The three banned-name variants (`Reverb-X`, `REVERB X`, `reverbx`) must all be caught by ONE normalized scanner token — this is the core scanner design constraint.

</specifics>

<deferred>
## Deferred Ideas

- Path A faceplate asset (Niko-approved original `sendbloom-faceplate.png`) — post-RC0, pending asset approval (`human_needed`).
- New preset-browser architecture, cloud licensing, storefront, telemetry — post-v1 (PROJECT non-negotiable).
- Visual redesign beyond required original branding — branding only this release.
- Fidelity/claim wording tied to hardware comparison — Phase 26 `CLAIM_STATUS.md`.

</deferred>
