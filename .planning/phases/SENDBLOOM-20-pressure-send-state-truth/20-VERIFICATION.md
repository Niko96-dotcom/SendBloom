---
phase: 20-pressure-send-state-truth
verified: 2026-07-12T16:03:00Z
status: passed
score: 5/5 must-haves verified
behavior_unverified: 0
overrides_applied: 0
re_verification: false
deferred:
  - truth: "SEND-14 full host-block invariance including numSamples > preparedMaxBlock_ (oversized wet continuity)"
    addressed_in: "Phase 21"
    evidence: "Phase 21 goal: host blocks of any size retain full wet processing; 20-RESEARCH Open Q1 + [v1][contract][oversized-block] intentionally remains red"
  - truth: "MIDI CC1 realtime pressure without APVTS mutation"
    addressed_in: "Phase 22"
    evidence: "Phase 22 goal: MIDI CC1 sample-position-aware realtime pressure; Phase 20 stubs midi target at 0"
---

# Phase 20: Pressure Send State Truth Verification Report

**Phase Goal:** Pressure Mode tells the truth from UI and presets — connected-and-resting is dry with decaying tails; release never flips back to always-on wet feed.

**Verified:** 2026-07-12T16:03:00Z  
**Status:** passed  
**Re-verification:** No — initial verification

**Success model (phase-specific):** `[v1][contract][pressure-release]` green; SEND-01…14 / UX-01…05 automated asserts green (SEND-14 in-range only); other Phase 19 `[v1][contract]` filters stay red.

## Goal Achievement

### Observable Truths

| # | Truth | Status | Evidence |
| --- | ------- | ---------- | -------------- |
| 1 | Disconnected mode is always-on wet feed; connected-at-rest feeds no new wet input while existing tails continue; press sends; release zeros amount without disconnecting | ✓ VERIFIED | `PressureController` disconnected→`1.0f`, connected+0→curve 0 (`PressureControllerTest`); `[pressure-release]` press energy >1e-3 then post-release wet energy <1e-6 with dry RMS >1e-3; tank preserve `[chain][send][tank]` |
| 2 | On-screen pad release keeps `send_connected=true`, sets `send_amount=0`, and pressed overlay follows pressure/pressed state — not connection alone | ✓ VERIFIED | `PressureSendPad::mouseUp` zeros amount only (no `setConnected(false)`); `[pressure-release]` asserts connected>0.5 & amount≈0; `shouldDrawFootswitchPressedOverlay` + `[send][SEND-06]` unit test; editor paints with `isPressed()`/`getDisplayAmount()` |
| 3 | Advanced UI exposes persistent Pressure Mode; pad press may auto-connect without disconnecting on release | ✓ VERIFIED | `AdvancedDrawer` `PRESSURE MODE` + `ButtonAttachment`→`sendConnected`; `PluginEditor` passes `ParameterIDs::sendConnected`; `mouseDown`→`setConnected(true)`; release keeps connected (`[pressure-release]`) |
| 4 | Factory pressure presets load connected with `send_amount=0`; always-on presets load disconnected; XML and `FactoryPresets.cpp` recall identical state; parameter IDs unchanged; default `send_amount` is 0 | ✓ VERIFIED | All 8 XML `send_*` match §9.7 matrix; `[preset]` 8/8 PASS incl. resting matrix + XML↔program parity; `ParameterIDs` still `send_amount`/`send_connected`; layout default `0.0f` + `[parm][layout]` |
| 5 | Firm vs Soft remain distinct; attack/release are ~3 ms / ~25 ms; behavior is invariant across host block sizes | ✓ VERIFIED | Firm≠Soft in `PressureControllerTest` + `PressureSendTest`; τ=0.003/0.025 in `PressureController.h`; `[send][SEND-14]` 2 cases / 52 asserts PASS across 64/128/256/512; oversized left red (Phase 21 caveat) |

**Score:** 5/5 truths verified (0 present, behavior-unverified)

### Deferred Items

| # | Item | Addressed In | Evidence |
|---|------|-------------|----------|
| 1 | Oversized-block wet continuity (full SEND-14 beyond prepared max) | Phase 21 | Roadmap Phase 21 goal; RESEARCH Q1 execution lock; `[oversized-block]` exit 42 |
| 2 | MIDI CC1 realtime pressure without APVTS mutation | Phase 22 | Roadmap Phase 22; processor forces `setMidiPressureTarget(0)` |

### Required Artifacts

| Artifact | Expected | Status | Details |
| -------- | ----------- | ------- | ------- |
| `source/PressureController.h` | asym smooth + curve send gain | ✓ VERIFIED | ~87 lines; `processSample`, 3/25 ms coeffs, disconnected unity |
| `source/ParameterLayout.cpp` | `send_amount` default 0 | ✓ VERIFIED | default `0.0f` at sendAmount param |
| `tests/PressureControllerTest.cpp` | SEND-01/02/09/10/12 + SEND-14 | ✓ VERIFIED | 7 cases; `[send][PressureController]` + `[SEND-14]` green |
| `tests/ParameterLayoutTest.cpp` | UX-02 default 0 | ✓ VERIFIED | `default send_amount is 0 (UX-02)` |
| `source/ui/PressureSendPad.cpp` | release-to-zero, no disconnect | ✓ VERIFIED | mouseUp zeros amount; mouseDown auto-connects |
| `source/ui/PedalFaceplatePaint.cpp` | overlay from press/amount | ✓ VERIFIED | `shouldDrawFootswitchPressedOverlay`; wired in `drawStateOverlays` |
| `source/ui/AdvancedDrawer.cpp` | PRESSURE MODE → send_connected | ✓ VERIFIED | ButtonAttachment on `pressureModeToggle` |
| `tests/V1ContractPressureReleaseTest.cpp` | pressure-release green | ✓ VERIFIED | 1 case / 8 asserts PASS |
| `resources/presets/{Firm_Pressure,Hot_Clip,Dry_Dub_Sends}.xml` | connected=1 amount=0 | ✓ VERIFIED | all three match |
| `tests/PresetTest.cpp` | 8-preset resting matrix | ✓ VERIFIED | SEND-11 / UX-04/05 case green |
| `20-RESEARCH.md` | SEND-14 oversized caveat | ✓ VERIFIED | Open Q1 RESOLVED + execution lock |

### Key Link Verification

| From | To | Via | Status | Details |
| ---- | --- | --- | ------ | ------- |
| `PluginProcessor.cpp` processBlock | `PressureController::processSample` | per-sample `sendGainScratch_` | ✓ WIRED | lines ~213–216 set targets; ~306 fills scratch into chain |
| `PressureController` | `ParameterCurves::sendGain` | smooth then curve when connected | ✓ WIRED | `processSample` return path |
| `ParameterLayout` | APVTS `send_amount` | default 0 | ✓ WIRED | layout + layout test |
| `PressureSendPad::mouseUp` | APVTS amount/connected | gesture + zero; never disconnect | ✓ WIRED | amount only; no `setConnected(false)` |
| `AdvancedDrawer` PRESSURE MODE | `ParameterIDs::sendConnected` | ButtonAttachment | ✓ WIRED | ctor + editor pass-through |
| `drawStateOverlays` | pad pressed / amount | overlay predicate | ✓ WIRED | editor passes `isPressed`/`getDisplayAmount` |
| preset XML | BinaryData / applyPreset | rebuild + PresetTest parity | ✓ WIRED | UX-03 asserts in resting matrix |
| SEND-14 tests | prepared block sizes | ≤ preparedMax; oversized red | ✓ WIRED | controller + processor `[SEND-14]` |

### Data-Flow Trace (Level 4)

| Artifact | Data Variable | Source | Produces Real Data | Status |
| -------- | ------------- | ------ | ------------------ | ------ |
| `PluginProcessor` wet send | `sendGainScratch_[i]` | `pressureController.processSample()` from APVTS snap | live host pressure / connection | ✓ FLOWING |
| `PressureSendPad` | APVTS `send_amount` / `send_connected` | mouse Y → `setValueNotifyingHost` | host-notified params | ✓ FLOWING |
| Factory presets | APVTS after `setCurrentProgram` | BinaryData XML via FactoryPresets | real embedded XML values | ✓ FLOWING |
| Overlay | `padPressed` / amounts | pad getters + APVTS norm | real gesture/APVTS state | ✓ FLOWING |

### Behavioral Spot-Checks

| Behavior | Command | Result | Status |
| -------- | ------- | ------ | ------ |
| pressure-release green | `Builds/Tests "[pressure-release]"` | 1 case / 8 asserts PASS, exit 0 | ✓ PASS |
| PressureController + SEND-14 | `Builds/Tests "[send][PressureController]"` / `"[send][SEND-14]"` | 6+2 cases PASS | ✓ PASS |
| Full `[send]` | `Builds/Tests "[send]"` | 20 cases / 130 asserts PASS | ✓ PASS |
| Preset matrix | `Builds/Tests "[preset]"` | 8 cases / 235 asserts PASS | ✓ PASS |
| Layout / IDs | `Builds/Tests "[parm][layout]"` | 4 cases PASS | ✓ PASS |
| Overlay predicate | `Builds/Tests "*Footswitch*"` | 1 case / 5 asserts PASS | ✓ PASS |
| Other v1 contracts still red | `Builds/Tests "[v1][contract]"` | 1 passed (pressure-release) / 9 failed | ✓ PASS (intentional) |
| Oversized / bypass / midi red | individual filters | exit 42 each | ✓ PASS (intentional) |
| BASE-04 greens | `"[release]"` / `"[DryPath]"` | 12 + 4 cases PASS | ✓ PASS |

### Probe Execution

| Probe | Command | Result | Status |
| ----- | ------- | ------ | ------ |
| — | — | No phase-declared `scripts/*/tests/probe-*.sh` for Phase 20 | SKIPPED |

### Requirements Coverage

| Requirement | Source Plan | Description | Status | Evidence |
| ----------- | ---------- | ----------- | ------ | -------- |
| SEND-01 | 20-01 | disconnected always-on wet | ✓ SATISFIED | PressureController + PressureSend tests |
| SEND-02 | 20-01 | connected+0 no new wet | ✓ SATISFIED | controller settle + pressure-release energy |
| SEND-03 | 20-01 | pressure >0 sends | ✓ SATISFIED | press energy in pressure-release |
| SEND-04 | 20-01 | release stops new wet; tails | ✓ SATISFIED | post-release energy 0; tank preserve test |
| SEND-05 | 20-02 | mouseUp amount 0, connected true | ✓ SATISFIED | pad + `[pressure-release]` |
| SEND-06 | 20-02 | overlay follows press/amount | ✓ SATISFIED | predicate + `[send][SEND-06]` |
| SEND-07 | 20-02 | Advanced persistent Pressure Mode | ✓ SATISFIED | AdvancedDrawer ButtonAttachment |
| SEND-08 | 20-02 | pad auto-connect, no disconnect on up | ✓ SATISFIED | mouseDown/up + contract |
| SEND-09 | 20-01 | Firm ≠ Soft | ✓ SATISFIED | controller + PressureSend tests |
| SEND-10 | 20-01 | ~3 ms / ~25 ms | ✓ SATISFIED | controller timing tests + coeffs |
| SEND-11 | 20-03 | pressure presets rest amount 0 | ✓ SATISFIED | PresetTest matrix |
| SEND-12 | 20-01 | disconnect → always-on | ✓ SATISFIED | disconnected unity path |
| SEND-13 | 20-01 | send_amount ID unchanged | ✓ SATISFIED | ParameterIDs + layout test |
| SEND-14 | 20-03 | in-range block invariant (+ caveat) | ✓ SATISFIED | `[send][SEND-14]`; oversized deferred P21 |
| UX-01 | 20-01/03 | Parameter IDs unchanged | ✓ SATISFIED | `send_amount` / `send_connected` strings |
| UX-02 | 20-01/03 | default send_amount 0 | ✓ SATISFIED | layout + `[parm][layout]` |
| UX-03 | 20-03 | XML ↔ program identical recall | ✓ SATISFIED | PresetTest parity loop |
| UX-04 | 20-03 | pressure presets connected + 0 | ✓ SATISFIED | Firm/Hot Clip/Dry Dub |
| UX-05 | 20-03 | always-on presets disconnected | ✓ SATISFIED | remaining five presets |

No orphaned Phase 20 REQUIREMENTS.md IDs — UX-06…16 map to Phase 25.

### Anti-Patterns Found

| File | Line | Pattern | Severity | Impact |
| ---- | ---- | ------- | -------- | ------ |
| `tests/V1ContractPressureReleaseTest.cpp` | ~156 | Stale comment “Intended failures on current tree” | ℹ️ Info | Comment rot only — asserts now expect green behavior |
| Phase-modified production sources | — | No TBD/FIXME/XXX debt markers | — | Clean |

### Human Verification Required

None required for phase pass. Roadmap SCs are covered by Catch2 contracts and unit tests. Optional host visual QA (overlay paint / Advanced drawer layout) remains available per `20-VALIDATION.md` Manual-Only table but does not gate this phase — logic and wiring are automated.

### Gaps Summary

No actionable gaps. Phase goal achieved: pressure-release is green; connected-at-rest is dry with tails preserved; pad release keeps connected; preset matrix matches §9.7; other Phase 19 `[v1][contract]` filters remain intentionally red for Phases 21–25.

---

_Verified: 2026-07-12T16:03:00Z_  
_Verifier: Claude (gsd-verifier)_
