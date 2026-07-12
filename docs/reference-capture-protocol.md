# SendBloom Reference-Capture Protocol

This protocol produces clean-room, reproducible evidence. It does not itself prove hardware fidelity.

## Allowed evidence and provenance

Only user-created audio recordings of lawfully owned hardware may enter a capture package. Never copy or commit third-party firmware, EEPROM, bytecode, schematics, proprietary dumps, extracted assets, or reverse-engineering material. Keep raw WAV captures outside Git by default; derived metric JSON/CSV may be reviewed before commit. Record operator, UTC date, device model/serial alias, audio interface, sample rate, bit depth, cabling, calibration level, source stimulus manifest SHA-256, capture SHA-256, and notes.

Use 48 kHz/24-bit mono PCM where possible, disable interface processing, leave 2 seconds of silence, avoid clipping (peak below -3 dBFS), and repeat each cell three times. Reject a cell for clipping, dropout, unstable wet subtraction, wrong setting, missing metadata, or hash mismatch.

## Deterministic stimuli

Run `python3 tools/reference/generate_sources.py <output-dir>`. The fixed-seed generator emits impulse, sweep, pink-noise burst, stepped sine, guitar-like pluck, palm mute, sustained chord, riff plus exact silence, repeated controller windows, and a hashed manifest. Re-running with the same sample rate must yield identical files.

## Capture naming and metadata

Name WAVs `<capture-id>__<mode>__<cell>__take-N.wav`. Supply analysis metadata:

```json
{"capture_id":"unit-bright-size50-take1","capture_metadata":{"operator":"Niko","captured_utc":"human_needed","device":"human_needed","stimulus_sha256":"...","capture_sha256":"..."},"settings":{"mode":"bright","size_pct":50,"input_pct":50,"distn_pct":0,"gate":"pre","pressure_pct":0}}
```

Analyze with `python3 tools/reference/analyze_reference.py capture.wav metadata.json --json metrics.json --csv metrics.csv`. Results include metadata/settings with predelay, EDT/RT20/RT30, spectral-centroid series, harmonic ratios, DC offset, and gate-close envelope time. The estimators are deterministic screening metrics, not listening judgments.

## Measurement grids

- Reverb: Size 0/25/50/75/100% in Bright and Dark. Measure predelay, decay, centroid over time, HF decay, modulation, density, and maximum output.
- Distortion: Input 20/50/80/100% × Distn 0/25/50/75/100%. Measure harmonic ratios, transfer curve, small-signal gain, DC, tilt, and clipping onset.
- Gate: Pre and Post at low/medium/high Input, deterministic burst then silence. Measure opening threshold, close decision/envelope, and chatter.
- Pressure: disconnected, connected/rest, 25/50/75/100%, then release with tail; repeat Firm/Soft. Measure feed curve, attack/release, tail preservation, and curve difference.

## Wet isolation and calibration

Capture true-bypass, active Level=0, and active effect. Align the active Level=0 baseline to the effect capture, match dry level/phase, subtract to estimate wet-only response, and record residual error. Do not assume true-bypass is phase-identical to active dry. Change only one subsystem at a time; preserve before renders and safety-gate results. Any tuning requires objective improvement, no safety regression, and Niko listening approval.

## Current gate status

Hardware comparison grids and blind/level-matched listening are `human_needed` until actual captures and Niko's dated verdict exist. Missing hardware is not an automated pass. See `CLAIM_STATUS.md` for the sole current ADR-V1-17 classification.
