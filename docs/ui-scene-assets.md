# Registered editor scene assets

The current editor is 840×700 logical pixels. `assets/realism-v6/manifest.json`
registers a 1680×1400 top-left-origin canvas, a base image, five 65-frame rotary
atlases, a 17-frame pressure atlas and three discrete control groups. Their
state sources map to host parameters, the processor clip flag and pressed
LOAD/SAVE/Advanced buttons. Parameters stay continuous; only art uses frames.

`source/ui/SendBloomSceneArt.h` loads the embedded manifest and validates canvas
and atlas dimensions, frame counts, layer containment, non-overlapping crops,
hitboxes and live-value regions. `PluginEditor` uses these regions for native
controls and value text. The pressure amount releases immediately; visual cap
travel and its fading display continue independently. Bypass attaches to the
existing host parameter. DSP and parameter identifiers are unchanged by this
editor update.

CMake lists the runtime assets explicitly. The auxiliary knob, switch, display
and button skins in `realism-v6` style Advanced settings. Two `realism-v2`
pressure-pad images remain embedded for the standalone pad's non-scene mode.
Other v2/v4 and clear-instruments design studies are ignored local inputs.
`resources/ui/` and `tools/render_ui.py` retain the earlier faceplate resources
and renderer; they do not regenerate the current scene.

The manifest records the source Blender filename and SHA-256, frozen projection
hash, rendering settings and measured compositing approximations. Its original
absolute source path is provenance, not a build dependency. The Blender source
and complete rendering pipeline are not part of this repository. The scene is
an approximation: omitted cross-layer reflections, including clip illumination,
are documented in `source_contract.documented_approximation`; this is not an
exact physical all-state lighting simulation.

Run the [local verification workflow](local-verification.md) and
`scripts/capture-ui-state-matrix.sh`. The matrix covers 23 states at 1x and 2x,
including opposing rotary values, Gate Pre/Post, pressure release and pressed
actions. Editor tests check accessibility, parameter attachments, control
registration, neutral palette, independent rotary motion and pressure release.
Screenshots and local validation do not establish installed DAW behavior,
hardware equivalence, signing/notarization or a public release.
