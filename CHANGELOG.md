# Changelog

All notable changes to SendBloom are recorded here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/) and
this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

The version at the top of this file must match the canonical `VERSION` file.
`scripts/release/check-version-consistency.sh` enforces that and blocks the
release otherwise. Historical entries below are never rewritten on a bump.

## [Unreleased]

Nothing yet.

## [1.0.0]

First public release of SendBloom — a gated dirty ambience guitar effect,
shipping as an Audio Unit and a VST3 for macOS.

### Added

- Parallel pedal topology: a unity dry path taken before input gain that is
  never gated and never distorted, alongside a gated, overdriven wet return.
- Fixed-rate reverb tank built as an FV-1-class allpass ring — four
  `allpass → delay → shelving` blocks fed by four series input diffusers,
  sized to the 32,768-word delay-RAM budget the reference chip class's
  manufacturer publishes.
- Bandlimited host-rate ↔ 32,768 Hz conversion on every host sample rate via
  r8brain ProperSRC (`FixedRateAdapter`), so behaviour does not change with the
  session rate.
- Size range of 1.2 s to 6.0 s. The ring recirculates over roughly 827 ms and
  cannot decay faster; that floor is a property of the modelled hardware class.
- Wet-only overdrive with an independent `distn` blend.
- Dual gate placement: Pre for hum suppression, Post for a wet chop of 15 ms
  or less.
- Momentary pressure send, driven from the on-screen pad or MIDI CC1, applied
  at sample-accurate positions within the block.
- Optional Extended Stereo wet return in the Advanced drawer.
- Eight factory presets with host save/load round-trip.
- Pedal-style interface with clip LED, dark-mode artwork, and an advanced
  drawer.
- Zero reported latency and a mono-first wet return.
- Release engineering: single canonical version source, fail-closed
  build/sign/notarize/package/publish pipeline, artifact manifest with
  provenance, hosted-artifact revalidation, and release-script regression
  tests. See `docs/release.md`.

### Changed

- The reverb tank was rebuilt from the previous Schroeder comb design to the
  allpass ring described above. The legacy comb, accumulator, and host-rate
  engines remain in the tree for diagnostics only and are not reachable from
  the production path.
- The customer-facing 32k Color parameter was removed. There is one
  permanently enabled reverb path at every host sample rate. Beta state
  containing `authentic_color` is tolerated and discarded on load.

### Removed

- Speculative 9-bit parameter quantisation. It is not part of the production
  signal path.
- The unimplemented Dirt OS control. It is outside the shipping parameter
  contract.

### Known caveats

- SendBloom is original software inspired by publicly described gated-ambience
  behaviour. It is not a verified match to any hardware product and is not
  firmware-derived. See `CLAIM_STATUS.md`.
- Hardware reference comparison grids and a dated blind listening verdict are
  outstanding; both are tracked as `human_needed` in `CLAIM_STATUS.md`.
- Windows and Linux VST3 builds run in CI but are not part of the signed
  public release.

## [1.0.0-rc0]

Internal release candidate. Never published as a hosted release; the
`v1.0.0-rc0` tag points at a superseded commit. Retained here as history.

- Established truthful pressure-mode rest/press/release behaviour.
- Sample-positioned MIDI CC1 control.
- Bounded realtime span processing.
- Channel-preserving true bypass.
- Corrected Input/Level/Gate behaviour and reverb continuity fixes.

[Unreleased]: https://github.com/Niko96-dotcom/SendBloom/compare/v1.0.0...HEAD
[1.0.0]: https://github.com/Niko96-dotcom/SendBloom/releases/tag/v1.0.0
[1.0.0-rc0]: https://github.com/Niko96-dotcom/SendBloom/releases/tag/v1.0.0-rc0
