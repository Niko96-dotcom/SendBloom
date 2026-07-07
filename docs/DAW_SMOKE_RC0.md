# SendBloom v1.0.0-rc0 — DAW Smoke Checklist

**Purpose:** Manual host verification before promoting RC0 to final v1.0.0.  
**Status:** Instructions only — do not mark passed until each step is performed in the target DAW.

**Build under test:** Release AU (macOS) or Release VST3 as noted per host.  
**Suggested session:** Electric guitar or similar mono source on an insert; stereo playback.

---

## Logic Pro — AU

1. **Plugin loads**
   - Insert **SendBloom** (AU) on an audio track.
   - Confirm the plugin appears in the plug-in manager and instantiates without error dialogs.

2. **UI opens**
   - Open the plugin editor; confirm the pedal UI renders at expected size with no blank panels or missing controls.

3. **Audio passes**
   - Play audio through the track with default settings; confirm dry signal is audible and wet bloom is present when `level` is raised.

4. **Presets load**
   - Step through all eight factory presets via the host preset menu or in-plugin preset selector.
   - Confirm each preset loads distinct settings and audio character changes accordingly.

5. **Bypass works**
   - Toggle host bypass and in-plugin bypass (if exposed); confirm clean dry passthrough when bypassed and normal processing when engaged.

6. **Wet/dry routing behaves correctly**
   - At `level` minimum: dry guitar passes, wet path is inaudible.
   - At `level` maximum: wet return is clearly audible while dry tap remains un-gated and undistorted.
   - Raise `distn`; confirm grind affects wet only, not the dry guitar.

7. **Pressure pad / MIDI CC1 send behavior**
   - With send **connected**, hold the pressure pad (or send MIDI CC1 from a controller); confirm momentary wet send increase.
   - With send **disconnected**, confirm pressure pad / CC1 has no effect on send amount.
   - Toggle Firm vs Soft send curve; confirm feel differs at mid send amount.

8. **Stability soak (10 minutes)**
   - For ~10 minutes, toggle bypass, presets, `32k Color`, gates, `distn`, `level`, and pressure pad while audio plays.
   - Confirm: no crash, no NaN artifacts, no unexpected silence, no runaway/exploding output.

---

## Cubase — VST3

1. **Plugin loads**
   - Insert **SendBloom** (VST3) on an audio track insert slot.
   - Confirm Cubase plug-in information shows correct name/vendor and no load failure.

2. **UI opens**
   - Open the plugin window; confirm full UI renders without layout glitches.

3. **Audio passes**
   - Play source material; confirm processed output with audible dry + wet when `level` > 0.

4. **Presets load**
   - Load each factory preset from the VST3 preset bank or host preset list.
   - Confirm parameter values and sound change per preset.

5. **Bypass works**
   - Use Cubase insert bypass and any in-plugin bypass; confirm true bypass vs engaged processing.

6. **Wet/dry routing behaves correctly**
   - Verify parallel routing: dry stays clean at all `distn` settings; wet scales with `level`.
   - Confirm post gate chops wet shortly after input stops (signature bloom-then-chop behavior).

7. **Pressure pad / MIDI CC1 send behavior**
   - Map MIDI CC1 to send or use the on-screen pressure pad with send connected.
   - Verify momentary send lift during hold and release back to stored send amount.
   - Disconnect send in UI; confirm CC1 / pad no longer modulates send.

8. **Stability soak (10 minutes)**
   - Rapidly switch presets, toggle `32k Color`, automate `level`/`distn`, and hammer bypass for ~10 minutes.
   - Confirm: no crash, no NaN/silence/explosion, stable CPU.

---

## REAPER — VST3

1. **Plugin loads**
   - Add **SendBloom** (VST3) to track FX chain.
   - Confirm FX loads without missing-DLL or blacklist warnings.

2. **UI opens**
   - Float or dock the plugin UI; confirm all controls and meters (if any) display correctly.

3. **Audio passes**
   - Route audio through the FX; confirm output is non-silent with sensible default preset.

4. **Presets load**
   - Select each factory preset from the FX preset dropdown.
   - Confirm state recall and audible differences between presets.

5. **Bypass works**
   - Toggle FX bypass (wet dry mix bypass / plug-in bypass per REAPER setting); confirm dry passthrough when bypassed.

6. **Wet/dry routing behaves correctly**
   - Set `level` low: mostly dry. Set `level` high: wet bloom audible.
   - Max `distn`: wet grind increases; dry path unchanged.
   - Test dual-mono behavior with identical L/R input (outputs should match).

7. **Pressure pad / MIDI CC1 send behavior**
   - Send MIDI CC1 on the track (or use pressure pad) with send connected; confirm wet send swells on press.
   - Toggle send disconnect; confirm CC1 is ignored.
   - Compare Firm vs Soft send response curves.

8. **Stability soak (10 minutes)**
   - Loop playback and aggressively toggle controls, presets, bypass, and `32k Color` for ~10 minutes.
   - Confirm: no crash, no NaN/silence/explosion, no runaway feedback.

---

## Sign-off (human only)

| Host   | Format | Tester | Date | Pass |
|--------|--------|--------|------|------|
| Logic  | AU     |        |      |      |
| Cubase | VST3   |        |      |      |
| REAPER | VST3   |        |      |      |

All three hosts must pass before tagging final **v1.0.0**.
