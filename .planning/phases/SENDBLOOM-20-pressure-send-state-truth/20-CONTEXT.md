# Phase 20: Pressure Send State Truth - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-accepted smart-discuss recommendations (user requested full autonomous end-to-end)

<domain>
## Phase Boundary

Make Pressure Mode tell the truth from UI and presets: connected-and-resting is dry with decaying tails; release zeros amount without disconnecting; disconnected mode remains always-on wet feed. Covers SEND-01…14 and UX-01…05 (pressure/preset/UI state truth). Does NOT implement span/no-alloc/true bypass (Phase 21), MIDI APVTS purity / per-sample delivery (Phase 22), or branding renames (Phase 25).

</domain>

<decisions>
## Implementation Decisions

### Pressure State Machine (ADR-V1-01)
- `send_connected=false` → always-on wet feed (SEND-01)
- `send_connected=true` + pressure/amount 0 → no new wet input; existing tails continue (SEND-02/04)
- Pressure >0 while connected sends into wet path (SEND-03)
- Release sets `send_amount=0`, leaves `send_connected=true` (SEND-05) — never flip to disconnected on pad release
- Disconnecting restores ordinary always-on reverb (SEND-12); `send_amount` APVTS ID unchanged; default amount 0 (SEND-13)

### UI Pad & Overlay
- On-screen pad `mouseUp`/`release` keeps connected and zeros amount (fix the Phase 19 pressure-release contract)
- Pressed overlay follows pressure/pressed state, not connection alone (SEND-06)
- Advanced UI exposes persistent Pressure Mode connection (SEND-07)
- Pad press may auto-connect without disconnecting on release (SEND-08)
- Prefer minimal UI surgery to achieve state truth; no visual redesign beyond required state/copy (UX-01…05 as mapped)

### Presets & Curves
- Factory pressure presets load connected with `send_amount=0`; always-on presets load disconnected (SEND-11, UX-04)
- XML and `FactoryPresets.cpp` recall identical state
- Firm vs Soft remain distinct (SEND-09); attack ~3 ms / release ~25 ms (SEND-10)
- Behavior invariant across host block sizes for pressure semantics (SEND-14) — full oversized wet path may still wait on Phase 21 span if blocked by dry fallback; pressure state semantics must still hold on supported blocks

### Contract Flip Strategy
- Flip Phase 19 `[v1][contract][pressure-release]` (and related SEND contracts) from intentionally red to green by fixing production behavior — do not delete contracts; update expectations only if ADR wording requires
- Leave unrelated Phase 19 contracts red (oversized, bypass, PostHard, Input, MIDI purity, shipping) for later phases
- Keep BASE-04 discipline for ProperSRC/HF/DryPath/release greens unless a SEND requirement explicitly updates them

### Claude's Discretion
- Exact pad component file edits, advanced UI control placement, and whether SEND-14 full block-size invariance needs a temporary caveat if Phase 21 span is still missing
- Whether to add new green SEND tests vs only flipping existing V1Contract pressure suites

</decisions>

<code_context>
## Existing Code Insights

### Reusable Assets
- Phase 19 `tests/V1Contract*` pressure-release failing contract (target to turn green)
- `PressureSend` / pad UI components; `FactoryPresets.cpp` + `resources/presets/*.xml`
- `tests/PressureSendTest.cpp` curve unit tests
- ADR-V1-01 / ADR-V1-02 / ADR-V1-04 locked in milestone spec

### Established Patterns
- APVTS parameters `send_amount`, connection flag (exact ID per ParameterIDs)
- Catch2 tags `[send]`, `[v1][contract]`

### Integration Points
- UI editor pad handlers; processor wet-input gain from pressure state; factory preset load path

</code_context>

<specifics>
## Specific Ideas

User accepted all recommended smart-discuss answers for autonomous milestone run — prefer recommended defaults; only pause for blockers / verification decisions.

</specifics>

<deferred>
## Deferred Ideas

- Span/no-alloc and true bypass → Phase 21
- MIDI CC1 realtime modulation without APVTS mutation → Phase 22
- Branding / third-party string removal beyond pressure UX copy → Phase 25

</deferred>
