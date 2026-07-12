# Phase 9: UI + Presets - Context

**Gathered:** 2026-07-06
**Status:** Ready for planning
**Mode:** Auto-generated (autonomous)

<domain>
## Phase Boundary

Guitarist-friendly pedal interface and 8 factory presets. Pedal layout: In, Size, Lvl, Distn, Out knobs; Dark and Gate Pre/Post toggles; PressureSendPad; clip LED; advanced drawer. No graphs, confidence meters, or algorithm language in main UI.

</domain>

<decisions>
## Implementation Decisions

### Pedal UI
- Five main knobs bound to APVTS params
- Dark + Gate Pre/Post toggles on main face
- PressureSendPad: mouse/touch down-drag-up with visual bloom feedback
- Clip LED reflects InputStage clip-hold flag in real time
- Advanced drawer: Gate Sens, Send Feel, 32k Color; Extended Stereo + Dirt OS shown disabled

### Presets
- 8 factory XML presets in plugin bundle
- Save/reload round-trip preserves all parameter values
- Preset menu integration via JUCE

### Visual Style
- Pedal aesthetic appropriate for guitarists; clean, no DSP jargon on main face
- Claude's discretion on colors/fonts within JUCE LookAndFeel patterns

### Claude's Discretion
Exact dimensions, custom component vs stock RotarySlider, drawer animation at planner discretion per UI-SPEC if generated.

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- PluginEditor shell from Phase 1 (minimal)
- Full APVTS param set from Phase 2
- InputStage clip-hold flag from Phase 4
- PressureSend send_amount from Phase 7

### Integration Points
- PluginEditor binds to AudioProcessorValueTreeState
- Presets via JUCE preset mechanism or custom XML in Resources/

</code_context>

<specifics>
## Specific Ideas

- ROADMAP UI hint: yes — generate UI-SPEC before planning if workflow.ui_phase active
- Main UI must not show graphs or algorithm names

</specifics>

<deferred>
## Deferred Ideas

- Extended Stereo / Dirt OS enablement (post-v1, show disabled in drawer)

</deferred>
