# SendBloom 1.0.0-rc0 DAW Smoke and Soak Evidence

No row is a PASS until a real tester supplies host version, OS, date, result, and notes. Automated plugin tests are not substitutes for this evidence.

## Required checks in every host

- Instantiate, open editor, and recall all eight presets.
- Always-on audio; Pressure Mode dry-at-rest, press, release, and preserved tail.
- MIDI CC1; Pre/Post gate placement (including flipping the switch over a live tail); DARK; consistent fixed-rate reverb behavior across realtime/offline rendering and the host's supported sample rates.
- Stereo true bypass; Input, Level, Distn, and Output automation.
- Normal PDC at 44.1 and 48 kHz: compare an engaged, APVTS-bypass, and
  host-bypass impulse or short transient to a dry reference in both realtime
  and offline rendering. Record the host's reported delay, project, and export
  with the receipt.
- Run a minimum 10-minute abuse/soak with repeated automation and bypass.

Cubase additionally checks automation read/write, MIDI routing, the normal-PDC
comparison above, and unusual block/offline parity where configurable. Do not
claim Cubase Low-Latency Mode: the shipping ProperSRC topology is nonzero until
a separately measured latency-free route and host receipt exist. REAPER
additionally checks variable block sizes, offline render, and mono/stereo
tracks.

| Host | Version | OS | Format | Tester | Date | Smoke | 10-minute soak | Notes |
|---|---|---|---|---|---|---|---|---|
| Logic Pro | Not supplied | Not supplied | AU | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
| Cubase | Not supplied | Not supplied | VST3 | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
| REAPER | Not supplied | Not supplied | VST3 | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
