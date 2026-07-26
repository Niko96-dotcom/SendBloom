# SendBloom 1.0.0

Gated dirty ambience for guitar, as an Audio Unit and a VST3 for macOS.

The dry guitar stays clean and untouched while a parallel wet path blooms,
overdrives, and then chops hard the moment you stop playing — the "edited
sample" ambience feel, with a momentary pressure send you can slam from the
pedal face or from MIDI CC1.

## What is in this release

- Parallel dry/wet routing. The dry path is a unity copy taken before input
  gain and is never gated or distorted.
- One fixed-rate reverb tank: an FV-1-class allpass ring of four
  `allpass → delay → shelving` blocks fed by four series input diffusers,
  sized to the 32,768-word delay-RAM budget the reference chip class's
  manufacturer publishes. Every host rate is bandlimited-converted to and from
  the 32,768 Hz tank. There is no sample-rate, fidelity, or colour control.
- Size from 1.2 s to 6.0 s. The ring recirculates over roughly 827 ms and
  cannot decay faster; that is a property of the modelled hardware class.
- Wet-only overdrive, blended independently.
- Dual gate placement: Pre for hum suppression, Post for a wet chop of 15 ms
  or less.
- Pressure send pad plus sample-accurate MIDI CC1 momentary control.
- Optional Extended Stereo wet return in the Advanced drawer.
- Eight factory presets with host save/load round-trip.
- Zero reported latency, mono-first wet return.

Full detail: [CHANGELOG.md](CHANGELOG.md).

## Install

Download the macOS disk image and follow [docs/install.md](docs/install.md),
which also covers checksum verification, upgrading, and uninstalling.

Verify what you downloaded before installing:

```bash
shasum -a 256 -c SHA256SUMS.txt
```

## Requirements

- macOS 11 or later, Apple silicon or Intel. The plugins are universal
  binaries.
- An AU or VST3 host.

## Honest caveats

These are stated here so they are not discovered later:

- **Fidelity.** SendBloom is original software inspired by publicly described
  gated-ambience behaviour. It is not a verified match to any hardware
  product, is not firmware-derived, and no exact-emulation claim is made. The
  active classification is `original-inspired`; see
  [CLAIM_STATUS.md](CLAIM_STATUS.md).
- **Outstanding human evidence.** Hardware reference comparison grids and a
  dated blind or level-matched listening verdict have not been supplied. Both
  remain open.
- **Platforms.** Windows and Linux VST3 builds are exercised in CI but are not
  part of this signed public release. macOS AU and VST3 are the released
  artifacts.
- **Host coverage.** Automated validation covers pluginval at strictness 10 on
  the VST3. AU validation and multi-host DAW smoke are recorded per release in
  the release report; where a host was not exercised, the report says so
  rather than implying coverage.

Anything skipped during release validation is named explicitly in that
release's report. See [docs/release-validation.md](docs/release-validation.md).
