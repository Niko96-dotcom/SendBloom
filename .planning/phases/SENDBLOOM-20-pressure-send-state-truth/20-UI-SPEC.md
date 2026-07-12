# Phase 20: Pressure Send State Truth - UI Spec

**Generated:** 2026-07-12
**Status:** Ready for planning
**Mode:** Minimal state-truth contract (not visual redesign)

## Scope

UI work in this phase is limited to Pressure Mode state honesty on the existing pedal faceplate / pressure pad / advanced controls. No layout redesign, no new brand surfaces (Phase 25), no meter chrome changes unless required for SEND-06 overlay correctness.

## Surfaces

### Pressure pad (primary)
- Press: may auto-set `send_connected=true`; drives `send_amount` from press depth
- Release: MUST keep `send_connected=true` and set `send_amount=0`
- Pressed overlay: follows pressed/pressure state, not connection alone

### Advanced / connection control
- Persistent Pressure Mode connection exposed (toggle or equivalent existing control)
- Disconnect restores always-on wet feed; does not require pad redesign

### Copy
- Explain Pressure Mode without third-party controller naming (UX-06 may partially land; full branding Phase 25)

## Non-goals
- New visual design system
- Branding asset replacement
- MIDI learn UI

## Acceptance
- Pad release contract green (`[v1][contract][pressure-release]`)
- SEND-05/06/07/08 observable in UI + APVTS
