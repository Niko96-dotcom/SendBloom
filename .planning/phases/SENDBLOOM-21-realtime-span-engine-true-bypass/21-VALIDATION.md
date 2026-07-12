---
phase: 21
slug: realtime-span-engine-true-bypass
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-12
---

# Phase 21 — Validation Strategy

> Per-phase validation contract for feedback sampling during execution.
>
> **Phase 21 success model:**
> - `[v1][contract][oversized-block]` and `[v1][contract][true-bypass]` flip **GREEN** via production DSP fixes
> - RT-01…03, RT-05, RT-08…15, CORE-14…18 automated asserts green
> - Other Phase 19 `[v1][contract]` filters stay **RED** (midi-apvts, posthard, input-anchors, shipping-policy)
> - Phase 20 greens (`[pressure-release]`, `[send]`) and BASE-04 (`[release]`, `[DryPath]`) remain green

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 3.8.1 + ctest (`Tests` executable) |
| **Config file** | `cmake/Tests.cmake` (via `include(Tests)` in root `CMakeLists.txt`) |
| **Quick run command** | `Builds/Tests "[oversized-block]" && Builds/Tests "[true-bypass]"` |
| **Realtime / stress** | `Builds/Tests "[realtime]" && Builds/Tests "[verb][EngineCrossfade]"` |
| **Still-red contracts** | `Builds/Tests "[v1][contract][midi-apvts]"` (expect fail); same for posthard / input-anchors / shipping-policy |
| **BASE-04 + pressure spot** | `Builds/Tests "[pressure-release]" && Builds/Tests "[release]" && Builds/Tests "[DryPath]"` |
| **Full suite command** | `ctest --test-dir Builds -C Release --output-on-failure` (do not require all-green while later-phase contracts remain) |
| **Estimated runtime** | Quick ~30–120s; stress filters ~1–3 min |

---

## Sampling Rate

- **After every task commit:** Run that task's `<automated>` command
- **After Wave 1 (21-01):** `[oversized-block]` **green**; IntegrationAllocScan green for setSize ban; pressure/release still green
- **After Wave 2 (21-02):** `[true-bypass]` **green**; oversized stays green
- **After Wave 3 (21-03) / before `/gsd-verify-work`:** authentic/realtime RT contracts + stress green; midi/posthard/input/shipping still red
- **Max feedback latency (task verify):** prefer &lt; 120s via targeted filters (stress tasks may take longer)

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| RT-01 | No heap/`setSize` in process | `Builds/Tests "[realtime][static][integration]"` | pass | `IntegrationAllocScanTest` (+ setSize ban) |
| RT-02 | Oversized keeps wet | `Builds/Tests "[oversized-block]"` | pass | `V1ContractOversizedBlockTest` |
| RT-03 | 2048 vs 4×512 parity | `Builds/Tests "[oversized-block]"` | pass | same |
| RT-05 | Control quantum ≤128 | `Builds/Tests "[v1][contract][realtime]"` / span tag | pass | `V1ContractRealtimeSpanTest` (W0) |
| RT-08 | One authentic request/change | `Builds/Tests "[v1][contract][authentic]"` | pass | `V1ContractAuthenticTransitionTest` (W0) |
| RT-09 | Latency 0 across transitions | `Builds/Tests "[chain][latency]"` | pass | `LatencyTest` |
| RT-10 | Crossfade starts first block | authentic contract + XFADE | pass | authentic + `EngineCrossfadeTest` |
| RT-11 | Rapid toggles converge | authentic + stress XFADE-02 | pass | `RealtimeStressTest` |
| RT-12 | Reset only idle engine | existing tank/crossfade path + contract note | pass | `SchroederTank32` + authentic test |
| RT-13 | Crossfade completion no heap | alloc scan + no new alloc in completion | pass | `IntegrationAllocScanTest` |
| RT-14 | 10k stress finite | `Builds/Tests "[realtime][stress]"` | pass | `RealtimeStressTest` (extend) |
| RT-15 | preparedMaxBlock_≤0 safe | realtime contract unprepared case | pass | `V1ContractRealtimeSpanTest` |
| CORE-14 | Channel-preserving bypass | `Builds/Tests "[true-bypass]"` | pass | `V1ContractTrueBypassTest` |
| CORE-15 | Unity tolerance | `Builds/Tests "[true-bypass]"` | pass | same |
| CORE-16 | Ignore Output/Input/… | `Builds/Tests "[true-bypass]"` | pass | same (+6 dB output) |
| CORE-17 | Click-bounded transitions | `Builds/Tests "[parm][bypass]"` | pass | `BypassCrossfadeTest` |
| CORE-18 | Engaged mono-first unchanged | true-bypass + engaged regression (non-bypass) | pass | true-bypass + existing block integ |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Threat Ref | Secure Behavior | Test Type | Automated Command | File Exists | Status |
|---------|------|------|-------------|-----------------|-----------------|-----------|-------------------|-------------|--------|
| 21-01-01 | 01 | 1 | RT-01,15 | T-21-01 | No heap; unprepared early return | static + unit | `cmake --build Builds --config Release --target Tests && Builds/Tests "[realtime][static][integration]"` | ✅ extend alloc scan | ⬜ pending |
| 21-01-02 | 01 | 1 | RT-02,03,05 | T-21-02 | Span ≤128; wet parity | contract | `Builds/Tests "[oversized-block]" && Builds/Tests "[v1][contract][realtime]" && Builds/Tests "[pressure-release]" && Builds/Tests "[DryPath]"` | ❌ W0 realtime span | ⬜ pending |
| 21-02-01 | 02 | 2 | CORE-14…16,18 | T-21-03 | Per-channel dry; Output on engaged only | contract | `Builds/Tests "[true-bypass]" && Builds/Tests "[oversized-block]"` | ✅ | ⬜ pending |
| 21-02-02 | 02 | 2 | CORE-17 | T-21-04 | 5 ms click-bounded | unit | `Builds/Tests "[parm][bypass]" && Builds/Tests "[true-bypass]"` | ✅ | ⬜ pending |
| 21-03-01 | 03 | 3 | RT-08…13,09 | T-21-05 | One request/edge; latency 0 | contract | `Builds/Tests "[v1][contract][authentic]" && Builds/Tests "[chain][latency]" && Builds/Tests "[verb][EngineCrossfade]"` | ❌ W0 authentic | ⬜ pending |
| 21-03-02 | 03 | 3 | RT-14 | T-21-06 | Finite under toggles/bypass/oversized | stress | `Builds/Tests "[realtime][stress]" && Builds/Tests "[pressure-release]" && (Builds/Tests "[v1][contract][midi-apvts]"; test $? -ne 0) && (Builds/Tests "[v1][contract][shipping-policy]"; test $? -ne 0)` | ✅ extend stress | ⬜ pending |

*Status: ⬜ pending · ✅ green · ❌ red · ⚠️ flaky*

---

## Wave 0 Requirements

Existing Catch2 harness covers discovery. Create during plans (not a separate infra plan):

- [ ] `tests/V1ContractRealtimeSpanTest.cpp` — RT-05 quantum + RT-15 unprepared (Task 21-01-02)
- [ ] Extend `tests/IntegrationAllocScanTest.cpp` — ban `.setSize(` in process bodies (Task 21-01-01)
- [ ] `tests/V1ContractAuthenticTransitionTest.cpp` — RT-08/10/11 request semantics (Task 21-03-01)
- [ ] Extend `tests/RealtimeStressTest.cpp` — bypass + oversized + authentic toggles finite (Task 21-03-02)

**Existing infrastructure covers framework/config** — no Catch2 install work. CMake GLOBs `tests/*.cpp`.

---

## Manual-Only Verifications

| Behavior | Requirement | Why Manual | Test Instructions |
|----------|-------------|------------|-------------------|
| — | — | — | All phase behaviors have automated verification |

---

## Validation Sign-Off

- [x] All tasks have `<automated>` verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency &lt; 180s for non-stress; stress &lt; 5 min acceptable
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** pending
