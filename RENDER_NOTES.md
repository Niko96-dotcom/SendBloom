# SendBloom UI render notes

All faceplate art in `resources/ui/` is path-traced by `tools/render_ui.py`
(Blender 5.2 LTS, Cycles, Metal GPU). One procedural scene builds the whole
pedal; every asset is a crop of that scene from the same orthographic camera,
so lighting, contact shadows, AO and reflections agree across parts by
construction.

There is intentionally no hand-edited `.blend` source. `tools/render_ui.py` is
the Blender project: it owns geometry, materials, light rig, camera, render
passes, crops and deterministic output. That keeps every hardware revision
reviewable and reproducible.

## 2026 physical-control and black-wrinkle pass

The first path-traced knob family still read as generic CGI, and its replacement
still failed at plugin size: a broad reflective centre visually inflated the
otherwise-flat control into a smooth "melon" with no obvious place to pinch.
The final family is anchored to measured commercial control proportions instead
of combining several products into an invented silhouette:

- **Main controls:** a tall Davies-1510-inspired pointer knob, 24 broad grip
  lands separated by narrow axial grooves, hard skirt/barrel ledge, recessed
  planar top and long ivory paint index.
- **Input/output controls:** a narrower 20-land barrel with the same genuinely
  vertical pinch wall, smaller planar top and index language.
- Both controls carry a radial chrome set screw and expose a separate stationary
  nickel mounting washer. The
  large hardware is 61 px across inside its 84 px hit target; the trim hardware
  is 40 px across inside its 66 px hit target.
- There is no silver crown, bowed lathe station or specular cap. A separate
  high-roughness planar insert prevents smoothed grip normals and softbox
  reflections from visually turning the top into a dome.
- The visible bodies no longer fill their entire hit-target squares. The spare
  space carries sparse nine/seven-mark screen-printed scales, which is both a
  physical mounting cue and a useful position reference.
- Knob plastic is satin, roughness-varied phenolic/ABS with fine mould grain;
  the pointer is rough paint, not metal.
- Captions and live hardware labels now use a restrained screen-print treatment
  rather than a one-pixel white/black digital bevel. Hover still replaces a
  caption with the exact value, preserving software precision.

The cream insert plate was replaced with a black wrinkle-powder enclosure. Two
procedural noise scales drive its cured-powder height, local charcoal albedo and
dry roughness; warm-ivory pad print and nickel hardware carry the contrast. This
is a new physical material, not the old stipple recoloured black.

## Fully modelled Niko FX nameplate

The badge no longer samples `brand_logo.png`. `brand_logo.svg` is now used only
as an exact silhouette source, imported by Blender and separated into physical
layers:

- the original slanted outer path is a bevelled nickel rim;
- the inner path is a recessed black-enamel bed;
- the large orange Niko field is a separately bevelled baked-enamel inlay;
- N, the two i pieces, k and o are individual blackened-metal solids seated
  through matching apertures in the orange field;
- F and the three stylised X pieces are individual orange-enamel solids rising
  from the black bed;
- the o counter is a real opening with an independently modelled recessed orange
  bottom, so its inner wall generates real occlusion while preserving the
  original two-colour artwork.

All glyph tops finish exactly `PX_PER_MM` (5.70 render units, one physical
millimetre) above the orange inlay. A 0.52-unit edge bevel catches the common
light rig without rounding away the sharp custom lettering. Material grain,
roughness variation, AO grime and four tiny contact-wear marks live on the
actual solids. No shadow, highlight, bevel or letter colour is baked into a
logo texture.

## Fully modelled preset manager

The selector and LOAD/SAVE controls no longer sample `preset_field.png`,
`preset_load.png` or `preset_save.png`. Those files remain archival extraction
outputs only. The production scene now contains:

- a selector panel cut-out, nickel bezel, black insulating gasket, smoked-black
  carrier and separate live-name face;
- a two-layer triangle arrow socket and inlay plus upper/lower guide rails;
- independent LOAD/SAVE wells, nickel carriers, gaskets, smoked-black caps,
  warm-ivory raised legends and orange side-witness grooves;
- a three-layer folder with a retained document for LOAD;
- a six-piece disk with shutter, slot, label and data lines for SAVE;
- separately extruded LOAD/SAVE legends.

Only the selected preset name remains live JUCE text. Cast-surface waviness,
mould grain, roughness variation and AO live on the physical materials. No
image-texture node remains in the production scene.

The camera is orthographic but pitched 13.5 degrees toward the front edge. The
enclosure is dimensioned from a Hammond 1590B: the 60.5 mm face width maps its
31 mm overall depth to 176.8 render units, with a separately modelled 4.19 mm
lid. This replaced the rejected 36-unit shell, which was only about one fifth of
the physical depth implied by its own face scale. A world-space Y compensation
exactly cancels the pitch's foreshortening at z=0, so every printed mark and JUCE
hit target retains its original layout coordinate while raised hardware shifts
correctly with depth.

The corrected geometry uses the measured proportions of Rogan PT-series and
Davies 1900-H controls as constraints, with BOSS/MXR/UAFX layouts informing
spacing and hierarchy. Softube's true-life Amp Room mode and UAD's plug-in
interaction conventions inform the render/interaction split. It remains a
clean-room family rather than a mesh copied from a manufacturer's part.

Re-run everything:

    /Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup \
        -P tools/render_ui.py -- all

Targets `background`, `knobs`, `states` render subsets; `--preview` drops to
64 samples for look-dev.

## Light rig (one rig, no per-part cheats)

| Light | Type | Values |
|---|---|---|
| Key | Sun | 2.7 W/m², cool (0.93, 0.955, 1.0), 9° disc, elevation 62°, azimuth = `lighting::toLight` (-0.55, -0.83) |
| Soft pool | Area 480×480 | 5.5e6 W, same direction as key, distance ~1.9k px — soft shadow wrap + readable rib highlights |
| Fill | Area 380×1200 | 3.6e6 W, warm (1.0, 0.70, 0.52), camera-right — the warm spill the old photo background had |
| Front-wall kicker | Area 520×180 | 1.35e6 W, warm (1.0, 0.78, 0.62), low and frontal — grazing reflection on chassis depth and chrome |
| World | Gradient env | dark warm floor → cool zenith, strength 0.42 — what gloss reflects where no light lands |

The ratio is deliberately sun-heavy: the directional key is what makes the
wrinkle relief and hardware edges cast micro-shadows; a softbox-dominant rig was tried
first and produced exactly the washed "glowy" look it was replaced for.

A sun key was a deliberate choice: its irradiance is uniform across the
plate, so the one filmstrip per knob size is exactly right at every knob
position in a row. View transform is `Standard` (not AgX/Filmic) so the badge
enamel and accent textures retain their intended colour. The plate lands as warm charcoal
with enough local range for the powder relief to survive at 1x.

## Imperfection system (the anti-CG layer)

Uniform surfaces read as CG, so every material carries wear:

- **Black wrinkle powder** uses broad high-detail cured-powder relief plus a
  fine highlight-breaking pass. The same wrinkle signal modulates height,
  roughness and charcoal albedo so the surface cannot read as flat noise pasted
  onto smooth geometry.
- **Roughness variation everywhere** (`add_rough_variation`): every material's
  roughness wanders a few points across the surface — handling smudges,
  uneven finish. This is the single biggest de-glow lever.
- **AO grime** (`add_ao_grime`): occlusion-driven warm dirt multiplied into
  the base colour, so crevices, part seams and the badge edge collect grime.
- **Panel waviness**: a very large, very soft normal modulation beneath the
  powder relief so the plate
  sheen never reads as an optically perfect plane.
- **Raised product furniture**: badge letters, selector rails and arrow, button
  icons and legends are independent solids with their own bevels, materials,
  contact shadows and internal occlusion.
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

## Mechanical-realism and use-history pass (2026-07-22)

The pristine studio render was physically coherent but still did not read like
a working stompbox. The revised scene authors use-history at three scales:

- a scarred, mottled black workbench with clustered drag marks, dropped-plug
  gouges, dust islands and contact shadows beneath the enclosure and leads;
- two weighted 5 mm-class instrument cables that rest on the floor before
  rising into ribbed strain reliefs. Each connection continues through a
  die-cast right-angle shell with seam and retained cover screws, inserted
  shaft, threaded socket nose, chrome hex facia and insulating panel bezel;
- coating damage built outside-in as a raised dark paint lip, brown primer or
  oxide, then a smaller recessed aluminium core. Directional grooves pair a
  dark recess with a narrow light-facing metal shoulder;
- wear placed by a physical cause: edge impacts, jack/tool arcs, screw-driver
  slips, finger/pick contact and treadle shoe travel. The old uniform silver
  freckles and arbitrary face slashes were removed;
- a large broken warm-ivory signal frame whose missing sections are real gaps
  and irregular abrasion masks, not a clean vector outline;
- shortened/faded scale ticks, directional finger/pick scars on every knob cap,
  chipped index paint, chrome set screws, stationary nickel washers and strong
  cast/contact shadows from tall occlusion proxies;
- a dark moulded treadle pad in a chrome carrier, full-width hinge, fasteners,
  traction ribs and only a few shoe-direction scuffs;
- sparse top-plane contact abrasion on the modelled logo letters plus edge wear
  on preset hardware, so bright furniture does not look newly pasted onto an old
  enclosure.

The wear is deterministic geometry plus procedural material response. This is
deliberate: sub-pixel shader noise disappears at 1x, while a 1–8 px chip remains
readable and keeps a stable silhouette across rerenders and moving-part states.

## Final assembly-clearance pass

The complete editor was captured in default, PRE, POST, pressed, dark, clip and
open-drawer states at its actual 420×780 size. The last pass treats print and
software furniture as parts of the assembly rather than independent decoration:

- the lower signal-frame segments have explicit keep-outs around both screw
  heads and the pressure-treadle carrier;
- the PRE/POST legends sit beside the toggle's full lever sweep instead of
  being painted underneath it;
- the closed ADVANCED legend clears the right frame, while the open drawer
  suppresses the underlying PRESSURE SEND label rather than clipping half of it;
- drawer toggles use a purpose-built horizontal track/thumb geometry;
- DARK MODE uses worn warm print that survives 1× and the dim-room overlay;
- the live preset name receives the same one-pixel registration shadow as the
  other dynamic pad print;
- LOAD and SAVE now share one 30×29 mounting size and visual weight.

These are assembly-order rules: enclosure print is applied before hardware is
mounted, so no intact line may pass through a screw, bezel, switch or carrier.

## Gate-toggle hardware pass

The gate control no longer uses the pristine all-chrome joystick construction.
It is now a deliberately small panel-toggle assembly: recessed panel bore,
black phenolic gasket, serrated washer, bevelled aged-nickel locknut, threaded
bushing and rings, compact pivot collar, and a slimmer warm-nickel lever. The
finger contact is a black phenolic cap with only a thin orange enamel identity
band, so its dominant material belongs to the knob and treadle family while the
metal remains a narrow mechanical highlight.

Sparse nut-face tarnish, restrained roughness variation and the smaller lever
reduce the clipped white reflection and long cast shadow of the old part. Both
PRE and POST positions are rendered from the shared scene and inspected at the
editor's actual 420×780 size, including the full lever sweep and adjacent print.

## Dual-resolution art in the plugin

JUCE's `highResamplingQuality` low-pass filters a 2:1 downscale hard enough
to erase wrinkle relief and grain entirely, so the painter never lets JUCE
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

- `pedal_background.jpg/png` (840×1560): plate, chassis, screws, fully modelled
  badge, preset furniture. Knob contact shadows are baked in — knobs are rendered as
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
artwork and stay in the render. The badge is generated from extruded vector
silhouettes; the preset furniture is also modelled in layers. The selected
preset name is the sole live element over that hardware. No image-texture
material path remains in `tools/render_ui.py`.

## Samples and times (Apple M5 Pro, Metal, after kernel warm-up)

- 384 samples, adaptive, OpenImageDenoise. Seed fixed (7) for determinism.
- The final dimensioned mechanical-realism all-assets run was **2 min 39 s**
  wall: background 23.0 s, main frames 0.9–1.3 s, trim frames 0.6–0.9 s, plus
  all eight state-correct overlays. The earlier original all-assets run was
  5 min 09 s.
- The production modelled-logo background pass remained **23.0 s** wall. Knob
  and state overlays were not rerendered because the badge exists only in the
  static background pass.
- The production all-modelled preset-hardware background pass was **24.0 s**
  wall. Knob and state overlays were not rerendered because these controls also
  exist only in the static background pass.
- First-ever run adds ~3 min of Metal kernel compilation.

## Sizes and the honest trade-off

| Asset | Size |
|---|---|
| pedal_background.jpg (embedded) | 235 KB |
| pedal_background.png (reference, not embedded) | 1.71 MiB |
| knob_large_strip.png | 1.63 MiB |
| knob_small_strip.png | 794 KB |
| 8 state overlays | 448 KB total |
| **Embedded total** | **~3.08 MiB** |

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
- The bench is a procedural scarred black work surface rather than a photographic
  plate. Its marks, cables and plugs share the scene lighting and remain fully
  rerenderable with the pedal.
