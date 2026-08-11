# Niko ClearShell contract — SendBloom

SendBloom belongs to the **Bright** Niko ClearShell register.  It keeps its
vertical pressure-pedal archetype and deliberately does not inherit desktop or
stompbox controls from other products.  The editable construction source remains
`tools/render_ui.py`; this contract records the portfolio rules that the local
implementation proves.

## Surface and live information

- The clear polycarbonate lid, populated PCB, component families, internal
  leads, and frost legend carriers are genuine construction elements, not a
  transparent overlay or empty cavity.
- Live preset text, labels, control faces, LEDs, logo/preset carriers, and
  readable values are deliberately opaque.  They remain dynamic JUCE state;
  permanent panel print stays registered hardware art.
- The established vertical pedal layout, tangible rotary/treadle interactions,
  keyboard focus, reset, tooltip, and accessibility behavior remain product
  specific and authoritative.

## Shared interaction and host vocabulary

- **Vendor:** generated host metadata says `Niko Music`; the established
  `com.nikoaudiolabs.sendbloom`, `NkMo`, and `SbLm` identifiers remain stable.
- **Presets:** the pedal selector uses concise names such as `Sparkle Verb`.
  The host program API returns `Factory: Sparkle Verb`, making factory state
  distinct from a custom/project program without polluting the compact UI.
- **Mix:** SendBloom is a send effect.  Its real send/pressure behaviour stays
  visible; no generic dry/wet control is invented merely for portfolio symmetry.

## Latency truth

The production ProperSRC topology has measurable non-zero priming latency.
Normal PDC, dry, and bypass routes must remain aligned to that live prepared
latency.  SendBloom therefore advertises no VST3 low-latency function and no
cosmetic Cubase mode; this is intentional and covered by the latency contract.

## Required evidence for an updated delivery

Copy-off exact-SHA Release bundles, full CTest, UI state evidence at 1x and
HiDPI, strict signatures, official VST3 validation, pluginval, AU validation,
reversible installed-bundle comparison, and a separately observed Cubase result
are required.  A render, source build, or validator alone is not an installation
or host claim.
