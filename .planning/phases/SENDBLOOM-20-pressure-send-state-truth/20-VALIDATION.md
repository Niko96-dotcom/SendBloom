---
phase: 20
slug: pressure-send-state-truth
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-12
---

# Phase 20 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
>
> **Phase 20 success model:**
> - `[v1][contract][pressure-release]` flips **GREEN** via production pad/DSP/preset fixes
> - Other Phase 19 `[v1][contract]` filters stay **RED** (oversized, true-bypass, posthard, input-anchors, midi-apvts, shipping-policy)
> - SEND-01…14 and UX-01…05 automated asserts green within caveats (SEND-14: prepared sizes only)
> - BASE-04 greens (`[release]`, `[DryPath]`, ProperSRC/HF, ENAB) remain green

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.8.1 + ctest (`Tests` executable) |
| **Config file** | `cmake/Tests.cmake` (via `include(Tests)` in root `CMakeLists.txt`) |
| **Quick run command** | `Builds/Tests "[pressure-release]" && Builds/Tests "[send]"` |
| **Preset/layout** | `Builds/Tests "[preset]" && Builds/Tests "[parm][layout]"` |
| **Still-red contracts** | `Builds/Tests "[v1][contract][oversized-block]"` (expect fail); same for true-bypass / midi-apvts / posthard / input-anchors / shipping-policy |
| **BASE-04 spot** | `Builds/Tests "[release]" && Builds/Tests "[DryPath]"` |
| **Full suite command** | `ctest --test-dir Builds -C Release --output-on-failure` (do not require all-green while non-pressure contracts remain) |
| **Estimated runtime** | Quick ~30–90s; `[send]`+`[preset]` ~1–3 min |

---

## Sampling Rate

- **After every task commit:** Run that task's `<automated>` command
- **After Wave 1 (20-01):** `[send]` + `[parm][layout]` + `[release]` + `[DryPath]` green; pressure-release may still be red until pad fix
- **After Wave 2 (20-02):** `[pressure-release]` **green**; at least one unrelated `[v1][contract]` still red
- **After Wave 3 (20-03) / before `/gsd-verify-work`:** `[preset]` + `[send]` + `[pressure-release]` green; SEND-14 in-range present; oversized still red
- **Max feedback latency (task verify):** prefer &lt; 90s via targeted filters

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| SEND-01 | disconnected → unity wet feed | `Builds/Tests "[send]"` | pass | `PressureControllerTest` / `PressureSendTest` |
| SEND-02 | connected + amount 0 → no new wet | `Builds/Tests "[send]"` + `[pressure-release]` | pass | controller + contract energy |
| SEND-03 | pressure &gt;0 feeds wet | `Builds/Tests "[send]"` + `[pressure-release]` | pass | pad press energy |
| SEND-04 | release stops new wet; tails continue | `Builds/Tests "[pressure-release]"` | pass | energy 0 after release; no tank clear |
| SEND-05 | mouseUp zeros amount, keeps connected | `Builds/Tests "[pressure-release]"` | pass | `V1ContractPressureReleaseTest.cpp` |
| SEND-06 | overlay follows pressed/amount | UI-SPEC + optional `[send]` predicate; production paint change | pass / human | `PedalFaceplatePaint.cpp` |
| SEND-07 | Advanced PRESSURE MODE | production AdvancedDrawer + smoke build | pass | `AdvancedDrawer.*` |
| SEND-08 | pad auto-connect, no disconnect on up | `Builds/Tests "[pressure-release]"` | pass | `PressureSendPad.cpp` |
| SEND-09 | Firm ≠ Soft | `Builds/Tests "[send][PressureSend]"` | pass | existing + controller |
| SEND-10 | ~3 ms / ~25 ms | `Builds/Tests "[send][PressureController]"` | pass | `PressureControllerTest.cpp` (W0 create) |
| SEND-11 | pressure presets rest amount 0 | `Builds/Tests "[preset]"` | pass | XML + `PresetTest` |
| SEND-12 | disconnect → always-on | `Builds/Tests "[send]"` | pass | controller disconnected path |
| SEND-13 | ID unchanged; default 0 | `Builds/Tests "[parm][layout]"` | pass | `ParameterLayoutTest` |
| SEND-14 | in-range block invariant + caveat | `Builds/Tests "[send][SEND-14]"` (or `[send]`); oversized expect-fail | pass / red | controller/integration + RESEARCH Q1 |
| UX-01 | Parameter IDs unchanged | `Builds/Tests "[parm]"` / ID grep stability | pass | `ParameterIDs.h` |
| UX-02 | default send_amount 0 | `Builds/Tests "[parm][layout]"` | pass | `ParameterLayout.cpp` |
| UX-03 | XML ↔ FactoryPresets identical | `Builds/Tests "[preset]"` | pass | `PresetTest` PRST-02 |
| UX-04 | pressure presets connected + 0 | `Builds/Tests "[preset]"` | pass | Firm/Hot Clip/Dry Dub |
| UX-05 | always-on presets disconnected | `Builds/Tests "[preset]"` | pass | remaining five |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------------|-----------------|-----------|-------------------|-------------|--------|
| 20-01-01 | 01 | 1 | SEND-01,02,09,10,12 | T-20-01 | Clamp norms; no heap in processSample | unit | `cmake --build Builds --config Release --target Tests && Builds/Tests "[send][PressureController]" && Builds/Tests "[send][PressureSend]"` | ❌ W0 → create `PressureControllerTest.cpp` | ⬜ pending |
| 20-01-02 | 01 | 1 | SEND-04,13 UX-01,02 | T-20-02 | Scratch reuse; no dual smooth | unit/integration | `Builds/Tests "[parm][layout]" && Builds/Tests "[send]" && Builds/Tests "[release]" && Builds/Tests "[DryPath]"` | ✅ layout; extend asserts | ⬜ pending |
| 20-02-01 | 02 | 2 | SEND-05,08 | T-20-04 | Gesture bracketing; jlimit Y | contract | `Builds/Tests "[pressure-release]"` | ✅ (red→green) | ⬜ pending |
| 20-02-02 | 02 | 2 | SEND-06,07 | T-20-06 | Minimal UI; no brand secrets | contract + still-red | `Builds/Tests "[pressure-release]" && Builds/Tests "[send]" && (Builds/Tests "[v1][contract][oversized-block]"; test $? -ne 0) && (Builds/Tests "[v1][contract][midi-apvts]"; test $? -ne 0)` | ✅ | ⬜ pending |
| 20-03-01 | 03 | 3 | SEND-11 UX-01…05 | T-20-07/08 | XML-only matrix; rebuild BinaryData | unit | `Builds/Tests "[preset]" && Builds/Tests "[parm][layout]" && Builds/Tests "[pressure-release]"` | ✅ extend PresetTest | ⬜ pending |
| 20-03-02 | 03 | 3 | SEND-14 | T-20-09 | In-range only; oversized stays red | unit + expect-fail | `Builds/Tests "[send]" && Builds/Tests "[pressure-release]" && (Builds/Tests "[v1][contract][oversized-block]"; test $? -ne 0) && (Builds/Tests "[v1][contract][true-bypass]"; test $? -ne 0) && (Builds/Tests "[v1][contract][midi-apvts]"; test $? -ne 0)` | ❌ W0 SEND-14 cases | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing Catch2 harness covers discovery. Create during plans (not a separate infra plan):

- [ ] `tests/PressureControllerTest.cpp` — SEND-01/02/09/10/12 timing and gain (Task 20-01-01)
- [ ] `ParameterLayoutTest` default `send_amount == 0` assert (Task 20-01-02)
- [ ] Preset resting-state matrix asserts for all eight programs (Task 20-03-01)
- [ ] SEND-14 in-range block-size cases + documented oversized caveat (Task 20-03-02)
- [ ] Optional overlay/Advanced unit helpers — covered by pressure-release + UI-SPEC if omitted

**Existing infrastructure covers framework/config** — no Catch2 install work.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| Pressed overlay visually lifts on press and clears at connected-at-rest | SEND-06 | Paint/visual | Open editor, enable Pressure Mode, press/release pad; overlay must not stay lit while amount=0 |
| Advanced PRESSURE MODE toggle visible and disconnects to always-on wet | SEND-07 | UI placement | Expand Advanced; toggle PRESSURE MODE; confirm APVTS send_connected and audible always-on vs dry-at-rest |

---

## Intentional Reds (do not treat as Phase 20 gaps)

| Filter | Owner phase |
|--------|-------------|
| `[v1][contract][oversized-block]` | 21 |
| `[v1][contract][true-bypass]` | 21 |
| `[v1][contract][posthard]` | 23 |
| `[v1][contract][input-anchors]` | 23 |
| `[v1][contract][midi-apvts]` | 22 |
| `[v1][contract][shipping-policy]` | 25 |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 create-then-verify
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 gaps listed and assigned to plan tasks
- [x] No watch-mode flags
- [x] Feedback latency target &lt; 90s for task filters
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending execution
