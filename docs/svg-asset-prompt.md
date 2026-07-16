# SVG asset prompt

The prompt below is the one to hand to the image→SVG agent. It is the previous
prompt plus the constraints of the renderer the art actually has to survive.

## Why this revision exists

The first asset set was, by every measure the old prompt asked for, a success:
zero rasters, zero filters, genuinely editable geometry, honest audits. It was
also unusable. Of 65 files, 46 rendered wrong or not at all in JUCE — the
backplate as a black slab, the clip and dark-mode buttons as *nothing at all*.

The old prompt caused this. It asked for `<symbol>`/`<use>` reuse, for masks,
and for a symbol library as a deliverable. Those are the three constructs
JUCE's parser does not implement. The agent complied exactly.

The validation missed it because it checked the art against the SVG *spec*
rather than against the renderer. Every rule was about not smuggling raster
data in — which the agent genuinely never did. Nothing asked whether the target
could draw the result.

One package came through flawlessly: the knobs, at 212 elements and 116 KB
each, built from plain paths, circles, ellipses and gradients with no `<use>`
anywhere. That is the proof the budget and the quality bar below are achievable
together — it is a real output of this pipeline, not an aspiration.

---

## THE PROMPT

Create a genuinely scalable, editable SVG reconstruction of the object or
objects in the attached reference image.

IMPORTANT: Start from the reference image itself. Do not reuse, edit, trace,
import, or derive from any previously generated SVG.

The result must be real vector illustration — not a raster image disguised as
SVG — and it must render correctly in a renderer that implements only a subset
of SVG. Both constraints are hard. Meeting one and failing the other is a
failed deliverable.

### TARGET RENDERER — READ FIRST

These files are drawn by JUCE 8's SVG parser (`juce_SVGParser.cpp`), not by a
browser. It implements a subset of SVG and **fails silently** outside it. There
is no error, no warning, no console output — the art simply comes out wrong.

Three failure modes, each verified against the parser source:

- **`<pattern>` fills render as SOLID BLACK.** A fill of `url(#some-pattern)`
  finds the element, rejects it (only gradients are accepted as paint), and
  falls through to the colour parser, which cannot read a URL and returns the
  default — black for any closed path.
- **`<mask>` is ignored entirely.** The masked geometry draws at full strength,
  unmasked.
- **`<use>` pointing at a `<symbol>` or a `<g>` renders NOTHING.** `<use>` is
  resolved only to a *single shape primitive*. Point it at anything that
  contains children and the element vanishes without trace.

A browser will show you a perfect picture. That proves nothing. **Never
validate this work in a browser.**

### HARD PROHIBITIONS

Not permitted, for the raster reasons:

1. No `<image>` elements.
2. No embedded PNG, JPEG, WebP, or other bitmap.
3. No base64 data, `data:image`, blob URLs, or external image links.
4. No `<foreignObject>`, HTML canvas, or CSS background images.
5. No pixel-for-pixel reconstruction.
6. No one rectangle, path, circle, or polygon per source pixel.
7. No grids of tiny rectangles or "vectorized pixels."
8. No full-color automatic raster tracing.
9. No Potrace-style output containing thousands of arbitrary microscopic regions.
10. No clipping a bitmap inside a vector outline.
11. No enormous path whose purpose is to encode raster pixels.

Not permitted, because the renderer cannot draw them:

12. **No `<pattern>`.** Renders solid black. Not "degrades" — black.
13. **No `<mask>`.** Silently ignored.
14. **No `<symbol>`.** Nothing that references one can render.
15. **No `<use>`.** Zero occurrences in the exported file. This is not a
    stylistic preference; see FLATTENING below.
16. **No SVG filters of any kind** — `feTurbulence`, `feGaussianBlur`,
    displacement maps, filter-generated noise. Unimplemented.
17. **No `<style>` blocks, CSS classes, or external stylesheets.** Inline every
    paint attribute directly on the element that uses it.
18. **No `<text>` or `<tspan>`.** Convert all lettering to outlined paths. Text
    depends on fonts the host will not have.
19. **No `mix-blend-mode`**, `filter`, or any CSS visual property.

Also do not claim the result is pixel-perfect or mathematically identical to
the photograph.

### THE ONLY ELEMENTS YOU MAY EMIT

Everything visible must be built from exactly these:

- `<path>`, `<rect>`, `<circle>`, `<ellipse>`, `<line>`, `<polyline>`, `<polygon>`
- `<g>` for grouping
- `<linearGradient>`, `<radialGradient>`, `<stop>` — fully supported, and your
  primary tool for metal, form, and light. Use them freely and richly.
- `<clipPath>` — supported, when it contains only the shapes above
- `<defs>` holding *only* gradients and clip paths
- `<title>`, `<desc>`, `<metadata>`

Supported attributes: `fill`, `stroke`, `stroke-width`, `stroke-linecap`,
`stroke-linejoin`, `opacity`, `fill-opacity`, `stroke-opacity`, `transform`,
`clip-path`, `d`, geometry attributes, `gradientUnits`, `gradientTransform`.

If a construct is not on this list, it does not exist for this project.

### FLATTENING — THE RULE THAT MATTERS MOST

Design with reuse. **Export without it.**

Reuse is a fine way to *think*: one authored pit, scratch, screw or rivet,
placed many times. But every instance must be written into the exported file as
**literal, standalone geometry**, with its transform, position and opacity baked
in. If your generator uses a symbol internally, expand it at write time.

Concretely: where you would write

```svg
<use href="#metal-pit" x="115" y="221" opacity="0.11"/>
```

you must instead write the pit's actual shapes, transformed into place:

```svg
<g opacity="0.11" transform="translate(115,221)">
  <circle cx="0" cy="0" r="2.1" fill="#10150f" fill-opacity="0.55"/>
  <circle cx="-0.55" cy="-0.55" r="0.75" fill="#c3b69c" fill-opacity="0.35"/>
</g>
```

The exported file must contain **zero** `<use>` elements. A file that is 100%
`<use>` instances parses without error and renders as an empty image — this has
already happened, twice, to a full set of buttons.

The same applies to texture: where you would reach for a `<pattern>` tile, emit
the marks themselves as real shapes across the surface.

### GOAL

Reconstruct the reference as closely as practical using meaningful vector
geometry. Match:

- Overall silhouette and proportions
- Position and scale of every component
- Rim thickness and perspective
- Bevels, recesses, grooves, and indicator slots
- Material colors and metallic shading
- Patina, oxidation, chipped paint, grime, pits, and scratches
- Uneven handmade or worn edges
- Lighting direction and dimensional depth

The SVG must remain visually coherent and crisp when enlarged to 800–1600%.

If a photographic texture cannot be reproduced honestly with compact vector
geometry, approximate it with a controlled number of intentional vector marks.
Never smuggle raster information into the SVG, and never reach for a construct
from the prohibited list to get an effect.

### VECTOR CONSTRUCTION REQUIREMENTS

Build the artwork semantically from named, editable groups. For each object,
use groups such as:

`base-shadow`, `outer-rim`, `side-fluting`, `face`, `face-bevel`,
`indicator-slot`, `patina`, `paint-chips`, `rust`, `pits`, `scratches`,
`highlights`, `edge-wear`

Grouping with `<g>` and `id` is fully supported and is how the art stays
editable once `<use>` is off the table. Lean on it.

Use:

- Circles and ellipses for primary forms
- Bézier paths for irregular outlines, chips, stains, and corrosion
- Stroked paths for scratches and machining marks
- Small circles or irregular closed paths for pits
- Linear and radial gradients for metallic form and lighting — these carry the
  material read, so spend your effort here
- Clip paths, containing vector geometry only

Soft shading must come from gradients, since blur filters are unavailable. A
multi-stop radial gradient replaces a blurred highlight; stacked low-opacity
gradient fills replace a soft shadow. This is a real constraint and it is the
one that most rewards care.

Texture must be deliberately designed, not random clutter. Larger corrosion
areas should be irregular connected paths. Scratches need intentional
placement, varied length, curvature, width, opacity, and direction. Chipped
edges should follow the actual reference rather than being evenly distributed.

### COMPLEXITY LIMITS

**The budget counts elements after flattening** — that is, the elements
actually in the exported file. Inlining is not an excuse to exceed it; it is a
reason to be deliberate about how much texture you place.

Target per object:

- Roughly 150–900 meaningful vector elements
- Prefer fewer than 1,500
- Prefer under 750 KB per individual SVG
- No microscopic element grids
- No thousands of near-identical contour fragments

For calibration, a fully successful asset from this pipeline — a worn metal
knob with patina, scratches, pits, fluting and a brass indicator — came in at
212 elements and 116 KB, fully flattened, with zero `<use>`, and renders
perfectly. Match that character. A few hundred carefully placed elements beat
tens of thousands of trace fragments.

If flattening pushes you past the budget, reduce the *count* of texture marks
and make each one better. Do not restore `<use>` to save bytes.

### WORKFLOW

1. Inspect the reference image carefully.
2. Measure the center, width, height, outer radius, face radius, rim thickness,
   indicator position, and perspective of each object.
3. Establish the exact primary geometry before adding texture.
4. Reproduce the silhouette, proportions, and lighting first.
5. Add bevels, shadows, and material gradients.
6. Add distinct patina and damage regions based on the reference.
7. Add scratches, pits, chips, corrosion, and edge wear as separate editable
   groups.
8. Render the SVG to PNG at the reference size.
9. Compare it side by side with the source.
10. Render again at 4× and inspect the geometry at high zoom.
11. Perform at least three deliberate correction passes:
    - Pass 1: geometry and proportions
    - Pass 2: color, material, and lighting
    - Pass 3: scratches, patina, chips, and small details
12. Run the compatibility validation below and fix everything it finds.
13. Continue refining until further changes would only add raster-like noise
    rather than meaningful visual accuracy.

Do not stop at a generic object that merely resembles the category. Match the
individual damage, coloring, and character of each reference object.

### DELIVERABLES

1. One transparent SVG per individual object, square viewBox, sensible padding.
2. One combined SVG preserving the reference arrangement and spacing.
3. One combined transparent version.
4. One combined version on the reference background color.
5. PNG previews rendered from the final SVGs.
6. The source script used to generate the SVG, if construction is procedural.
7. A plain-text structural audit.

**Do not deliver a symbol library.** It cannot be consumed. If the generator
uses symbols internally, that is an implementation detail that must not reach
the exported files.

**State pairs.** If the reference shows a control that has more than one state
(switch up/down, LED on/off, toggle pre/post), deliver *every* state as its own
file, with an identical viewBox and identical registration, so the states can
be swapped without the art shifting. A set of five different switches all in the
"up" position is not a set of states and cannot be used for a two-state control.

Filenames:

- `button_01_true_vector.svg`
- `button_02_true_vector.svg`
- `buttons_reference_layout_true_vector.svg`
- `buttons_reference_layout_preview.png`
- `TRUE_VECTOR_AUDIT.txt`

### VALIDATION

Parse every final SVG and verify all of the following. A failure here is a
failed deliverable regardless of how good it looks.

Raster honesty:

- `<image>` count is exactly 0
- `<foreignObject>` count is exactly 0
- No `data:image`, no `base64`, no external references
- No one-pixel rectangle grid, no per-pixel paths
- No suspicious repeated elements on a regular pixel grid
- All visible texture comes from vector paths, circles, ellipses, gradients,
  and vector-only clipping geometry

Renderer compatibility — every one of these must be exactly 0:

- `<use>` count
- `<symbol>` count
- `<pattern>` count
- `<mask>` count
- `<filter>` count
- `<style>` count
- `<text>` and `<tspan>` count
- Count of elements whose tag is outside the allowlist above
- Count of `fill`/`stroke` values of the form `url(#x)` where `#x` is anything
  other than a `<linearGradient>` or `<radialGradient>`
- Count of `url(#x)` references where `#x` does not resolve

Then confirm the picture is actually there:

- Compute the rendered bounding box of each file. A blank or near-zero bounding
  box means the content was dropped — report it as a failure, never as a pass.
- Confirm the bounding box is close to the intended viewBox. Content far
  smaller than the canvas means most of the art vanished.

### AUDIT

Report per file:

- File size
- Counts: paths, circles, ellipses, rectangles, lines, polygons, groups
- Counts: gradients, clip paths
- Counts: images, filters, patterns, masks, symbols, uses — each must be 0
- Rendered bounding box, and whether it matches the intended viewBox
- Whether any raster data or external resources exist
- A brief explanation of how the texture was constructed
- Confirmation that every `url(...)` paint reference resolves to a gradient

Any rectangle must have an obvious semantic purpose — an optional background,
say. Explain every rectangle. "A pattern tile" is no longer a valid purpose.

### ACCURACY AND HONESTY

Call the result a "visually faithful true-vector reconstruction," not an exact
photographic duplicate.

Order of precedence when these conflict:

1. **Renders correctly in the target renderer.** Art that does not draw is
   worth nothing, however faithful it is in a browser.
2. **Genuine vector construction.** Never resolve a conflict by converting
   pixels into SVG elements.
3. **Photographic fidelity.**

Where you sacrifice fidelity to stay inside the subset, say so plainly in the
audit and describe the trade-off. An honest note about an approximated texture
is worth far more than a construct that silently renders black.

Present the transparent individual SVGs, the combined layout, the previews, the
source generator, and the audit as downloadable files.
