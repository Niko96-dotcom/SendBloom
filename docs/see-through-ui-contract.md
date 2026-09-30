# SendBloom Bright/Clear Shell Contract

Status: current runtime art is embedded from `assets/realism-v6/`; see
[scene assets](ui-scene-assets.md). The product register is
**bright/clear**.  This is a separate product direction from the former black
opaque faceplate, not a global alpha adjustment.

## Earlier renderer construction and internals inventory

The following inventory, optical settings and probe values describe the earlier
`tools/render_ui.py` faceplate. They are historical evidence and must not be
attributed to the current registered scene.

The clear moulding is one generated shell mesh (`plate`) with a 2.35 mm wall,
2.10 mm top skin, cavity ceiling, sidewall, and bottom return.  The rear
chassis is an open-top tray with a welded rim and cavity floor.  The exposed
inventory is:

- white-soldermask PCB substrate with procedural weave/routing relief;
- copper pour, traces, pads, vias, and dark silkscreen designators;
- DIP ICs (`U1`, `U2`, `U4`, `U5`, `U6`), QFP (`U3`), film capacitors
  (`C1`–`C8`), axial resistors (`R1`–`R8`), diodes (`D1`, `D2`),
  electrolytic cans (`E1`, `E2`), and headers (`H1`, `H2`);
- red signal, graphite ground, and gate flying leads with slack;
- four nylon standoffs with brass inserts;
- a raised U-shaped pressure-sensor daughterboard with routed dark FR-4 edge,
  white mask, four inter-board spacers, plated vias, signal/return traces, and
  two sparse sensor packages (`S1`, `S2`);
- the back of every potentiometer, the gate switch, dark button, clip LED, and
  both jack bodies/lugs;
- frosted legend carriers on the cavity side of the lid;
- dust/wear and the side-plug mechanical chain.

`_validate_internals_layout()` checks board containment, panel keep-outs, and
duplicate reference designators before any render starts.  The current source
check reports 28 footprints, 9 panel keep-outs, no overhangs, and no keep-out
intrusions.

## Optics and rig

- Polycarbonate IOR: `1.585`.
- Shell tint: `ShaderNodeVolumeAbsorption`, density `0.0060`; tint follows path
  length rather than shell base colour.
- Frost zones use a rough transmissive material with a separate volume.
- Cycles transmission bounces: `24`; total bounces: `32`; alpha remains opaque.
- One rig-level gain (`RIG_GAIN = 0.55`) drives key, fill, kicker, rear rim, and
  interior wash.  The rear rim lights the far shell wall; `board_wash` targets
  the cavity/PCB, not the control plane.
- `clear_shell_specular_flag` is glossy-ray-only and derived from the camera
  pitch, so the key does not turn the lid into a white mirror.

## Bright-register value plan

Measured from the 840×1560 preview composite after post-processing (64 Cycles
samples; values are sRGB 0–255 and are not a hardware/perceptual claim):

| Probe | sRGB |
|---|---:|
| shell over interior | 183.2 |
| plated hardware | 115.2 |
| shell edge | 121.1 |
| darkest print pixel (`min`) | 24.0 |
| frosted print ground | 205.8 |

The validator asserts direction-agnostic separations (`abs(measured[a] -
measured[b])`) rather than assuming that metal or edges must be lighter/darker.

## Deliberately opaque surfaces

- the live preset-name face and runtime labels, so text remains legible over
  changing internals;
- the readable face of every knob, including its pointer insert;
- the quiet `SENDBLOOM` / `NIKO MUSIC` identity carrier and preset hardware,
  which are physical opaque inserts;
- the clip LED lens, gate phenolic cap, dark-mode cap, and pressure treadle,
  because these are user-contact/readout surfaces rather than shell windows;
- PCB component bodies and jack/control backs, each opaque by construction so
  the shell has real value-separated internals instead of a texture placeholder.

Permanent panel legends and scale marks are second-surface geometry at
`SECOND_SURFACE_PRINT_Z`, with frosted zones below them and the populated board
behind.

## Runtime state matrix

`scripts/capture-ui-state-matrix.sh` captures the real `EditorSnapshot` in 23
states: default, Dark, Gate Pre/Post, Send, released pressure, Clip, Advanced,
Bypass, opposing rotary values, all-rotary minimum/centre/maximum, open preset menu with the longest factory name, longest
factory preset, Custom preset, LOAD/SAVE hover/focus/pressed feedback, and
pressed Advanced. Every state
is captured at both 840×700 and 1680×1400.  The script verifies dimensions and
fails closed on a missing snapshot.  These composites are the 1x/HiDPI review
surface; isolated render assets do not substitute for them.

## Evidence boundary

The earlier source validator describes that renderer only. Current scene
validation checks asset dimensions, frame counts, distinct layer crops, and
manifest hitboxes; the snapshot matrix shows runtime registration. They do not
prove human perceptual equivalence to a
physical clear enclosure; that remains a `human_needed` listening/visual check.
