# SendBloom UI render notes

All faceplate art in `resources/ui/` is path-traced by `tools/render_ui.py`
(Blender 5.2 LTS, Cycles, Metal GPU). One procedural scene builds the whole
pedal; every asset is a crop of that scene from the same orthographic camera,
so lighting, contact shadows, AO and reflections agree across parts by
construction.

Re-run everything:

    /Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup \
        -P tools/render_ui.py -- all

Targets `background`, `knobs`, `states` render subsets; `--preview` drops to
64 samples for look-dev.

## Light rig (one rig, no per-part cheats)

| Light | Type | Values |
|---|---|---|
| Key | Sun | 2.7 W/m², cool (0.93, 0.955, 1.0), 9° disc, elevation 62°, azimuth = `lighting::toLight` (-0.55, -0.83) |
| Soft pool | Area 480×480 | 5.5e6 W, same direction as key, distance ~1.9k px — soft shadow wrap + the pool the glossy caps reflect |
| Fill | Area 380×1200 | 3.6e6 W, warm (1.0, 0.70, 0.52), camera-right — the warm spill the old photo background had |
| World | Gradient env | dark warm floor → cool zenith, strength 0.42 — what gloss reflects where no light lands |

The ratio is deliberately sun-heavy: the directional key is what makes the
stipple and scratches cast micro-shadows; a softbox-dominant rig was tried
first and produced exactly the washed "glowy" look it was replaced for.

A sun key was a deliberate choice: its irradiance is uniform across the
plate, so the one filmstrip per knob size is exactly right at every knob
position in a row. View transform is `Standard` (not AgX/Filmic) so the badge
and accent textures keep their sRGB values; plate calibrated to ~#efeae4
against the previous UI's #f0eee8.

## Imperfection system (the anti-CG layer)

Uniform surfaces read as CG, so every material carries wear:

- **Plate stipple is the real product texture**: `plate_stipple.png` is
  derived deterministically from the reference photo
  `UI SENDBLOOM/PNG/back plate.png` (sharp centre crop, FFT high-pass to
  strip the photo's lighting, torus cross-fade to tile seamlessly). It drives
  the plate's bump, a matte-fleck roughness modulation AND a subtle albedo
  fleck, tiled at ~215 px so the grain lands ~2 px on screen.
- **Roughness variation everywhere** (`add_rough_variation`): every material's
  roughness wanders a few points across the surface — handling smudges,
  uneven finish. This is the single biggest de-glow lever.
- **Scratches**: two directions of sparse, heavily stretched noise windowed
  into thin streaks; they cut roughness (shiny bottoms), groove the normal,
  and read only when the light catches them.
- **AO grime** (`add_ao_grime`): occlusion-driven warm dirt multiplied into
  the base colour, so crevices, part seams and the badge edge collect grime.
- **Panel waviness**: a very large, very soft normal modulation so the plate
  sheen never reads as an optically perfect plane.
- **Print relief on art slabs**: the badge/preset/button art drives a small
  bump, so the enamel print sits a hair proud of its ground — this is what
  cured the "milky glass" nameplate — plus painted-metal orange peel.
- **Film grain + vignette**: a seeded 2.6/255-sigma monochrome grain
  (2-px correlated) and a gentle photographic vignette (~13% at the frame
  corners, centred on the key's pool) are added to the background in post
  (`post_background`).

**Author and verify detail at 1x.** The editor displays the 2x renders
downscaled, and the first realism pass buried all its texture at 1–2 px
frequencies that averaged away to nothing at display size — it looked great
zoomed and identical to the glow pass in the plugin. Every imperfection is
now tuned at 4-ish px (2x) and checked on a box-downscaled 1x image against
the old UI. If you retune any of it, judge it at 1x, not on the raw render.

## Dual-resolution art in the plugin

JUCE's `highResamplingQuality` low-pass filters a 2:1 downscale hard enough
to erase the stipple and grain entirely, so the painter never lets JUCE
downscale the renders: `boxHalveImage` (PedalFaceplatePaint) derives an exact
2x2 box-averaged 1x variant of every asset (and of each knob filmstrip) at
load, and `wantsHiResArt` picks hi/lo per paint from the context's physical
pixel scale. Standard-DPI hosts draw the 1x art 1:1 (texture intact); hi-DPI
backing stores get the 2x originals.

## Tooling landmine, fixed in passing

`juce::FileOutputStream` opens in append mode. EditorSnapshot used to write
each PNG onto the end of any existing file, so a re-generated snapshot kept
showing its first-ever content to every decoder while quietly growing — an
extremely convincing impression of stale assets. EditorSnapshot now deletes
the target first (SvgSnapshot already did). If a snapshot ever seems
impossibly stale again, check the file size.

## Composition contract

- `pedal_background.jpg/png` (840×1560): plate, chassis, screws, badge, preset
  furniture. Knob contact shadows are baked in — knobs are rendered as
  camera-invisible occluders, using smooth flute-dip-radius proxies so the
  baked dark core can never peek past the strip silhouette at any rotation.
- `knob_{large,small}_strip.png`: 65 frames (odd → frame 32 is a true centre
  detent at 12 o'clock), 288° linear sweep from 216° matching
  `setRotaryParameters(1.2π, 2.8π)`. Frames are 168/132 px (2×). The knob
  alone, alpha, no shadow (the background carries it — it doesn't change with
  rotation).
- `gate_pre/post`, `footswitch_up/down`, `dark_off/on`, `clip_off/on`: each
  state is the complete assembly rendered over a shadow-catcher plate, so the
  overlay carries its own state-correct soft shadow. Both states of a pair
  share camera + crop → pixel-exact registration. Overlay alpha is feathered
  over the outer 14 render px so the crop boundary never cuts a shadow
  gradient visibly.
- Art rects live in `ART_RECTS` (render_ui.py) and are mirrored as
  `kGateArt/kFootArt/kDarkArt/kClipArt` in `PedalFaceplatePaint.cpp`. Keep in
  sync by hand.

No host-drawn text is baked (knob captions, GATE/PRE/POST, CLIP, PRESSURE
SEND, ADVANCED, preset name). "DARK MODE" and the Niko FX badge are product
artwork and stay in the render; the badge, preset field and load/save button
faces are textured with the existing 2D art.

## Samples and times (Apple M5 Pro, Metal, after kernel warm-up)

- 384 samples, adaptive, OpenImageDenoise. Seed fixed (7) for determinism.
- Full run: **5 min 09 s** wall. Background 26 s; state overlays 2–8 s each;
  strip frames ~2.0–2.6 s each (130 frames).
- First-ever run adds ~3 min of Metal kernel compilation.

## Sizes and the honest trade-off

| Asset | Size |
|---|---|
| pedal_background.jpg (embedded) | 313 KB |
| pedal_background.png (reference, not embedded) | 1.8 MB |
| knob_large_strip.png | 2.31 MB |
| knob_small_strip.png | 1.51 MB |
| 8 state overlays | ~335 KB total |
| **Embedded total** | **~4.5 MB** |

The previous pass got embedded assets down to 542 KB; filmstrips undo that,
knowingly — 65 path-traced frames per knob size is what buys "the lighting
never rotates". Residual render noise, not resolution, dominates the PNG
entropy (bit-depth quantisation was tested and bought <40% at visible banding
risk; RGB under alpha=0 is zeroed, which is free). If the budget ever matters
more than the sweep smoothness: 33 frames ≈ half the cost, still odd-centred;
`pngquant` (not installed here) would roughly halve it again.

## Fidelity trade-offs made

- "DARK MODE" lettering is moulded proud of the cap rather than engraved
  (reference photo shows engraving); booleans on text geometry were not worth
  the fragility, and raised moulding is period-correct for rubberised caps.
- The footswitch is the current product's rectangular treadle pad, not the
  round vintage switch in the reference photo folder — this is a re-render of
  the shipped design, not a redesign. Its two states are treadle tilt
  (raised toward the key light vs. flattened).
- The small-knob strip is rendered with the knob parked at the plate's
  horizontal centre of its row, so the baked fill-light gradient is the row
  average; the residual left/right fill difference between INPUT and OUTPUT
  positions is below visibility.
- The bench/scene around the chassis is a clean dark studio sweep rather than
  the old moody workbench photograph.
