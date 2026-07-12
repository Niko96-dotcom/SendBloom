# SendBloom 1.0.0-rc0 DAW Smoke and Soak Evidence

No row is a PASS until a real tester supplies host version, OS, date, result, and notes. Automated plugin tests are not substitutes for this evidence.

## Required checks in every host

- Instantiate, open editor, and recall all eight presets.
- Always-on audio; Pressure Mode dry-at-rest, press, release, and preserved tail.
- MIDI CC1; PreSoft/PostHard gate; Dark; 32k Color transition.
- Stereo true bypass; Input, Level, Distn, and Output automation.
- Run a minimum 10-minute abuse/soak with repeated automation and bypass.

Cubase additionally checks automation read/write, MIDI routing, and unusual block/offline parity where configurable. REAPER additionally checks variable block sizes, offline render, and mono/stereo tracks.

| Host | Version | OS | Format | Tester | Date | Smoke | 10-minute soak | Notes |
|---|---|---|---|---|---|---|---|---|
| Logic Pro | Not supplied | Not supplied | AU | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
| Cubase | Not supplied | Not supplied | VST3 | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
| REAPER | Not supplied | Not supplied | VST3 | Niko | Not supplied | `human_needed` | `human_needed` | No observation supplied |
