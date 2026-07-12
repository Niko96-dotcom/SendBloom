---
phase: 24
slug: reverb-state-wet-dirt-integrity
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-12
---

# Phase 24 — Validation Strategy

> **Phase 24 success model:**
> - New greens: `[v1][contract][predelay]`, `[mod-invariant]`, `[src-underfill]`, `[wet-dirt]`
> - DSP-01…15 automated asserts green
> - ProperSRC/HF gates stay green (`[SRC-06]`, HF diagnostics)
> - Phase 20–23 greens stay green: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`, `[input-anchors]`, `[posthard]`, `[per-sample]`, `[realtime]`, `[authentic]`
> - Later-phase red stays red: `[shipping-policy]`

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + `Builds/Tests` |
| **Quick Predelay** | `Builds/Tests "[v1][contract][predelay]"` |
| **Quick Mod/SRC** | `Builds/Tests "[v1][contract][mod-invariant]" && Builds/Tests "[v1][contract][src-underfill]" && Builds/Tests "[verb][FixedRateAdapter][SRC-06]"` |
| **Quick Dirt** | `Builds/Tests "[v1][contract][wet-dirt]" && Builds/Tests "[release][safe]"` |
| **Regression** | `Builds/Tests "[pressure-release]" && Builds/Tests "[oversized-block]" && Builds/Tests "[true-bypass]" && Builds/Tests "[midi-apvts]" && Builds/Tests "[input-anchors]" && Builds/Tests "[posthard]"` |
| **Still-red** | `Builds/Tests "[v1][contract][shipping-policy]"` expect fail |
| **Full** | `ctest --test-dir Builds -C Release --output-on-failure` (not all-green until later phases) |

---

## Sampling Rate

- **After every task commit:** Run plan-scoped Catch2 filter
- **After every plan wave:** Plan tags + SRC-06 + affected SchroederTank*/WetOverdrive*
- **Before verify-work:** Phase gate regression list above green; shipping-policy still red
- **Max feedback latency:** ~120 seconds for quick filters

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| DSP-01 | Predelay clocks in bright | `Builds/Tests "[v1][contract][predelay][DSP-01]"` | pass | `V1ContractPredelayTest` |
| DSP-02 | Fixed 55 ms tap + mix | `Builds/Tests "[v1][contract][predelay][DSP-02]"` | pass | `V1ContractPredelayTest` |
| DSP-03 | No stale burst | `Builds/Tests "[v1][contract][predelay][DSP-03]"` | pass | `V1ContractPredelayTest` |
| DSP-04 | Bounded darkMix automation | `Builds/Tests "[v1][contract][predelay][DSP-04]"` | pass | `V1ContractPredelayTest` |
| DSP-05 | LFO depth seconds invariant | `Builds/Tests "[v1][contract][mod-invariant][DSP-05]"` | pass | `V1ContractModInvariantTest` |
| DSP-06 | ProperSRC pre-clear | `Builds/Tests "[v1][contract][src-underfill][DSP-06]"` | pass | `V1ContractSrcUnderfillTest` |
| DSP-07 | Unwritten output zero | `Builds/Tests "[v1][contract][src-underfill][DSP-07]"` | pass | `V1ContractSrcUnderfillTest` |
| DSP-08 | HF/SRC imaging green | `Builds/Tests "[verb][FixedRateAdapter][SRC-06]"` | pass | Existing ProperSRC suite |
| DSP-09 | 100 Hz pre-clip HP | `Builds/Tests "[v1][contract][wet-dirt][DSP-09]"` | pass | `V1ContractWetDirtTest` |
| DSP-10 | 20 Hz DC blocker | `Builds/Tests "[v1][contract][wet-dirt][DSP-10]"` | pass | `V1ContractWetDirtTest` |
| DSP-11 | Long-run DC `<1e-4` | `Builds/Tests "[v1][contract][wet-dirt][DSP-11]"` | pass | `V1ContractWetDirtTest` |
| DSP-12 | Dry path unaffected | `Builds/Tests "[v1][contract][wet-dirt][DSP-12]"` | pass | `V1ContractWetDirtTest` + DryPath |
| DSP-13 | dirt_os disabled | `Builds/Tests "[v1][contract][wet-dirt][DSP-13]"` | pass | `V1ContractWetDirtTest` |
| DSP-14 | authentic_color default off | `Builds/Tests "[release][safe]"` | pass | ReleaseTruth |
| DSP-15 | Presets authentic_color=0 | `Builds/Tests "[release][safe]"` | pass | ReleaseTruth / PresetTest |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Automated Command | Status |
|---------|------|------|-------------|-------------------|--------|
| 24-01-01 | 01 | 1 | DSP-01…04 Wave 0 stubs | `[predelay]` (red→green after fix) | pending |
| 24-01-02 | 01 | 1 | DSP-01…04 production fix | `[predelay]` | pending |
| 24-02-01 | 02 | 1 | DSP-05 Wave 0 + fix | `[mod-invariant]` | pending |
| 24-02-02 | 02 | 1 | DSP-06/07 Wave 0 + fix | `[src-underfill]` + SRC-06 | pending |
| 24-03-01 | 03 | 2 | DSP-09…13 Wave 0 + filters | `[wet-dirt]` | pending |
| 24-03-02 | 03 | 2 | DSP-14/15 + regressions | `[release][safe]` + Phase 20–23 tags | pending |

---

## Wave 0 Requirements

- [ ] `tests/V1ContractPredelayTest.cpp` — DSP-01…04
- [ ] `tests/V1ContractModInvariantTest.cpp` — DSP-05
- [ ] `tests/V1ContractSrcUnderfillTest.cpp` — DSP-06…07
- [ ] `tests/V1ContractWetDirtTest.cpp` — DSP-09…13
- [ ] `OnePoleHighpass` in `WetOverdrive.h` — prerequisite for dirt contracts

---

## Regression Gates (must stay green)

- ProperSRC/HF: `[verb][FixedRateAdapter][SRC-06]`, HF diagnostics
- Phase 20–23: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`, `[input-anchors]`, `[posthard]`, `[per-sample]`, `[realtime]`, `[authentic]`

## Explicit non-goals this phase

- Do not green `[shipping-policy]`
- Do not retune reverb character / ProperSRC quality preset
- Do not enable `dirt_os`

## Manual-Only Verifications

All phase behaviors have automated verification.

## Validation Sign-Off

- [x] All tasks have automated verify or Wave 0 dependencies
- [x] Sampling continuity: no 3 consecutive tasks without automated verify
- [x] Wave 0 covers all MISSING references
- [x] No watch-mode flags
- [x] Feedback latency acceptable for Catch2 filters
- [x] `nyquist_compliant: true` set in frontmatter

**Approval:** approved 2026-07-12 (autonomous auto-accept)
