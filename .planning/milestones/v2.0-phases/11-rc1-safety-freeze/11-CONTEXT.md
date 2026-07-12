# Phase 11: RC1 Safety Freeze - Context

**Gathered:** 2026-07-08
**Status:** Ready for planning

<domain>
## Phase Boundary

Protect RC1 from the broken pseudo-SRC 32k Color path (`processAuthentic()` accumulator + hold) by making **host-rate Schroeder tank** the production/default sound and **32k Color optional/off by default**. Users loading the plugin fresh or selecting any factory preset must not hit the HF imaging whistle at 48 kHz.

**In scope:** APVTS default, factory preset XML values, honest docs, Advanced drawer tooltip copy, automated SAFE assertions, full existing test suite green.

**Out of scope (hard constraints):** r8brain / ProperSRC (Phase 13+), `SchroederTank32` rewrite, `SchroederTankCore` extraction (Phase 12), `WetOverdrive` tuning, gate/send/dry routing changes, new UI controls, product rename, release tags, engine crossfade (Phase 16), PDC/ADR-003 (Phase 17), enabling 32k Color by default (Phase 18).

</domain>

<decisions>
## Implementation Decisions

- **D-01:** Phase 11 may only touch ParameterLayout.cpp, resources/presets/*.xml, ReleaseTruthTest.cpp, optional ParameterLayoutTest.cpp, AdvancedDrawer.cpp tooltip, README.md, docs/RELEASE_CHECKLIST.md
- **D-02:** All 8 factory presets ship authentic_color=0
- **D-03:** APVTS default authentic_color=false in ParameterLayout.cpp
- **D-04:** README and RELEASE_CHECKLIST get RC1 safety wording; no ProperSRC ship claims
- **D-05:** Keep "32k Color" label; tooltip adds experimental/off-by-default warning
- **D-06:** Extend ReleaseTruthTest.cpp with [release][safe] cases for SAFE-01/02/03
- **D-07:** Do not touch SchroederTank32, GatedBloomChain routing, WetOverdrive, r8brain/SRC, UI layout beyond tooltip

### D-01: Allowed file touch list

Phase 11 may modify **only** these paths:

| Path | Change |
|------|--------|
| `source/ParameterLayout.cpp` | `authentic_color` APVTS default `false` |
| `resources/presets/*.xml` (all 8) | `authentic_color` value `0` |
| `tests/ReleaseTruthTest.cpp` | New `[release][safe]` cases for SAFE-01/02/03 |
| `tests/ParameterLayoutTest.cpp` | Optional: assert `authentic_color` default off on fresh `PluginProcessor` |
| `source/ui/AdvancedDrawer.cpp` | Tooltip copy only — experimental/advanced warning |
| `README.md` | State 32k Color off by default for RC1 |
| `docs/RELEASE_CHECKLIST.md` | RC1 safety note + honest accumulator disclaimer when enabled |

**May read but not change:** `source/FactoryPresets.cpp` (loads embedded XML — presets fix is at XML source), `tests/PresetTest.cpp`, `tests/HighFrequencyRingingDiagnosticsTest.cpp` (reference for HF assertion patterns).

**Rebuild note:** After preset XML edits, BinaryData is regenerated on next CMake build — no manual BinaryData edits.

### D-02: Factory presets — all `authentic_color=0`

- **Decision:** All 8 factory presets ship `authentic_color=0`.
- **Current state:** 7 presets have `value="1"`; only `Hot_Clip.xml` is already `0`.
- **Rationale:** SAFE-02; presets are the primary user entry point after fresh load. Partial fix (APVTS only) still exposes whistle on preset recall.
- **Preserves:** All other preset parameter values unchanged — only flip `authentic_color`.

### D-03: APVTS default — `authentic_color=false`

- **Decision:** Change `ParameterLayout.cpp` line 56 from `true` → `false`.
- **Rationale:** SAFE-01; fresh plugin instance (no preset, no saved state) must use host-rate path.
- **Display name:** Keep APVTS name `"Authentic Color"` — UI label `"32k Color"` in `AdvancedDrawer.h` is the user-facing string (UI-04: no new controls).

### D-04: Documentation wording

**README.md:**
- Keep existing description of what 32k Color *does* when enabled.
- Add explicit RC1 line: **32k Color is off by default**; host-rate tank is the production path until ProperSRC passes acceptance gates (Phases 13–18).
- Do not claim ProperSRC is shipped or that accumulator path is fixed.

**docs/RELEASE_CHECKLIST.md:**
- Add **RC1 Safety Freeze** subsection under or beside existing 32k Color Truth (VERB-05).
- When enabled: keep honest accumulator-path description (current `processAuthentic` behavior).
- Add disclaimer: path is **experimental / not production-default** until TEST-11, DIAG-04, LAT-02, XFADE-01 pass (Phase 18 enablement).
- Do not update checklist to describe ProperSRC as shipped — that belongs to Phase 18.

**REQUIREMENTS.md:** Planner/executor may tick SAFE-01/02/03 on verification; no requirement text rewrite in Phase 11 unless acceptance wording is stale after implementation.

### D-05: UI label and tooltip

- **Label:** Keep `"32k Color"` on `colorToggle` — no rename, no new controls (INTEG-04).
- **Tooltip:** Update to experimental/advanced framing. Recommended copy (planner may tighten prose):
  - Lead: *Experimental — off by default until validated.*
  - Body: Retain existing technical description (32,768 Hz stepping, fixed delay table, 9-bit quantization, original software).
  - Close: *May exhibit HF artifacts at some host rates; host-rate reverb is the production default.*
- **No changes to:** `AdvancedDrawer.h` layout, `PluginEditor.cpp`, pedal layout, disabled Extended Stereo / Dirt OS toggles.

### D-06: Tests that prove the safety freeze

Extend `tests/ReleaseTruthTest.cpp` with `[release][safe]` cases:

1. **SAFE-01 — Fresh load default off:** Construct `PluginProcessor` with no `setStateInformation`; assert `authentic_color` APVTS raw value ≈ 0.
2. **SAFE-02 — All presets auth=0:** For each `setCurrentProgram(0..7)`, assert `authentic_color` ≈ 0. Complements existing preset/XML parity test — adds explicit SAFE assertion.
3. **SAFE-03 — Host-rate path no HF whistle at 48 kHz:** Render wet chain (or `GatedBloomChain` + default-off processor) with guitar-pluck or impulse fixture at 48 kHz, `authentic_color=false`, `distn=0`; assert tail metrics stay below existing host-rate ceilings (reuse thresholds from `HighFrequencyRingingDiagnosticsTest` config A / imaging band limits). Proves production default is clean — not a new tuning pass on authentic path.

**Regression gates:**
- Full Catch2 suite green (`ctest --test-dir Builds -C Release`).
- Existing `authentic_color produces distinct response` test **stays** — still validates paths differ when user explicitly enables 32k Color (diagnostic value for Phase 15+).
- `RealtimeStressTest` may continue forcing `authentic_color=1` for stress — that tests engine robustness when toggled, not RC1 default.

**No new test file** — keep SAFE cases in `ReleaseTruthTest.cpp` alongside release truth cases.

### D-07: Files and areas to avoid (conflict prevention)

| Avoid | Reason |
|-------|--------|
| `source/SchroederTank32.h` | No rewrite of `processAuthentic()` / accumulator — Phase 12–13 |
| `source/SchroederTank32DelayTable.h`, new `SchroederTankCore.*` | Phase 12 |
| Any r8brain / `FixedRateAdapter` / `Authentic32Mode` | Phase 13 |
| `source/GatedBloomChain.h`, `source/PluginProcessor.cpp` routing | Gate/send/dry/OD routing frozen |
| `source/WetOverdrive.*` | No tuning |
| `source/Fdn8Reverb.h`, `source/IReverbEngine.h` | Integration/SRC phases |
| `source/ui/*` except `AdvancedDrawer.cpp` tooltip | Avoid UI phase clash (Phase 9 shipped layout) |
| `CMakeLists.txt` (new deps), `.github/workflows/*` | No SRC dependency |
| Product name, version strings, git tags | User constraint |
| `HighFrequencyRingingDiagnosticsTest.cpp` threshold changes | Don't retune authentic-path acceptance — only add host-rate default proof |

### Claude's Discretion

- Exact tooltip prose and README/checklist sentence-level wording (within D-04/D-05 intent).
- Whether SAFE-01 lives in `ParameterLayoutTest.cpp` or only `ReleaseTruthTest.cpp` (at least one must cover it).
- HF assertion fixture choice (guitar pluck vs impulse) as long as it matches host-rate config A semantics at 48 kHz.

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Requirements and roadmap
- `.planning/REQUIREMENTS.md` — SAFE-01, SAFE-02, SAFE-03; INTEG-04 (no new UI controls)
- `.planning/ROADMAP.md` — Phase 11 success criteria (§ Phase 11: RC1 Safety Freeze)
- `.planning/PROJECT.md` — v2.0 decisions: 32k off by default, Option C branch strategy
- `.planning/STATE.md` — current blocker: 7/8 presets ship auth=1

### Research (context, do not implement from these in Phase 11)
- `.planning/research/PITFALLS.md` — RC1 preset regression, accumulator never user-default
- `.planning/research/ARCHITECTURE.md` — RC1 safety parallel track (step 0)
- `.planning/research/FEATURES.md` — 32k Color off by default feature row

### Source — parameters and presets
- `source/ParameterLayout.cpp` — APVTS default (currently `true`, must become `false`)
- `source/ParameterIDs.h` — `authentic_color` ID
- `resources/presets/*.xml` — factory preset source (embedded via BinaryData)

### Source — DSP (read-only for Phase 11)
- `source/SchroederTank32.h` — `processAuthentic()` accumulator path (do not modify)
- `source/GatedBloomChain.h` — wet chain order (do not modify)

### UI
- `source/ui/AdvancedDrawer.cpp` — tooltip target
- `source/ui/AdvancedDrawer.h` — `"32k Color"` label

### Tests
- `tests/ReleaseTruthTest.cpp` — primary SAFE test extension target
- `tests/HighFrequencyRingingDiagnosticsTest.cpp` — HF metric thresholds reference
- `tests/PresetTest.cpp` — preset round-trip patterns
- `tests/ParameterLayoutTest.cpp` — layout default patterns

### Docs
- `README.md` — product-facing default behavior
- `docs/RELEASE_CHECKLIST.md` — VERB-05 32k Color Truth section

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `ParameterLayout.cpp` bool param: third ctor arg is default; single-line SAFE-01 fix.
- `resources/presets/*.xml`: uniform `<PARAM id="authentic_color" value="0"/>` across 8 files.
- `ReleaseTruthTest.cpp` `setCurrentProgram loads embedded XML preset values`: loop over presets — extend with explicit auth=0 REQUIRE.
- `HighFrequencyRingingDiagnosticsTest.cpp` `kMatrixConfigs[0]` (`A_rev_bright`, `authenticColor=false`): template for SAFE-03 host-rate HF proof.

### Established Patterns
- Factory presets: XML in `resources/presets/` → BinaryData at build → `FactoryPresets::applyEmbeddedXml`.
- Release truth tests tagged `[release][verb]`, `[release][preset]` — use `[release][safe]` for new cases.
- Advanced drawer: disabled toggles use `"Coming soon"` tooltip pattern; 32k Color stays enabled but off by default.

### Integration Points
- `PluginProcessor` reads `smoothedBank.getNextAuthenticColorTarget()` → `GatedBloomChain::processSample(..., authenticColor, ...)`.
- `SchroederTank32::processSample` branches on `authenticColor` to `useAuthenticPath` — no Phase 11 change; default `false` avoids branch.

### Current gaps (must fix)
- APVTS default: `true` at `ParameterLayout.cpp:56`.
- Presets: 7× `value="1"`, 1× `value="0"` (`Hot_Clip.xml`).
- Tooltip: describes feature positively with no experimental/off-by-default warning.
- No dedicated SAFE-01/02/03 automated assertions.

</code_context>

<specifics>
## Specific Ideas

- User confirmed: ringing disappears when `authentic_color` off — root cause is accumulator + hold pseudo-SRC, not host-rate tank.
- RC1 ships for listen-test with **host-rate tank + tamed wet OD** (OD already tuned in v1; do not retune).
- Branch `feature/proper-32k-src` — Phase 11 is first v2.0 execution step.
- 32k Color remains available in Advanced drawer for power users / future A/B once ProperSRC lands.

</specifics>

<deferred>
## Deferred Ideas

- **ProperSRC / r8brain integration** — Phase 13
- **SchroederTankCore extraction** — Phase 12
- **Engine crossfade on toggle** — Phase 16 (XFADE-01)
- **PDC / ADR-003 latency policy** — Phase 17
- **Re-enable `authentic_color=1` in factory presets** — Phase 18 after acceptance gates
- **Remove or demote legacy accumulator path** — post-ProperSRC validation (Phase 15+)
- **Multi-DAW smoke (TEST-07)** — still deferred from v1.0

</deferred>

---

*Phase: 11-RC1 Safety Freeze*
*Context gathered: 2026-07-08*
