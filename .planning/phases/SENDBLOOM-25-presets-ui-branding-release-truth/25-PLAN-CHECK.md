---
phase: 25-presets-ui-branding-release-truth
status: approved
reviewer: gsd-plan-checker
checked: 2026-07-12
plans_reviewed:
  - 25-01-PLAN.md
  - 25-02-PLAN.md
  - 25-03-PLAN.md
sources_cross_checked:
  - source/ui/PedalFaceplatePaint.cpp
  - source/PluginEditor.cpp
  - source/ui/AdvancedDrawer.cpp
  - source/FactoryPresets.cpp
  - scripts/check-legal-metadata.sh
  - tests/V1ContractShippingPolicyTest.cpp
  - tests/ReleaseTruthTest.cpp
  - CMakeLists.txt
  - design-qa.md
  - docs/CLEAN_ROOM.md
---

# Phase 25 Plan Check — Presets, UI, Branding & Release Truth

Independent review of the three Phase 25 PLAN.md files against the locked CONTEXT
decisions, the UI-SPEC design contract, the RESEARCH findings, REQUIREMENTS
(UX-06..16), and the ROADMAP success criteria. Source-line claims were
spot-verified against the working tree on 2026-07-12.

## Summary verdict

All three plans are **executable, complete, and internally consistent**. Every
locked decision (D-01..D-05) is honored. Every requirement UX-06..16 is covered.
The critical Pitfall-1 ordering (source gut → CMake edit → file delete) is
correctly sequenced. The `[shipping-policy]` flip is by material removal (string
swap + CMake entry removal + PNG deletion), never by scanner weakening. The
plans are ready for execution.

One non-blocking observation about the dropped `P001` banned token and a few
minor clarifications are listed in the per-dimension sections below.

---

## Dimension 1 — Requirement coverage

**PASS.** Every requirement UX-06..16 is owned by at least one task; none orphaned.

| Req | Owning plan / task | Evidence |
|-----|--------------------|----------|
| UX-06 | 25-03 T1 (Pressure Mode tooltip verify), T2 (32k Color HF-warning trim/retain) | Plan 03 must_haves pin the verbatim canonical copy; D-05 locks it |
| UX-07 | 25-01 T1 (wordmark swap), T2 (central art removal), T7 (normalized content scan) | Flips `V1ContractShippingPolicyTest.cpp:100` assertion green |
| UX-08 | 25-01 T4 (CMake entry removal), T7 (filename scan) | Flips `V1ContractShippingPolicyTest.cpp:111` assertion green |
| UX-09 | 25-01 T1 (SENDBLOOM wordmark at unchanged rect 104,90,218,42) | UI-SPEC §1.1 locked |
| UX-10 | 25-01 T3 (gut loadFrom), T4 (CMake), T5 (git rm PNG) | Three-step asset removal contract |
| UX-11 | 25-03 T3 (Path A deferral documented human_needed) | Path B satisfies; asset approval stays human_needed (BASE-08) |
| UX-12 | 25-01 T7 (normalize both sides + filename walk + `*.png`/`*.cmake` exts) | D-02 locked |
| UX-13 | 25-02 T2 (design-qa.md repo-relative + runtime test discovery) | D-04 locked; RELEASE_CHECKLIST pattern |
| UX-14 | 25-01 T6 (INITIAL PATCH gap fix), T8 (hotspot/overlay alignment verify) | UI-SPEC §5.1/§5.3 tables locked |
| UX-15 | 25-02 T1 (preset_class="pre-v1-dev" root attribute on 8 XMLs) | D-03 locked; applyEmbeddedXml stays pure replaceState |
| UX-16 | 25-02 T3 (CLEAN_ROOM scanner description), T4 (README verify-only) | D-04 locked |

Coverage is exhaustive with no orphans and no duplicates.

---

## Dimension 2 — Task executability

**PASS.** Tasks cite concrete file paths, line ranges, and runnable commands. Spot
verification of the load-bearing claims:

- `source/ui/PedalFaceplatePaint.cpp:303` is indeed `g.drawFittedText ("REVERB X", 104, 90, 218, 42, ...)`. Task 1's 1:1 swap target is correct.
- Vertical REVERB loop (320-323), giant X glyph (325-326), concentric ellipses loop (327-332) all verified at the cited lines. Task 2's deletion ranges are accurate.
- `paintPedalFaceplate` body (422-436) matches the Path B gut description exactly, including the `BinaryData::reverbxfaceplate_png` symbol at line 425. Task 3's replacement body is correct.
- `CMakeLists.txt:82` is `resources/ui/reverbx-faceplate.png` inside `juce_add_binary_data(SendBloomPresets SOURCES ...)`. Task 4's edit is precise.
- `source/PluginEditor.cpp:170-181` contains the stale comment and the `presetBox.getSelectedId() != 1` conditional. Task 6's gap analysis is accurate.
- `source/FactoryPresets.cpp:29-38` `applyEmbeddedXml` is a pure `replaceState` with `hasTagName` check. Task 1 of Plan 02 correctly leaves it untouched.
- `source/ui/AdvancedDrawer.cpp:34-35` (Pressure Mode tooltip) and `38-43` (32k Color tooltip) match verbatim the strings referenced in Plan 03.
- `scripts/check-legal-metadata.sh` confirmed: literal `grep -qi "Reverb-X"`, content-only scan, no filename walk, `*.png`/`*.cmake` excluded from `find`. The rewrite rationale is sound.

**Build-target verification (non-trivial):** The plans invoke
`cmake --build Builds --target SendBloom_All`. `SendBloom_All` does not appear
in the top-level `CMakeLists.txt` `add_custom_target` calls (only
`add_executable(EditorSnapshot ...)` is local). However, `cmake --build Builds
--target help` confirms `SendBloom_All`, `SendBloom_AU`, `SendBloom_VST3`,
`SendBloomPresets`, `Tests`, and `EditorSnapshot` all exist as configured
targets (provided by the Pamplejuce build template includes at CMakeLists.txt:5,
8, 22). All build commands in the plans are runnable.

**Test-filter verification:** `[v1][contract][shipping-policy][UX-07]`,
`[UX-08]`, `[RT-01]`, `[release][legal]`, `[release][safe]`,
`[release][preset][xml]`, `[release][verb][authentic]` and the Phase 19-24
regression filter names all appear in the test sources or match the harness tag
convention used by prior phases.

**Scanner invocation chain:** `ReleaseTruthTest.cpp:271-278` invokes the script
via `std::system`, exit 0 = pass. The plan preserves this contract.

---

## Dimension 3 — TDD where applicable

**PASS (with one note).** Tasks that add or change behavior carry `tdd="true"`:

- 25-01 T1 (wordmark swap) — `tdd="true"`; the UX-07 test already exists as an intentionally-red assertion, so the red→green flip is genuine.
- 25-01 T6 (INITIAL PATCH gap) — `tdd="true"`; the existing editor paint is the behavior under change.
- 25-01 T7 (scanner rewrite) — `tdd="true"`; behavior is validated by `[release][legal]` plus explicit scratch negative tests for all three spelling variants and a banned-path filename.
- 25-02 T1 (preset classification) — `tdd="true"`; `[release][preset][xml]` and `[release][safe]` are the stay-green guard.
- 25-03 T2 (32k Color warning) — `tdd="true"`; gated by `[release][verb][authentic]` and `[shipping-policy]`.

**Note (non-blocking):** Tasks 2, 3, 4, 5 of 25-01 are marked `type="auto"`
without `tdd="true"`, which is appropriate — they are deletion/gut operations
whose correctness is asserted by Task 8's regression bundle rather than by a
new test. The intentionally-red UX-07/UX-08 assertions serve as the red side of
the TDD loop for the overall plan. This is a defensible posture.

---

## Dimension 4 — Dependency / ordering correctness

**PASS.**

**Plan 01 internal ordering (Pitfall 1 — the highest-risk sequencing):**
- T3 (gut `paintPedalFaceplate`, remove `BinaryData::reverbxfaceplate_png` reference) is correctly ordered BEFORE T4 (remove CMake entry). T3 step 3 and T4 step 3 both call out that the source gut must land in the same commit or before the CMake edit, else the dangling `BinaryData::reverbxfaceplate_png` symbol breaks compilation.
- T5 (`git rm` the PNG) is correctly ordered AFTER T4 (CMake entry removal). Deleting the file while CMake still references it would break `juce_add_binary_data` reconfiguration.
- The committed order is therefore: T1/T2 (paint edits) → T3 (source gut) → T4 (CMake) → T5 (file delete). This matches RESEARCH Pitfall 1 exactly.

**Wave dependencies:**
- 25-01: `wave: 1`, `depends_on: []` — correct; the faceplate rebrand + scanner rewrite is the foundation. UX-07/08/09/10/12/14 must flip green before docs (Plan 02) can truthfully describe them.
- 25-02: `wave: 2`, `depends_on: ["01"]` — correct. design-qa.md rewrite references the procedural chassis as the source of truth and states "shipping-policy gates flip green after Plan 01"; this requires Plan 01 to have landed. CLEAN_ROOM.md scanner description depends on the Plan 01 scanner rewrite being final.
- 25-03: `wave: 2`, `depends_on: ["01", "02"]` — correct. The final release-truth confirmation (T4) runs the full green/stay-green matrix across all three plans and requires both prior plans' artifacts. 25-SUMMARY.md aggregates results from 01 and 02.

**Parallelism note:** Plans 02 and 03 are both `wave: 2`. They touch disjoint
files (02: presets + design-qa + CLEAN_ROOM; 03: AdvancedDrawer + 25-SUMMARY),
so they can execute in parallel after 01. The only shared artifact is
`25-SUMMARY.md`, which Plan 03 Task 3 creates and which Plan 02 does not touch.
No conflict.

---

## Dimension 5 — Acceptance criteria

**PASS.** Every task has verifiable acceptance criteria. Each criterion is either
a `rg`/`grep` source check, a build command, or a test-filter exit code.

**Critical policy check — the `[shipping-policy]` flip is by material removal,
not scanner weakening:**
- UX-07 (faceplate string ban) flips green because Task 1 replaces `"REVERB X"` with `"SENDBLOOM"` and Task 2 removes the vertical letters / giant X. The scanner is not involved in this assertion (it's a direct `faceplate.find("REVERB X") == npos` check in `V1ContractShippingPolicyTest.cpp:100`).
- UX-08 (filename ban) flips green because Task 4 removes the CMake BinaryData entry. The assertion is `containsIgnoreCase(cmake, "reverbx") == false` at `V1ContractShippingPolicyTest.cpp:111` — a direct CMake-content check, independent of the scanner.
- The scanner rewrite (Task 7) STRENGTHENS detection (adds normalization + filename walk). It cannot weaken the UX-07/UX-08 assertions because those assertions do not invoke the scanner.
- Plan 01 must_haves explicitly state: *"[v1][contract][shipping-policy][UX-07] and [UX-08] assertions flip green by material removal"* and the threat register T-25-01-T (Tampering: scanner weakened to pass) is mitigated by "Green only by material removal (Tasks 1-5); scanner normalized on both sides; product-facing paths never allowlisted."

This is the correct posture and matches UI-SPEC §8 anti-goal: *"Do NOT green `[shipping-policy]` by weakening the scanner or allowlisting the banned product-facing paths."*

**Stay-green guardrails:** Plan 01 T8, Plan 02 T5, and Plan 03 T4 each run a
regression bundle covering `[release][legal]`, `[release][safe]`,
`[release][preset][xml]`, `[release][verb][authentic]`, RT-01, and the Phase
19-24 contract filters. authentic_color=0 preservation is explicitly verified
(no preset param is touched in any of the three plans).

---

## Dimension 6 — Threat model

**PASS.** All seven RESEARCH pitfalls are addressed with explicit mitigations:

| Pitfall | Addressed in |
|---------|--------------|
| 1 — Dangling BinaryData symbol | 25-01 T3 step 3 + T4 step 3 + T5 step 3 (ordering calls); threat T-25-01-D |
| 2 — Pamplejuce allowlist | 25-01 T7 action 7 (skip `include(pamplejuce` lines, normalized); do NOT blanket-allowlist); threat T-25-01-R |
| 3 — Docs citation exclusion | 25-01 T7 action 10 (exclude docs/RELEASE_CHECKLIST.md and CLEAN_ROOM.md); threat T-25-01-I; Plan 02 T5 confirms design-qa/CLEAN_ROOM not in surface |
| 4 — Normalize both sides for required terms | 25-01 T7 action 3-4 (REQUIRED_TOKENS normalized; `nikoaudiolabs` matches normalized CMake); threat T-25-01-DoS |
| 5 — INITIAL PATCH double-render | 25-01 T6 action 2 (pick exactly one of Option A/B); threat T-25-01-E |
| 6 — Preset root attribute breaks parse | 25-02 T1 read_first cites `hasTagName` ignores attributes + parity test iterates known IDs; threat T-25-02-T |
| 7 — Stale 32k Color HF warning | 25-03 T2 (verify against Phase 24 DSP-06/07/08, default trim); threat T-25-03-I |

**Additional STRIDE coverage:** The plans model spoofing (third-party name
re-shipping), tampering (scanner weakening, hidden migration), DoS (build break,
stale counts), information disclosure (docs flagging their own citation),
elevation (UX-11 human gate silently passed), and repudiation (green claimed
without evidence). Each has a concrete mitigation tied to a task.

The threat models are not boilerplate — they reference the specific locked
decisions and pitfalls. This is above the bar.

---

## Dimension 7 — Scope discipline

**PASS.** No scope creep beyond UX-06..16.

- No DSP changes. Plans explicitly forbid touching parameter values,
  `authentic_color`, the reverb tank, or ProperSRC.
- No reverb retuning. The 32k Color tooltip trim (Plan 03 T2) is a copy-truth
  edit (remove a stale warning line), not a DSP change; it explicitly preserves
  the off-by-default behavior and the authentic_color parameter (DSP-14/15).
- No new features. Extended Stereo and Dirt OS stay disabled (verified:
  `AdvancedDrawer.cpp:48-49` `setEnabled(false)` untouched).
- No coordinate changes. UI-SPEC §5 locks the canvas (420×780), hotspot grid,
  and overlay coords; the plans repeatedly enforce "zero coordinate changes."
- No Path A asset implementation. Plan 03 T3 defers Path A post-RC0 with
  human_needed, matching UI-SPEC §8 anti-goal.
- Out-of-scope items (preset browser, cloud licensing, fidelity claim wording)
  are correctly deferred to Phase 26+ or post-v1.

The only edit to user-facing DSP-adjacent copy is the 32k Color tooltip warning,
and the plan correctly frames it as a copy-truth decision requiring Phase 24
evidence rather than a branding decision.

---

## Dimension 8 — Consistency with locked context

**PASS.** All three plans honor the UI-SPEC and CONTEXT decisions.

**UI-SPEC Path B (procedural chassis = production):**
- Plan 01 T3 makes `paintProceduralChassis` the sole paint path (UI-SPEC §2 locked body).
- Plan 03 T3 documents Path B as production and Path A as deferred (UI-SPEC §7, §8).

**SENDBLOOM wordmark:**
- Plan 01 T1 uses the exact locked rect `(104, 90, 218, 42)`, font `FontOptions(34.0f, bold)`, colour black, centred, max scale `0.86f`, and preserves the cyan underline segments `(48,113)→(112,113)` and `(312,113)→(372,113)`. All match UI-SPEC §1.1.

**Locked palette:**
- Plan 01 T2 restricts the central-art replacement to cyan/black/white-gradient/optional-accent-orange. No new `juce::Colour` literals outside UI-SPEC §3. Acceptance criterion: `rg` for new colour literals.

**Hotspot alignment:**
- Plans make zero edits to `PluginEditor::resized()`. Plan 01 T8 diffs the coords against UI-SPEC §5.1 to confirm zero drift. Verified: the current `resized()` bounds match the §5.1 table.

**CONTEXT D-01..D-05:**
- D-01 (Path B, SENDBLOOM, original art, delete asset, Path A deferred) → Plan 01 T1-T6, Plan 03 T3.
- D-02 (normalized matching + filename coverage, both sides, allowlist for cited research) → Plan 01 T7.
- D-03 (pre-v1 classification, root attribute, no migration, applyEmbeddedXml pure) → Plan 02 T1.
- D-04 (repo-relative paths, runtime discovery, verified behavior only) → Plan 02 T2, T3, T4.
- D-05 (Pressure Mode verbatim, 32k Color warning verify/trim) → Plan 03 T1, T2.

**Spec 14.7 reconciliation (non-blocking note):** Spec §14.7 says both *"no
hidden state-version migration is promised"* AND *"factory presets are
explicitly migrated."* Plan 02 reconciles these correctly: classification is
*declared* in XML (the "explicitly migrated/classified" clause) while the loader
stays a pure `replaceState` (the "no hidden migration promise" clause). The
plan's must_haves make this distinction explicit. No conflict.

---

## Non-blocking observations

These do not block execution. The executor may address them at discretion.

1. **`P001` banned token dropped without explicit note.** The current scanner
   bans `P001` (a Pamplejuce concept, line 13). Plan 01 T7 action 2 defines
   `BANNED_TOKENS` as `"reverbx" "rainger" "igor"` and handles Pamplejuce/Pamp
   via allowlist, but does not explicitly mention `P001`. Since `P001` is a
   Pamplejuce build-template concept (like Pamp), it is implicitly covered by
   the same rationale as Pamplejuce. If the executor wants to be explicit,
   adding a one-line comment in the rewritten scanner ("P001 dropped: it is a
   Pamplejuce concept, not a product reference; covered by the include()
   allowlist rationale") would close the loop. Low risk either way — `P001` does
   not appear in any product-facing source (verified by grep: only hit is the
   current scanner's own banned-term list).

2. **Plan 02 T1 acceptance criterion string.** The criterion
   `rg -n 'authentic_color" value="0"' resources/presets/ returns 8 matches`
   was verified against the working tree and returns exactly 8 matches. Good.
   But the executor should use the actual XML attribute quoting style when
   running the check (the XMLs use `value="0"` with double quotes, which the
   criterion already reflects).

3. **Plan 03 T2 evidence dependency.** The 32k Color warning trim/retain
   decision depends on reading Phase 24 ProperSRC evidence (DSP-06/07/08). The
   plan correctly identifies these but does not cite a specific test file path
   for the HF imaging gate. The executor should `rg -n "HF|imaging|underfill"
   tests/` to locate the exact Phase 24 test before deciding trim vs retain.
   This is research guidance, not a plan defect.

4. **`cmake-local` in scan surface.** Plan 01 T7 action 8 lists `cmake-local`
   as a scan-surface dir. Verified `cmake-local/` exists at repo root alongside
   `cmake/`. Good — both are product-facing build-template dirs and should be
   scanned (with the Pamplejuce allowlist applied).

5. **Plan 02 T2 `design-qa.md` rewrite is open-ended.** The task gives the
   executor discretion on structure ("Keep the Findings/Open
   Questions/Implementation Checklist structure or refactor cleanly"). This is
   appropriate for a docs rewrite but means the acceptance criteria are
   primarily negative (no `/Users/`, no `RAINGER`, no `135/135`) plus positive
   presence checks (references `PedalFaceplatePaint.cpp`, documents
   `ctest --test-dir Builds -N`). The executor should ensure the §14.8 item
   list (default state, Pressure Mode off/on, pressure pressed/released, gate
   pre/post, dark on/off, bypass, advanced drawer, clip LED, original-branding
   sign-off) is reflected or explicitly marked human_needed.

---

## Blocking issues

**None.**

No plan contains a defect that would prevent execution, cause incorrect
results, or violate a locked decision. The source→CMake→delete ordering is
correct. The `[shipping-policy]` flip is by material removal. All requirements
are covered. All build commands are runnable. All file/line references verified
against the working tree.

---

## PLAN CHECK APPROVED

The three Phase 25 plans (25-01, 25-02, 25-03) are approved for execution. They
are tightly scoped, correctly ordered, honor every locked decision (D-01..D-05)
and UI-SPEC contract, address all seven research pitfalls, and flip the two
intentionally-red `[v1][contract][shipping-policy]` assertions green by material
removal while preserving every Phase 19-24 release-safe contract. The five
non-blocking observations above are executor discretion items, not gates.
