---
phase: 22
slug: midi-per-sample-control-delivery
status: draft
nyquist_compliant: true
wave_0_complete: false
created: 2026-07-12
---

# Phase 22 — Validation Strategy

> **Phase 22 success model:**
> - `[v1][contract][midi-apvts]` flips **GREEN**
> - MIDI-01…10, RT-04, RT-06, RT-07 automated asserts green
> - Phase 20/21 greens stay green: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`
> - Later-phase reds stay red: posthard, input-anchors, shipping-policy

---

## Test Infrastructure

| Property | Value |
|----------|-------|
| **Framework** | Catch2 + `Builds/Tests` |
| **Quick MIDI** | `Builds/Tests "[midi-apvts]" && Builds/Tests "[midi]"` |
| **Regression** | `Builds/Tests "[pressure-release]" && Builds/Tests "[oversized-block]" && Builds/Tests "[true-bypass]"` |
| **Still-red** | `Builds/Tests "[v1][contract][posthard]"` / input-anchors / shipping-policy expect fail |
| **Full** | `ctest --test-dir Builds -C Release --output-on-failure` (not all-green until later phases) |

---

## Requirement → Automated Command Map

| Req ID | Behavior | Automated Command | Expect | Artifact |
|--------|----------|-------------------|--------|----------|
| MIDI-02/03 | No APVTS mutate / no notify | `Builds/Tests "[midi-apvts]"` | pass | `V1ContractMidiApvtsPurityTest` |
| MIDI-01/06/09 | Connected-only; 0 releases; non-CC1 | `Builds/Tests "[v1][contract][midi]"` | pass | new/extended MIDI contract |
| MIDI-04/05/RT-04 | Sample position + ordered + span cut | same | pass | same |
| MIDI-07/08 | max combine; state clean | same | pass | same |
| MIDI-10 | Deterministic across block sizes | same / `[midi]` | pass | MidiSendAmount + contract |
| RT-06 | Distn/threshold/send per sample | `Builds/Tests "[v1][contract][per-sample]"` or midi+chain | pass | chain + processor tests |
| RT-07 | Input/level/output/bypass per sample | same | pass | processSpan contract / existing |

---

## Per-Task Verification Map

| Task ID | Plan | Wave | Requirement | Automated Command | Status |
|---------|------|------|-------------|-------------------|--------|
| 22-01-01 | 01 | 1 | MIDI-02/03/08 | `[midi-apvts]` + updated `[midi][send]` | pending |
| 22-01-02 | 01 | 1 | MIDI-01/07 | MIDI target + max; pressure-release green | pending |
| 22-02-01 | 02 | 2 | MIDI-04/05/06/09, RT-04 | sample-accurate MIDI contracts | pending |
| 22-02-02 | 02 | 2 | MIDI-10 | block-size MIDI parity / finite | pending |
| 22-03-01 | 03 | 3 | RT-06 | distn/threshold arrays in chain | pending |
| 22-03-02 | 03 | 3 | RT-07 | input/level/output/bypass per-sample proof | pending |

---

## Regression Gates (must stay green)

- `[pressure-release]`
- `[oversized-block]`
- `[true-bypass]`
- `[realtime][static][integration]` (no new alloc tokens / no sendParam store)

## Explicit non-goals this phase

- Do not green `[posthard]`, `[input-anchors]`, `[shipping-policy]`
