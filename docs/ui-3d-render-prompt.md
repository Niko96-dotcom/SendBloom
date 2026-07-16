# SendBloom UI — 3D render pipeline brief

Hand this to the agent doing the UI. It is the whole job: model, light, render,
and wire a photoreal faceplate for a JUCE plugin, replacing hand-drawn art.

Everything in "Ground truth" below was measured from this repo or verified by
rendering. Do not re-derive it, and do not trust anything here that you can
cheaply check — but do check before contradicting it.

---

## The job

SendBloom is a guitar-pedal plugin. Its UI is currently vector and bitmap art
composited at runtime, and it reads as **parts floating on a panel** rather than
one photographed object. Fix that the way UAD, Waves and Soundtoys do: model the
pedal in 3D, light it once, path-trace it, and ship the renders.

Success is a faceplate where a stranger cannot tell which parts move.

## Why the current UI floats — learn from these, they are paid-for lessons

Three failures already diagnosed here. Your design must make each one impossible.

1. **The lighting disagreed.** The plate's key light is upper-left. The knob art
   was lit upper-right. An object lit from the opposite side to its panel cannot
   look seated, no matter how good it is. In a single 3D scene this cannot
   happen — that is the main reason to do it this way.

2. **The shadow was drawn, not rendered.** Contact is currently a JUCE
   `DropShadow` ellipse: a uniform blur at a fixed offset. Real contact darkens
   tightly at the base and falls off nonlinearly, and the knob occludes the
   plate's own texture. The eye knows. **Bake it.**

3. **The knob was one image, rotated.** Rotating a single bitmap rotates its
   lighting with it — physically impossible; the room light does not move when
   you turn a knob. `PedalKnob::paintShading` exists solely to hand-paint a fixed
   gloss back over the spinning art. **Filmstrips delete that function.** Render
   each angle as its own frame, lit correctly for that angle.

## Ground truth (measured — build on it)

**Editor**: fixed 420 x 780. Not resizable. Render at **2x** (840 x 1560) for
hi-DPI, and let JUCE downscale.

**Key light**: `lighting::toLight = { -0.55, -0.83 }` — upper-left, normalised-ish.
Shadows fall lower-right. Every part shares this. Keep it.

**Plate**: `kPlate { 38, 60, 345, 642 }`, corner radius 19. Cream, stippled
texture. Centre line x = 210.

**Layout rects** (`source/ui/PedalFaceplatePaint.h`, namespace `facelayout`) —
these are the contract; art must land on them exactly:

| Part | Rect (x, y, w, h) |
|---|---|
| Logo nameplate | 85, 70, 250, 52 |
| Preset field | 54, 129, 232, 42 |
| Preset load / save | 294,136,30,29 / 332,136,27,29 |
| Knobs, large (x3) | DISTORTION 54,184 · SIZE 168,184 · LEVEL 282,184 — each **84 x 84** + 16 caption |
| Knobs, small (x2) | INPUT 86,314 · OUTPUT 268,314 — each **66 x 66** + 16 caption |
| Clip lens | 194, 328, 32, 32 |
| Dark-mode button | 84, 438, 74, 74 |
| Gate toggle | 272, 444, 56, 62 |
| Footswitch | 138, 546, 144, 145 |

**Knob rotation** (`PedalKnob`): `setRotaryParameters(1.2π, 2.8π)` — start 216°,
end 504°, i.e. a **288° sweep** clockwise from lower-left to lower-right.
Value 0 = 216°. **The art must point at 12 o'clock at rest.** This was wrong
before (the art rested at -15.2°) and every reading was off by that much.

**Colours**: ink `0xff161413`, orange accent `0xffe66c0b`.

**Captions are drawn by the plugin** (`PedalKnob::paintCaption`, `drawEngravedText`).
Never bake "SIZE", "INPUT", "VALUE", a preset name, or any lettering the host
draws into a render. This has bitten this project twice.

## Environment (verified working — do not re-litigate)

- **Blender 5.2.0 LTS** at `/Applications/Blender.app/Contents/MacOS/Blender`
- **Cycles + Metal GPU** confirmed on an Apple M5 Pro (20-core). Cycles is not in
  the engine enum under `--factory-startup`; enable it:
  ```python
  import addon_utils; addon_utils.enable("cycles", default_set=True, persistent=True)
  scn.render.engine = "CYCLES"
  cp = bpy.context.preferences.addons["cycles"].preferences
  cp.compute_device_type = "METAL"; cp.get_devices()
  scn.cycles.device = "GPU"
  ```
- Headless: `Blender -b -P script.py`. Use `--factory-startup` for reproducibility.
- **First render compiles Metal kernels (~3 min).** Later renders are far faster.
  Do not conclude the GPU is unused.
- A working smoke test is in this repo's history: a lathed knob, orthographic
  camera, cold key upper-left + warm fill right, path-traced. Start from it.

## Method

**Model procedurally, in Python. Not by hand, not with a text-to-3D generator.**
A knob is a lathe: a 2D profile revolved 360° (`SCREW` modifier), plus fluting
instanced around the rim. Precision objects want parametric code, and code is
reviewable, diffable and re-runnable. Text-to-3D tools make organic game assets
and are the wrong instrument here.

**Camera: orthographic, straight down.** No perspective — a plugin panel is a
flat surface viewed square on. `cam.type = "ORTHO"`.

**Light once, physically.** Cold key upper-left (matching `toLight`), warm fill
right, plus an HDRI so gloss catches something to reflect. Balance them; do not
add per-part lights.

**Render the whole pedal in one scene.** This is the rule that kills floating.
The AO, contact shadows and bounce light between a knob and the plate must be
computed by Cycles, in one scene, with everything in place. Then:

- Export the **background plate** with the parts' shadows and AO **baked in**.
- Export each **moving part** separately with alpha, over the same scene, so its
  own shading matches the plate it was lit against.

**Filmstrips for anything that moves.** Vertical sprite sheet, one frame per
angle. **Odd frame count** (65 or 129) so there is a true centre detent. Sweep
the 288° the plugin actually uses. No easing — linear angle per frame.

**Two-state parts** (gate up/down, footswitch up/down, dark on/off, clip on/off):
render each state from the same camera in the same scene, so they register
exactly and only the moving element differs.

## Deliverables

1. `tools/render_ui.py` — one headless Blender script that builds the scene,
   renders everything, and writes to `resources/ui/`. Re-runnable, deterministic,
   commented. This is the source of truth for the art; treat it as code.
2. `pedal_background.png` (840 x 1560) — plate with all baked shadows/AO.
3. `knob_large_strip.png`, `knob_small_strip.png` — vertical filmstrips, 2x,
   alpha, odd frame count.
4. Two-state PNGs for gate / footswitch / dark / clip, 2x, alpha, registered.
5. A JUCE `FilmstripKnob` (or a `PedalKnob` change) that blits frame
   `round(value * (frames-1))` instead of rotating — and **deletes
   `paintShading`**, which exists only to compensate for rotating one image.
6. A short `RENDER_NOTES.md`: light rig, sample counts, frame counts, render
   time, and any fidelity trade-off you made.

## Verify by rendering — the tooling is already here

This repo has a working loop. Use it; do not invent another.

- `./Builds/EditorSnapshot artifacts/foo.png` renders the real editor to a PNG.
  Flags: `--advanced --dark --gate-pre --send --clip`.
- Compare against the current build **numerically**, not by eye. A previous
  change looked perfect at 6x zoom and had a worst-case error of 254/255 in a
  band nobody thought to check. Diff the whole editor, not the region you expect
  to change.
- `python3 tools/validate_svg_juce.py <dir>` if any SVG survives.

**Do not judge a render in a browser or an image viewer alone.** Put it in the
plugin and look at it there.

## Rules

- **Never bake text the plugin draws.**
- **Never rotate a single knob image.** That is the bug you are here to remove.
- **One light rig for the whole scene.** No per-part cheats.
- The art must land on the `facelayout` rects unchanged. If a rect genuinely has
  to move, say so and why — do not silently reflow the panel.
- Keep the pedal recognisably SendBloom: warm cream plate, orange accent, Niko FX
  nameplate. This is a re-render of an existing product, not a redesign.
- Report render times and file sizes. The last art pass took embedded assets from
  4.32 MB to 542 KB; do not casually undo that.

## Order of precedence when these conflict

1. **It looks seated.** One object, one light, no floating parts. That is the
   entire point of this exercise.
2. **It is correct.** Pointer at 12 o'clock at rest, states registered, rects
   honoured, no baked captions.
3. **It is faithful to the reference photos.**
4. **It is small.**
