---
phase: 09
slug: ui-presets
status: approved
shadcn_initialized: false
preset: juce-pedal
created: 2026-07-06
---

# Phase 9 — UI Design Contract

> JUCE native pedal interface for SendBloom. Guitarist-facing copy only on main face.

---

## Design System

| Property | Value |
|----------|-------|
| Tool | JUCE 7 LookAndFeel (custom `SendBloomLookAndFeel`) |
| Preset | Dark stompbox pedal |
| Component library | JUCE `Slider`, `ToggleButton`, custom `PressureSendPad` |
| Icon library | None (typography + shapes only) |
| Font | System sans (JUCE default), 11px labels / 13px titles |

---

## Window & Layout

| Property | Value |
|----------|-------|
| Editor size | 340 × 520 px (fixed, non-resizable) |
| Body | Rounded rectangle, 12px corner radius |
| Main face | 5 rotary knobs in single row |
| Pad zone | 120 × 100 px centered below knobs |
| Drawer | 200 px slide-down panel below pad |

### Control Map (main face)

| Control | Label | APVTS ID |
|---------|-------|----------|
| Knob 1 | In | `input_gain` |
| Knob 2 | Size | `size` |
| Knob 3 | Lvl | `level` |
| Knob 4 | Distn | `distn` |
| Knob 5 | Out | `output_gain` |
| Toggle | Dark | `dark_mode` |
| Toggle | Gate Pre/Post | `gate_pre_post` (PreSoft / PostHard) |
| Pad | (no label) | `send_connected` + `send_amount` |
| LED | CLIP | InputStage clip-hold (read-only) |

### Advanced drawer (collapsed by default)

| Control | Label | APVTS ID | Notes |
|---------|-------|----------|-------|
| Knob | Gate Sens | `input_threshold` | |
| Choice | Send Feel | `send_feel` | Firm / Soft |
| Toggle | 32k Color | `authentic_color` | |
| Toggle | Extended Stereo | `extended_stereo` | **disabled** |
| Toggle | Dirt OS | `dirt_os` | **disabled** |

Drawer toggle button label: **Advanced ▾** / **Advanced ▴**

---

## Spacing Scale

| Token | Value | Usage |
|-------|-------|-------|
| xs | 4px | LED margin, inline gaps |
| sm | 8px | Knob label gap |
| md | 16px | Row padding, drawer padding |
| lg | 24px | Section breaks |
| xl | 32px | Outer margin |

---

## Typography

| Role | Size | Weight | Usage |
|------|------|--------|-------|
| Knob label | 11px | Regular | Under each knob |
| Section title | 13px | SemiBold | Drawer header |
| Product title | 16px | Bold | Top bar "SendBloom" |
| Drawer labels | 11px | Regular | Advanced controls |

---

## Color

| Role | Value | Usage |
|------|-------|-------|
| Body (60%) | `#2A2A2E` | Pedal chassis |
| Face plate (30%) | `#3D3D42` | Knob ring background |
| Accent (10%) | `#E8A838` | Active pad bloom, knob pointers |
| Clip LED on | `#FF3B30` | Clip hold active |
| Clip LED off | `#4A4A50` | Idle |
| Label text | `#E8E6E3` | All labels |
| Disabled | `#6A6A70` | Extended Stereo, Dirt OS |

Accent reserved for: pad bloom gradient, knob value arc, Advanced toggle hover.

---

## Interaction Contracts

### PressureSendPad (UI-02)

1. **Pointer down** on pad → set `send_connected` = true
2. **Drag vertical** (up increases) → map Y position to `send_amount` 0..1
3. **Pointer up** → set `send_connected` = false; pad bloom fades (200 ms timer)
4. **Visual**: radial bloom from touch point; opacity ∝ `send_amount`
5. All parameter writes on message thread only (`setValueNotifyingHost`)

### Clip LED (UI-03)

- Poll `PluginProcessor::isClipHoldActive()` at 30 Hz via `Timer`
- LED fills red when active; dark grey when idle

### Copywriting Contract

| Element | Copy |
|---------|------|
| Product title | SendBloom |
| Main knobs | In, Size, Lvl, Distn, Out |
| Dark toggle | Dark |
| Gate toggle | Gate Pre / Gate Post (two-state) |
| Clip LED | CLIP |
| Drawer | Advanced |
| Gate Sens | Gate Sens |
| Send Feel | Send Feel |
| 32k Color | 32k Color |
| Disabled toggles | Extended Stereo, Dirt OS (greyed, tooltip "Coming soon") |

**Forbidden on main face:** Schroeder, FDN, envelope, algorithm, confidence, graph, RT60, dB readouts.

---

## Preset Menu

- Factory preset combobox in top bar (8 entries)
- Names: Sparkle Verb, Cut Sample Gate, Spacerock Burn, Dry Dub Sends, Dark Bloom, Firm Pressure, Gated Room, Hot Clip
- User save via host preset mechanism (APVTS state XML)

---

## Registry Safety

| Registry | Blocks Used | Safety Gate |
|----------|-------------|-------------|
| N/A (native JUCE) | — | not required |

---

## Checker Sign-Off

- [x] Dimension 1 Copywriting: PASS
- [x] Dimension 2 Visuals: PASS
- [x] Dimension 3 Color: PASS
- [x] Dimension 4 Typography: PASS
- [x] Dimension 5 Spacing: PASS
- [x] Dimension 6 Registry Safety: PASS

**Approval:** approved 2026-07-06
