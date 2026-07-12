---
phase: 23
slug: input-level-gate-truth
status: approved
shadcn_initialized: false
preset: none
created: 2026-07-12
---

# Phase 23 — UI Design Contract (Display Truth Only)

> Minimal display-truth contract for Input / Level / Gate Sens. **Not a redesign** — no layout, color, or hotspot changes.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | none (existing JUCE pedal UI) |
| Preset | not applicable |
| Component library | existing `PedalKnob` / `AdvancedDrawer` |
| Icon library | none |
| Font | unchanged faceplate typography |

---

## Scope Fence

**In scope**
- Input knob value readout must call `ParameterCurves::inputGainDb(norm)` and show signed dB matching DSP anchors (−9 / 0 / +9 at 0 / 0.5 / 1)
- Gate Sens (advanced) value readout must call `ParameterCurves::inputThresholdDb(norm)` and show canonical threshold dB
- Level knob may keep its existing numeric formatter (wet-return amount); DSP already wet-only — no new Level dB faceplate requirement

**Out of scope**
- Faceplate art, hotspot geometry, colors, fonts, Pressure Mode copy, branding (Phase 25)
- New knobs, ranges, or parameter IDs
- Gate Pre/Post toggle visual redesign

---

## Copy / Display Contract

| Control | Parameter ID | Display source of truth | Example at mid |
|---------|--------------|-------------------------|----------------|
| Input | `input_gain` | `ParameterCurves::inputGainDb` → `"±X.XX"` dB style (existing signed formatting) | `0.00` or `-0.00` at 0.5 |
| Level | `level` | existing 0–1 numeric (unchanged) | `0.50` |
| Gate Sens | `input_threshold` | `ParameterCurves::inputThresholdDb` → dB string | e.g. mid-skew threshold dB |

Rules:
1. No duplicated Input/Gate Sens arithmetic in the editor — formatters call `ParameterCurves` only (CORE-02, CORE-07).
2. Parameter IDs unchanged (CORE-06 / UX-01).
3. Gate Sens remains advanced-drawer only.

---

## Verification

| Check | Command / evidence |
|-------|--------------------|
| Input anchors | `Builds/Tests "[input-anchors]"` |
| Display uses curve | source scan: Input formatter references `inputGainDb`; Gate Sens references `inputThresholdDb` |
| No layout churn | `PluginEditor` / faceplate bounds unchanged except formatter lambdas |

---

## Anti-Goals

- Do not redesign the pedal UI
- Do not green `[shipping-policy]`
