"""SendBloom faceplate renderer — the source of truth for resources/ui art.

Builds the whole pedal as one procedural Blender scene, lights it once, and
path-traces every UI asset with Cycles so all shadows, AO and reflections agree:

  pedal_background.png/.jpg   840x1560 plate with baked contact shadows
  knob_large_strip.png        65-frame vertical filmstrip, 168px frames, alpha
  knob_small_strip.png        65-frame vertical filmstrip, 132px frames, alpha
  gate_pre/post.png           two-state toggle overlays, registered, with shadow
  footswitch_up/down.png      two-state treadle overlays
  dark_off/on.png             two-state button overlays
  clip_off/on.png             two-state lens overlays

Run headless (first run compiles Metal kernels, ~3 min):

  /Applications/Blender.app/Contents/MacOS/Blender -b --factory-startup \
      -P tools/render_ui.py -- [all|background|knobs|states] [--preview]

--preview quarters the sample count for fast look-dev iterations.

Composition contract (mirrors source/ui/PedalFaceplatePaint.h facelayout):
  * Every moving part renders as a complete assembly over a shadow-catcher
    plate, so each state carries its own physically-correct cast shadow and
    the two states of a pair register exactly (same camera, same crop).
  * Knob contact shadows are baked into the background instead (they do not
    change as the knob turns, and the 84px frame has no room for them):
    during the background pass the knobs are camera-invisible shadow casters.
  * Filmstrip frame i covers slider proportion t = i/64: pointer angle
    216 + 288*t degrees clockwise from 12 o'clock — matching
    setRotaryParameters(1.2*pi, 2.8*pi). Frame 32 is exactly 12 o'clock.
  * No text the plugin draws is baked. "DARK MODE" and the brand/preset art
    are product artwork, not host-drawn text, and stay in the render.
"""

import math
import sys
import time
from pathlib import Path

import bpy
import numpy as np
from mathutils import Vector

# ----------------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------------

REPO = Path(__file__).resolve().parent.parent
OUT = REPO / "resources" / "ui"
TMP = REPO / "Builds" / "render_ui_tmp"

EDITOR_W, EDITOR_H = 420, 780
SCALE = 2                      # render at 2x for hi-DPI backing stores
SAMPLES = 384
PREVIEW_SAMPLES = 64
KNOB_FRAMES = 65               # odd, so frame 32 is a true centre detent
SWEEP_START_DEG = 216.0        # JUCE rotary: 1.2*pi cw from 12 o'clock
SWEEP_DEG = 288.0              # 1.2*pi .. 2.8*pi

# facelayout rects (x, y, w, h in editor px) — the layout contract.
PLATE = (38.0, 60.0, 345.0, 642.0)
PLATE_RADIUS = 19.0
LOGO = (85, 70, 250, 52)
PRESET_FIELD = (54, 129, 232, 42)
PRESET_LOAD = (294, 136, 30, 29)
PRESET_SAVE = (332, 136, 27, 29)
KNOB_LARGE_D = 84.0
KNOB_SMALL_D = 66.0
KNOBS_LARGE = [(96.0, 226.0), (210.0, 226.0), (324.0, 226.0)]   # centres
KNOBS_SMALL = [(119.0, 347.0), (301.0, 347.0)]
CLIP_CENTRE = (210.0, 344.0)   # kClipLens {194,328,32,32}
DARK_CENTRE = (121.0, 475.0)   # kDarkButton {84,438,74,74}
GATE_CENTRE = (300.0, 475.0)   # kGateSwitch {272,444,56,62}
FOOT_CENTRE = (210.0, 618.5)   # kFootswitch {138,546,144,145}
SCREW_INSET = 20.0
SCREW_SLOTS = [0.6, 2.2, 1.1, 2.9]  # radians, per corner: TL TR BL BR

# Overlay art rects (x, y, w, h in editor px). Each two-state pair is rendered
# into its rect with room for the state's own cast shadow; the same values are
# hardcoded as the draw rects in PedalFaceplatePaint.cpp. Keep them in sync.
ART_RECTS = {
    "gate": (262, 430, 76, 100),
    "foot": (126, 534, 168, 168),
    "dark": (78, 432, 86, 86),
    "clip": (186, 320, 48, 48),
}

# Materials are tuned so the render lands near the current UI's palette:
# plate ~#f0eee8, ink 0x161413, orange accent 0xe66c0b.
COL_PLATE = (0.790, 0.780, 0.715)
COL_CHASSIS = (0.030, 0.028, 0.024)
COL_BENCH = (0.012, 0.011, 0.010)
COL_KNOB = (0.018, 0.018, 0.021)
COL_POINTER = (0.700, 0.690, 0.640)
COL_RUBBER = (0.022, 0.022, 0.024)


def px(x, y, z=0.0):
    """Editor px (y down) -> Blender world (y up). 1 unit == 1 editor px."""
    return (x, -y, z)


# ----------------------------------------------------------------------------
# Scene plumbing
# ----------------------------------------------------------------------------

def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)

    import addon_utils
    addon_utils.enable("cycles", default_set=True, persistent=True)

    scn = bpy.context.scene
    scn.render.engine = "CYCLES"
    prefs = bpy.context.preferences.addons["cycles"].preferences
    prefs.compute_device_type = "METAL"
    prefs.get_devices()
    for device in prefs.devices:
        device.use = True
    scn.cycles.device = "GPU"

    scn.cycles.samples = SAMPLES
    scn.cycles.use_denoising = True
    scn.cycles.seed = 7
    scn.cycles.use_animated_seed = False

    scn.render.resolution_x = EDITOR_W * SCALE
    scn.render.resolution_y = EDITOR_H * SCALE
    scn.render.image_settings.file_format = "PNG"
    scn.render.image_settings.color_mode = "RGBA"
    scn.render.image_settings.color_depth = "8"
    scn.render.image_settings.compression = 90

    # UI art wants faithful texture colours, not filmic tone mapping.
    scn.view_settings.view_transform = "Standard"
    scn.view_settings.look = "None"
    scn.view_settings.exposure = 0.0

    return scn


def add_camera(scn):
    cam_data = bpy.data.cameras.new("cam")
    cam_data.type = "ORTHO"
    cam_data.sensor_fit = "VERTICAL"
    cam_data.ortho_scale = EDITOR_H
    cam_data.clip_start = 10
    cam_data.clip_end = 5000
    cam = bpy.data.objects.new("cam", cam_data)
    # Straight down: image +x = editor +x, image top = editor y = 0.
    cam.location = px(EDITOR_W / 2, EDITOR_H / 2, 1500)
    cam.rotation_euler = (0.0, 0.0, 0.0)
    scn.collection.objects.link(cam)
    scn.camera = cam


def add_lights(scn):
    """One rig for everything. Key: big cool softbox upper-left, matching
    lighting::toLight = (-0.55, -0.83); shadows fall lower-right. Fill: broad
    warm panel camera-right (the current background's warm spill). The world
    is a dim vertical gradient so gloss always has a room to reflect."""
    centre = Vector(px(EDITOR_W / 2, EDITOR_H / 2, 0))

    def aim(obj, target):
        d = target - obj.location
        obj.rotation_euler = d.to_track_quat("-Z", "Y").to_euler()

    to_light = Vector((-0.55, 0.83, 0.0)).normalized()  # blender y = -editor y

    # Key: a sun, so irradiance is uniform across the plate — the shared knob
    # filmstrip is then exactly right at all three positions of a row. The
    # 12 degree disc softens shadow edges to a believable studio penumbra.
    sun_data = bpy.data.lights.new("key_sun", type="SUN")
    sun_data.color = (0.93, 0.955, 1.0)
    sun_data.energy = 2.7
    sun_data.angle = math.radians(9.0)
    sun = bpy.data.objects.new("key_sun", sun_data)
    sun.location = centre + Vector((0, 0, 800))
    elev = math.radians(62.0)
    shine = -Vector((to_light.x * math.cos(elev), to_light.y * math.cos(elev),
                     math.sin(elev)))
    sun.rotation_euler = shine.to_track_quat("-Z", "Y").to_euler()
    scn.collection.objects.link(sun)

    # Soft pool: the same upper-left direction as the sun, but broad and dim.
    # It rounds off the shadows and is what the glossy caps reflect.
    key_data = bpy.data.lights.new("key_soft", type="AREA")
    key_data.shape = "SQUARE"
    key_data.size = 480
    key_data.color = (0.93, 0.955, 1.0)
    key_data.energy = 5.5e6
    key = bpy.data.objects.new("key_soft", key_data)
    key.location = centre + to_light * 900 + Vector((0, 0, 1700))
    aim(key, centre)
    scn.collection.objects.link(key)

    fill_data = bpy.data.lights.new("fill", type="AREA")
    fill_data.shape = "RECTANGLE"
    fill_data.size = 380
    fill_data.size_y = 1200
    fill_data.color = (1.0, 0.70, 0.52)
    fill_data.energy = 3.6e6
    fill = bpy.data.objects.new("fill", fill_data)
    fill.location = centre + Vector((760, 0, 420))
    aim(fill, centre)
    scn.collection.objects.link(fill)

    world = bpy.data.worlds.new("world")
    scn.world = world
    world.use_nodes = True
    nodes = world.node_tree.nodes
    links = world.node_tree.links
    nodes.clear()
    out = nodes.new("ShaderNodeOutputWorld")
    bg = nodes.new("ShaderNodeBackground")
    bg.inputs["Strength"].default_value = 0.42
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (0.055, 0.045, 0.038, 1)  # floor: warm dark
    ramp.color_ramp.elements[1].color = (0.16, 0.19, 0.24, 1)     # zenith: cool
    sep = nodes.new("ShaderNodeSeparateXYZ")
    coord = nodes.new("ShaderNodeTexCoord")
    map_ = nodes.new("ShaderNodeMapRange")
    map_.inputs["From Min"].default_value = -1.0
    map_.inputs["From Max"].default_value = 1.0
    links.new(coord.outputs["Generated"], sep.inputs["Vector"])
    links.new(sep.outputs["Z"], map_.inputs["Value"])
    links.new(map_.outputs["Result"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bg.inputs["Color"])
    links.new(bg.outputs["Background"], out.inputs["Surface"])


# ----------------------------------------------------------------------------
# Materials
# ----------------------------------------------------------------------------

def principled(name, base, rough, metallic=0.0, coat=0.0, coat_rough=0.15):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    bsdf = mat.node_tree.nodes["Principled BSDF"]
    bsdf.inputs["Base Color"].default_value = (*base, 1.0)
    bsdf.inputs["Roughness"].default_value = rough
    bsdf.inputs["Metallic"].default_value = metallic
    try:
        bsdf.inputs["Coat Weight"].default_value = coat
        bsdf.inputs["Coat Roughness"].default_value = coat_rough
    except KeyError:
        pass
    return mat


# The reference photo of the real enamel plate. A high-passed crop of it
# drives the plate's bump/roughness/albedo so the stipple is the actual
# product texture, not an approximation of one.
STIPPLE_SRC = REPO / "UI SENDBLOOM" / "PNG" / "back plate.png"
STIPPLE_TEX = OUT / "plate_stipple.png"


def prepare_stipple_texture():
    """Deterministically derive a tileable grayscale stipple map from the
    reference photo: sharp centre crop, FFT high-pass to strip the photo's
    lighting gradient, normalised to mean 0.5."""
    if STIPPLE_TEX.exists() or not STIPPLE_SRC.exists():
        return
    img = bpy.data.images.load(str(STIPPLE_SRC))
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    buf = np.zeros(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    lum = buf.reshape(h, w, 4)[..., :3].mean(axis=2)
    bpy.data.images.remove(img)

    size = 512
    crop = lum[h // 2 - size // 2: h // 2 + size // 2,
               w // 2 - size // 2: w // 2 + size // 2]

    # High-pass: divide by a heavily blurred copy (gaussian via FFT).
    fy = np.fft.fftfreq(size)[:, None]
    fx = np.fft.fftfreq(size)[None, :]
    sigma = 18.0
    gauss = np.exp(-2.0 * (np.pi ** 2) * (sigma ** 2) * (fx ** 2 + fy ** 2))
    low = np.real(np.fft.ifft2(np.fft.fft2(crop) * gauss))
    hp = crop / np.maximum(low, 1e-4)
    hp = (hp - hp.mean()) / max(hp.std(), 1e-6)          # zero mean, unit sigma
    hp = np.clip(hp * 0.16 + 0.5, 0.0, 1.0)              # mean 0.5, gentle range

    # Torus blend so the map tiles seamlessly: cross-fade with a half-rolled
    # copy, weighted toward the copy at the borders (whose borders were the
    # original's centre, hence continuous across the wrap).
    rolled = np.roll(np.roll(hp, size // 2, 0), size // 2, 1)
    ramp = np.minimum(np.arange(size), np.arange(size)[::-1]) / (size / 2 - 1)
    ramp = np.clip(ramp * 1.6, 0.0, 1.0)
    ramp = ramp * ramp * (3 - 2 * ramp)
    w2 = ramp[:, None] * ramp[None, :]
    hp = hp * w2 + rolled * (1.0 - w2)

    out = bpy.data.images.new("stipple", width=size, height=size, alpha=False)
    out.colorspace_settings.name = "Non-Color"
    px4 = np.repeat(hp[..., None], 4, axis=2)
    px4[..., 3] = 1.0
    out.pixels.foreach_set(px4.ravel().astype(np.float32))
    out.filepath_raw = str(STIPPLE_TEX)
    out.file_format = "PNG"
    out.save()
    bpy.data.images.remove(out)
    print(f"[render_ui] derived {STIPPLE_TEX.name} from reference photo")


def _nodes(mat):
    return mat.node_tree.nodes, mat.node_tree.links, mat.node_tree.nodes["Principled BSDF"]


def _mapped_tex(mat, path, scale, rotation=0.0):
    """Image texture on Generated coords with a Mapping scale (REPEAT tiling)."""
    nodes, links, _ = _nodes(mat)
    coord = nodes.new("ShaderNodeTexCoord")
    mapping = nodes.new("ShaderNodeMapping")
    mapping.inputs["Scale"].default_value = (*scale, 1.0)
    mapping.inputs["Rotation"].default_value = (0.0, 0.0, rotation)
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(str(path), check_existing=True)
    tex.image.colorspace_settings.name = "Non-Color"
    tex.extension = "REPEAT"
    links.new(coord.outputs["Generated"], mapping.inputs["Vector"])
    links.new(mapping.outputs["Vector"], tex.inputs["Vector"])
    return tex


def _noise(mat, scale, detail=3.0, stretch=None, rotation=0.0, w=0.0):
    """Object-space noise; `stretch` (sx, sy) elongates it into streaks."""
    nodes, links, _ = _nodes(mat)
    coord = nodes.new("ShaderNodeTexCoord")
    noise = nodes.new("ShaderNodeTexNoise")
    noise.noise_dimensions = "4D"
    noise.inputs["W"].default_value = w
    noise.inputs["Scale"].default_value = scale
    noise.inputs["Detail"].default_value = detail
    src = coord.outputs["Object"]
    if stretch is not None or rotation:
        mapping = nodes.new("ShaderNodeMapping")
        if stretch is not None:
            mapping.inputs["Scale"].default_value = (stretch[0], stretch[1], 1.0)
        mapping.inputs["Rotation"].default_value = (0.0, 0.0, rotation)
        links.new(src, mapping.inputs["Vector"])
        src = mapping.outputs["Vector"]
    links.new(src, noise.inputs["Vector"])
    return noise


def _ramp(mat, src_socket, start, end):
    """ColorRamp windowing a float signal into a sparse 0..1 mask."""
    nodes, links, _ = _nodes(mat)
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = start
    ramp.color_ramp.elements[1].position = end
    links.new(src_socket, ramp.inputs["Fac"])
    return ramp


def _math(mat, op, a, b=None, value=None):
    nodes, links, _ = _nodes(mat)
    node = nodes.new("ShaderNodeMath")
    node.operation = op
    node.use_clamp = False
    if hasattr(a, "node"):
        links.new(a, node.inputs[0])
    else:
        node.inputs[0].default_value = a
    if b is not None:
        if hasattr(b, "node"):
            links.new(b, node.inputs[1])
        else:
            node.inputs[1].default_value = b
    if value is not None:
        node.inputs[1].default_value = value
    return node


def chain_bumps(mat, sources):
    """Chain (height_socket, strength, distance) triples into the BSDF normal."""
    nodes, links, bsdf = _nodes(mat)
    prev_normal = None
    for height, strength, distance in sources:
        bump = nodes.new("ShaderNodeBump")
        bump.inputs["Strength"].default_value = strength
        bump.inputs["Distance"].default_value = distance
        links.new(height, bump.inputs["Height"])
        if prev_normal is not None:
            links.new(prev_normal, bump.inputs["Normal"])
        prev_normal = bump.outputs["Normal"]
    links.new(prev_normal, bsdf.inputs["Normal"])


def add_rough_variation(mat, base, spread, scale=0.06, extra=None):
    """The single biggest anti-CG lever: roughness that wanders across the
    surface (handling smudges, uneven finish) instead of one constant."""
    nodes, links, bsdf = _nodes(mat)
    noise = _noise(mat, scale, detail=4.0)
    span = _math(mat, "MULTIPLY", noise.outputs["Fac"], spread * 2.0)
    rough = _math(mat, "ADD", span.outputs["Value"], base - spread)
    out = rough.outputs["Value"]
    if extra is not None:  # e.g. shiny scratches cutting roughness down
        out = _math(mat, "SUBTRACT", out, extra).outputs["Value"]
    links.new(out, bsdf.inputs["Roughness"])
    return out


def scratch_mask(mat, rotation, w, density=(0.70, 0.75)):
    """Sparse thin streaks: heavily stretched noise windowed by a ramp."""
    noise = _noise(mat, 1.6, detail=3.0, stretch=(0.09, 4.2), rotation=rotation, w=w)
    ramp = _ramp(mat, noise.outputs["Fac"], density[0], density[1])
    return ramp


def add_ao_grime(mat, tint=(0.55, 0.50, 0.42), amount=0.5, distance=9.0):
    """Occlusion-driven dirt: crevices and part seams collect a warm grime.
    Multiplies base colour toward `tint` where AO is strong."""
    nodes, links, bsdf = _nodes(mat)
    existing = bsdf.inputs["Base Color"].links
    src = existing[0].from_socket if existing else None
    ao = nodes.new("ShaderNodeAmbientOcclusion")
    ao.inputs["Distance"].default_value = distance
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].position = 0.35
    ramp.color_ramp.elements[0].color = (*[c * (1.0 - amount) + amount * t
                                           for c, t in zip((1, 1, 1), tint)], 1.0)
    ramp.color_ramp.elements[1].position = 0.9
    ramp.color_ramp.elements[1].color = (1, 1, 1, 1)
    links.new(ao.outputs["AO"], ramp.inputs["Fac"])
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    mix.blend_type = "MULTIPLY"
    mix.inputs[0].default_value = 1.0
    if src is not None:
        links.new(src, mix.inputs[6])
    else:
        mix.inputs[6].default_value = bsdf.inputs["Base Color"].default_value
    links.new(ramp.outputs["Color"], mix.inputs[7])
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])


def image_material(name, path, rough=0.45, rough_spread=0.10, art_bump=0.10):
    """Slab material: art mapped across the object's bounding box, plus the
    imperfections that keep it from reading as backlit glass: roughness
    grunge, a whisper of orange-peel, and edge relief from the art itself
    (printed enamel sits a hair proud of its ground)."""
    mat = principled(name, (0.5, 0.5, 0.5), rough)
    nodes, links, bsdf = _nodes(mat)
    tex = nodes.new("ShaderNodeTexImage")
    tex.image = bpy.data.images.load(str(path), check_existing=True)
    tex.extension = "EXTEND"
    coord = nodes.new("ShaderNodeTexCoord")
    links.new(coord.outputs["Generated"], tex.inputs["Vector"])
    mix = nodes.new("ShaderNodeMix")
    mix.data_type = "RGBA"
    # Mix sockets by index: 0 Factor, 6 A (colour), 7 B (colour), output 2.
    links.new(tex.outputs["Alpha"], mix.inputs[0])
    mix.inputs[6].default_value = (0.01, 0.01, 0.01, 1.0)  # alpha = 0
    links.new(tex.outputs["Color"], mix.inputs[7])         # alpha = 1
    links.new(mix.outputs[2], bsdf.inputs["Base Color"])

    add_rough_variation(mat, rough, rough_spread, scale=0.08)
    art_bw = nodes.new("ShaderNodeRGBToBW")
    links.new(tex.outputs["Color"], art_bw.inputs["Color"])
    peel = _noise(mat, 0.9, detail=3.0)
    chain_bumps(mat, [
        (art_bw.outputs["Val"], art_bump, 0.5),   # print edge relief
        (peel.outputs["Fac"], 0.10, 0.35),        # painted-metal orange peel
    ])
    add_ao_grime(mat, amount=0.35, distance=6.0)
    return mat


def make_plate_material():
    """Cream stippled enamel built from the reference photo: the stipple works
    the bump, the roughness AND the albedo; sparse glossy scratches cross it;
    grime gathers where geometry meets the plate; a large-scale waviness
    breaks the sheen so the panel never reads as a perfect plane."""
    mat = principled("plate", COL_PLATE, 0.42)
    nodes, links, bsdf = _nodes(mat)

    # -- stipple, from the product photo (tile ~215 px so grain lands ~2 px)
    tile = (PLATE[2] / 400.0, PLATE[3] / 400.0)
    stip = _mapped_tex(mat, STIPPLE_TEX, tile) if STIPPLE_TEX.exists() else None

    # -- scratches: two directions, very sparse, shiny-bottomed
    s1 = scratch_mask(mat, rotation=0.42, w=3.0)
    s2 = scratch_mask(mat, rotation=-1.05, w=11.0, density=(0.715, 0.76))
    scr = _math(mat, "MAXIMUM", s1.outputs["Color"], s2.outputs["Color"])

    # -- roughness: enamel base +- smudge grunge, scratches cut it glossy
    scr_cut = _math(mat, "MULTIPLY", scr.outputs["Value"], 0.16)
    rough_out = add_rough_variation(mat, 0.42, 0.09, scale=0.045,
                                    extra=scr_cut.outputs["Value"])
    if stip is not None:  # matte flecks: stipple peaks read slightly drier
        stip_r = _math(mat, "MULTIPLY_ADD", stip.outputs["Color"], 0.10)
        stip_r.inputs[2].default_value = -0.05
        final = _math(mat, "ADD", rough_out, stip_r.outputs["Value"])
        links.new(final.outputs["Value"], bsdf.inputs["Roughness"])

    # -- albedo: cream, mottled a few percent, flecked by the stipple
    mottle = _noise(mat, 0.02, detail=3.0)
    mot_span = _math(mat, "MULTIPLY_ADD", mottle.outputs["Fac"], 0.11)
    mot_span.inputs[2].default_value = 0.945  # 0.965 .. 1.035
    base_rgb = nodes.new("ShaderNodeMix")
    base_rgb.data_type = "RGBA"
    base_rgb.blend_type = "MULTIPLY"
    base_rgb.inputs[0].default_value = 1.0
    base_rgb.inputs[6].default_value = (*COL_PLATE, 1.0)
    links.new(mot_span.outputs["Value"], base_rgb.inputs[7])
    if stip is not None:
        fleck = _math(mat, "MULTIPLY_ADD", stip.outputs["Color"], 0.16)
        fleck.inputs[2].default_value = 0.92
        fleck_mix = nodes.new("ShaderNodeMix")
        fleck_mix.data_type = "RGBA"
        fleck_mix.blend_type = "MULTIPLY"
        fleck_mix.inputs[0].default_value = 1.0
        links.new(base_rgb.outputs[2], fleck_mix.inputs[6])
        links.new(fleck.outputs["Value"], fleck_mix.inputs[7])
        # Scratches shade the enamel slightly grey (scuffed paint reads by its
        # tone at UI size, not by its groove) before feeding the BSDF.
        scr_tone = _math(mat, "MULTIPLY_ADD", scr.outputs["Value"], -0.14)
        scr_tone.inputs[2].default_value = 1.0
        scr_mix = nodes.new("ShaderNodeMix")
        scr_mix.data_type = "RGBA"
        scr_mix.blend_type = "MULTIPLY"
        scr_mix.inputs[0].default_value = 1.0
        links.new(fleck_mix.outputs[2], scr_mix.inputs[6])
        links.new(scr_tone.outputs["Value"], scr_mix.inputs[7])
        links.new(scr_mix.outputs[2], bsdf.inputs["Base Color"])
    else:
        links.new(base_rgb.outputs[2], bsdf.inputs["Base Color"])

    # -- normal: waviness (stamped panel) -> stipple -> scratch grooves
    wavy = _noise(mat, 0.004, detail=2.0)
    bumps = [(wavy.outputs["Fac"], 0.22, 2.4)]
    if stip is not None:
        bumps.append((stip.outputs["Color"], 1.0, 1.3))
    scr_h = _math(mat, "MULTIPLY", scr.outputs["Value"], -1.0)
    bumps.append((scr_h.outputs["Value"], 0.35, 0.5))
    chain_bumps(mat, bumps)

    add_ao_grime(mat, amount=0.5, distance=12.0)
    return mat


class Mats:
    def __init__(self):
        prepare_stipple_texture()
        self.plate = make_plate_material()
        self.chassis = principled("chassis", COL_CHASSIS, 0.42, metallic=0.85)
        add_rough_variation(self.chassis, 0.42, 0.12, scale=0.05)
        cgrain = _noise(self.chassis, 0.5, detail=4.0)
        chain_bumps(self.chassis, [(cgrain.outputs["Fac"], 0.18, 0.5)])
        self.bench = principled("bench", COL_BENCH, 0.85)
        self.knob = principled("knob", COL_KNOB, 0.30, coat=0.32, coat_rough=0.14)
        add_rough_variation(self.knob, 0.30, 0.08, scale=0.16)
        kgrain = _noise(self.knob, 0.85, detail=3.0)
        chain_bumps(self.knob, [(kgrain.outputs["Fac"], 0.09, 0.3)])
        self.pointer = principled("pointer", COL_POINTER, 0.32, metallic=0.7)
        add_rough_variation(self.pointer, 0.32, 0.10, scale=0.3)
        self.chrome = principled("chrome", (0.85, 0.86, 0.88), 0.14, metallic=1.0)
        add_rough_variation(self.chrome, 0.16, 0.08, scale=0.25)
        self.steel = principled("steel", (0.62, 0.62, 0.63), 0.34, metallic=1.0)
        add_rough_variation(self.steel, 0.34, 0.12, scale=0.3)
        self.nickel = principled("nickel", (0.72, 0.71, 0.68), 0.28, metallic=1.0)
        add_rough_variation(self.nickel, 0.28, 0.10, scale=0.4)
        self.rubber = principled("rubber", COL_RUBBER, 0.62)
        add_rough_variation(self.rubber, 0.60, 0.10, scale=0.08)
        rgrain = _noise(self.rubber, 0.38, detail=4.0)
        rwear = _noise(self.rubber, 0.03, detail=2.0)
        chain_bumps(self.rubber, [(rgrain.outputs["Fac"], 0.22, 0.5),
                                  (rwear.outputs["Fac"], 0.15, 1.2)])
        self.button_black = principled("button_black", (0.016, 0.016, 0.017), 0.46)
        add_rough_variation(self.button_black, 0.44, 0.10, scale=0.1)
        bgrain = _noise(self.button_black, 0.55, detail=4.0)
        chain_bumps(self.button_black, [(bgrain.outputs["Fac"], 0.12, 0.4)])
        self.button_text = principled("button_text", (0.006, 0.006, 0.006), 0.62)
        self.slot = principled("slot", (0.05, 0.05, 0.05), 0.5, metallic=0.6)
        self.lens_off = principled("lens_off", (0.045, 0.006, 0.005), 0.08, coat=1.0, coat_rough=0.05)
        self.lens_on = self._lens_on()
        self.logo = image_material("logo", OUT / "brand_logo.png",
                                   rough=0.36, rough_spread=0.12, art_bump=0.12)
        self.preset = image_material("preset", OUT / "preset_field.png",
                                     rough=0.55, rough_spread=0.08, art_bump=0.08)
        self.load = image_material("load", OUT / "preset_load.png", rough=0.5)
        self.save = image_material("save", OUT / "preset_save.png", rough=0.5)

    @staticmethod
    def _lens_on():
        mat = principled("lens_on", (0.9, 0.12, 0.05), 0.08, coat=1.0, coat_rough=0.05)
        bsdf = mat.node_tree.nodes["Principled BSDF"]
        bsdf.inputs["Emission Color"].default_value = (1.0, 0.16, 0.05, 1.0)
        bsdf.inputs["Emission Strength"].default_value = 14.0
        return mat


# ----------------------------------------------------------------------------
# Geometry helpers
# ----------------------------------------------------------------------------

import bmesh


def link_object(name, mesh, mat=None):
    obj = bpy.data.objects.new(name, mesh)
    if mat is not None:
        obj.data.materials.append(mat)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def shade_smooth(obj, angle=math.radians(40)):
    for poly in obj.data.polygons:
        poly.use_smooth = True
    try:
        with bpy.context.temp_override(object=obj, selected_objects=[obj], active_object=obj):
            bpy.ops.object.shade_auto_smooth(angle=angle)
    except Exception:
        pass


def rounded_rect_outline(w, h, r, seg=10):
    """CCW outline of a w x h rounded rect centred on origin."""
    hw, hh = w / 2, h / 2
    r = min(r, hw, hh)
    pts = []
    corners = [
        (hw - r, hh - r, 0.0),           # top-right
        (-(hw - r), hh - r, 90.0),       # top-left
        (-(hw - r), -(hh - r), 180.0),   # bottom-left
        (hw - r, -(hh - r), 270.0),      # bottom-right
    ]
    for cx, cy, start in corners:
        for i in range(seg + 1):
            a = math.radians(start + 90.0 * i / seg)
            pts.append((cx + r * math.cos(a), cy + r * math.sin(a)))
    # Deduplicate closing points
    dedup = []
    for p in pts:
        if not dedup or (abs(p[0] - dedup[-1][0]) + abs(p[1] - dedup[-1][1])) > 1e-6:
            dedup.append(p)
    if (abs(dedup[0][0] - dedup[-1][0]) + abs(dedup[0][1] - dedup[-1][1])) < 1e-6:
        dedup.pop()
    return dedup


def make_prism(name, mat, w, h, r, z0, z1, bevel=1.2, seg=10, centre=(0.0, 0.0),
               well=None):
    """Rounded-rect prism from z0 to z1 with a bevelled top edge. `well`
    optionally insets the top face by (thickness, depth) to form a tray."""
    bm = bmesh.new()
    outline = rounded_rect_outline(w, h, r, seg)
    verts = [bm.verts.new((x, y, z0)) for x, y in outline]
    base = bm.faces.new(verts)
    ret = bmesh.ops.extrude_face_region(bm, geom=[base])
    top_verts = [g for g in ret["geom"] if isinstance(g, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, verts=top_verts, vec=(0, 0, z1 - z0))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    if bevel > 0:
        top_edges = [e for e in bm.edges
                     if all(abs(v.co.z - z1) < 1e-5 for v in e.verts)]
        bmesh.ops.bevel(bm, geom=top_edges, offset=bevel, segments=3,
                        profile=0.72, affect="EDGES")

    if well is not None:
        thickness, depth = well
        top_face = max((f for f in bm.faces
                        if all(abs(v.co.z - z1) < 1e-4 for v in f.verts)),
                       key=lambda f: f.calc_area())
        ret = bmesh.ops.inset_region(bm, faces=[top_face], thickness=thickness,
                                     use_even_offset=True)
        bmesh.ops.translate(bm, verts=list(top_face.verts), vec=(0, 0, -depth))
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = link_object(name, mesh, mat)
    obj.location = (centre[0], centre[1], 0)
    shade_smooth(obj)
    return obj


def make_lathe(name, mat, profile, centre, flutes=0, flute_depth=0.0,
               flute_stations=None, steps=192):
    """Revolve a (z, radius) profile about +Z at `centre`. Stations listed in
    `flute_stations` (by index) get a cosine scallop of `flute_depth`."""
    flute_stations = flute_stations or set()
    bm = bmesh.new()
    rings = []
    for si, (z, r) in enumerate(profile):
        scallop = flute_depth if si in flute_stations else 0.0
        if r <= 1e-5:
            rings.append([bm.verts.new((0.0, 0.0, z))])
            continue
        ring = []
        for ti in range(steps):
            a = 2 * math.pi * ti / steps
            rr = r - scallop * (0.5 + 0.5 * math.cos(flutes * a)) if flutes else r
            ring.append(bm.verts.new((rr * math.cos(a), rr * math.sin(a), z)))
        rings.append(ring)

    for lower, upper in zip(rings, rings[1:]):
        if len(lower) == 1:
            for ti in range(steps):
                bm.faces.new((lower[0], upper[ti], upper[(ti + 1) % steps]))
        elif len(upper) == 1:
            for ti in range(steps):
                bm.faces.new((lower[ti], lower[(ti + 1) % steps], upper[0]))
        else:
            for ti in range(steps):
                tj = (ti + 1) % steps
                bm.faces.new((lower[ti], lower[tj], upper[tj], upper[ti]))
    if len(rings[0]) > 1:  # close the bottom
        bm.faces.new(list(reversed(rings[0])))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)

    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = link_object(name, mesh, mat)
    obj.location = centre
    shade_smooth(obj, angle=math.radians(46))
    return obj


def make_cylinder(name, mat, r, z0, z1, centre, verts=96, smooth=True):
    bm = bmesh.new()
    bmesh.ops.create_cone(bm, cap_ends=True, segments=verts,
                          radius1=r, radius2=r, depth=(z1 - z0))
    bmesh.ops.translate(bm, verts=bm.verts[:], vec=(0, 0, (z0 + z1) / 2))
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = link_object(name, mesh, mat)
    obj.location = centre
    if smooth:
        shade_smooth(obj)
    return obj


def make_sphere(name, mat, r, centre, squash=1.0):
    bm = bmesh.new()
    bmesh.ops.create_uvsphere(bm, u_segments=48, v_segments=32, radius=r)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = link_object(name, mesh, mat)
    obj.location = centre
    obj.scale = (1.0, 1.0, squash)
    shade_smooth(obj)
    return obj


def make_box(name, mat, size, centre, rot=(0, 0, 0)):
    bm = bmesh.new()
    bmesh.ops.create_cube(bm, size=1.0)
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    obj = link_object(name, mesh, mat)
    obj.scale = size
    obj.location = centre
    obj.rotation_euler = rot
    return obj


def load_font():
    for candidate in ("/System/Library/Fonts/Supplemental/Arial Bold.ttf",
                      "/System/Library/Fonts/Supplemental/Verdana Bold.ttf",
                      "/System/Library/Fonts/Helvetica.ttc"):
        try:
            return bpy.data.fonts.load(candidate)
        except Exception:
            continue
    return None


def make_text(name, mat, body, centre, size, extrude=0.6):
    curve = bpy.data.curves.new(name, type="FONT")
    curve.body = body
    curve.size = size
    curve.align_x = "CENTER"
    curve.align_y = "CENTER"
    curve.extrude = extrude
    font = load_font()
    if font is not None:
        curve.font = font
    obj = bpy.data.objects.new(name, curve)
    obj.data.materials.append(mat)
    obj.location = centre
    bpy.context.scene.collection.objects.link(obj)
    return obj


# ----------------------------------------------------------------------------
# The pedal
# ----------------------------------------------------------------------------

class Pedal:
    """Builds every object once and keeps the moving assemblies addressable so
    render passes can toggle camera visibility / hide entire states."""

    def __init__(self, mats):
        self.m = mats
        self.static = []      # always in the background render
        self.knobs = {}       # 'large'/'small' -> [knob objects at rest]
        self.proxies = []     # smooth knob stand-ins for the background bake
        self.assemblies = {}  # 'gate_pre', 'dark_on', ... -> [objects]
        self._build()

    # -- static -------------------------------------------------------------

    def _build(self):
        m = self.m
        x, y, w, h = PLATE

        bench = make_box("bench", m.bench, (2400, 4000, 8),
                         px(EDITOR_W / 2, EDITOR_H / 2, -22))
        chassis = make_prism("chassis", m.chassis, w + 34, h + 34, PLATE_RADIUS + 12,
                             -20, -3.5, bevel=3.0,
                             centre=px(x + w / 2, y + h / 2)[:2])
        plate = make_prism("plate", m.plate, w, h, PLATE_RADIUS,
                           -8, 0, bevel=2.2,
                           centre=px(x + w / 2, y + h / 2)[:2])
        self.plate = plate
        self.static += [bench, chassis, plate]

        self._build_badge_and_preset()
        self._build_screws()
        self._build_knobs()
        self._build_gate()
        self._build_dark()
        self._build_foot()
        self._build_clip()

    def _build_badge_and_preset(self):
        m = self.m
        lx, ly, lw, lh = LOGO
        badge = make_prism("badge", m.logo, lw, lh, 9, 0, 4.5, bevel=1.4,
                           centre=px(lx + lw / 2, ly + lh / 2)[:2])
        px_, py_, pw, ph = PRESET_FIELD
        field = make_prism("preset_field", m.preset, pw, ph, 6, 0, 2.0, bevel=0.9,
                           centre=px(px_ + pw / 2, py_ + ph / 2)[:2])
        bx, by, bw, bh = PRESET_LOAD
        load = make_prism("preset_load", m.load, bw, bh, 6, 0, 3.0, bevel=1.0,
                          centre=px(bx + bw / 2, by + bh / 2)[:2])
        bx, by, bw, bh = PRESET_SAVE
        save = make_prism("preset_save", m.save, bw, bh, 6, 0, 3.0, bevel=1.0,
                          centre=px(bx + bw / 2, by + bh / 2)[:2])
        self.static += [badge, field, load, save]

    def _build_screws(self):
        m = self.m
        x, y, w, h = PLATE
        corners = [(x + SCREW_INSET, y + SCREW_INSET),
                   (x + w - SCREW_INSET, y + SCREW_INSET),
                   (x + SCREW_INSET, y + h - SCREW_INSET),
                   (x + w - SCREW_INSET, y + h - SCREW_INSET)]
        for i, ((sx, sy), slot) in enumerate(zip(corners, SCREW_SLOTS)):
            centre = px(sx, sy)
            bore = make_cylinder(f"screw_bore_{i}", m.chassis, 9.0, -1.6, 0.15, centre)
            head = make_sphere(f"screw_head_{i}", m.nickel, 7.0,
                               px(sx, sy, -1.2), squash=0.55)
            bar = make_box(f"screw_slot_{i}", m.slot, (11.5, 1.7, 1.2),
                           px(sx, sy, 2.55), rot=(0, 0, -slot))
            self.static += [bore, head, bar]

    # -- knobs ----------------------------------------------------------------

    def _knob_profile(self, R, H):
        """(z, radius) stations bottom to top; returns (profile, flute set)."""
        profile = [
            (0.0, R * 0.965),
            (1.5, R),           # skirt flare
            (H * 0.40, R * 0.985),
            (H * 0.52, R * 0.93),   # last fluted station
            (H * 0.60, R * 0.80),   # shoulder into the cap
            (H * 0.78, R * 0.745),
            (H * 0.92, R * 0.70),
            (H * 0.985, R * 0.52),  # rounded top corner
            (H * 1.02, R * 0.30),
            (H * 1.04, 0.0),        # slightly domed top
        ]
        flute_stations = {0, 1, 2, 3}
        return profile, flute_stations

    def _make_knob(self, name, centre_px, diameter, flutes):
        m = self.m
        R = diameter / 2 - 1.0          # 1px air inside the layout square
        H = diameter * 0.34             # squat: this is viewed top-down
        profile, flute_st = self._knob_profile(R, H)
        body = make_lathe(name, m.knob, profile, px(*centre_px),
                          flutes=flutes, flute_depth=R * 0.055,
                          flute_stations=flute_st)
        # Background-pass occluder: same knob without flutes, at the flute-dip
        # radius. The baked contact darkening must stay INSIDE the overlay's
        # silhouette at every rotation; a fluted occluder's scallop tips would
        # peek out from under the filmstrip at other angles.
        # 0.6px inside the flute-dip radius: the baked dark core must never
        # peek past the strip's silhouette, even by an antialiased subpixel.
        dip = [(z, max(0.0, r - R * 0.055 - 0.6)) for z, r in profile]
        proxy = make_lathe(name + "_proxy", m.knob, dip, px(*centre_px))
        proxy.hide_render = True
        self.proxies.append(proxy)
        # Pointer: a light inlaid bar on the cap, pointing at 12 o'clock
        # (blender +y). It parents to the body so filmstrip rotation carries it.
        bar = make_box(name + "_ptr", m.pointer,
                       (R * 0.115, R * 0.52, 2.4),
                       (0.0, R * 0.42, H * 0.955))
        bar.parent = body
        return [body, bar]

    def _build_knobs(self):
        self.knobs["large"] = []
        self.knobs["small"] = []
        for i, c in enumerate(KNOBS_LARGE):
            self.knobs["large"].append(
                self._make_knob(f"knob_large_{i}", c, KNOB_LARGE_D, 18))
        for i, c in enumerate(KNOBS_SMALL):
            self.knobs["small"].append(
                self._make_knob(f"knob_small_{i}", c, KNOB_SMALL_D, 16))
        # Rest pose: value 0 -> pointer at SWEEP_START_DEG clockwise from 12.
        for pair in self.knobs["large"] + self.knobs["small"]:
            pair[0].rotation_euler.z = -math.radians(SWEEP_START_DEG)

    # -- gate toggle ----------------------------------------------------------

    def _build_gate(self):
        m = self.m
        cx, cy = GATE_CENTRE
        base = px(cx, cy)
        shared = []
        shared.append(make_cylinder("gate_washer", m.steel, 21.0, 0, 2.0, base))
        nut = make_cylinder("gate_nut", m.nickel, 19.0, 2.0, 9.5, base, verts=6,
                            smooth=False)
        shared.append(nut)
        shared.append(make_cylinder("gate_collar", m.steel, 10.5, 9.5, 14.5, base))
        shared.append(make_sphere("gate_pivot", m.chrome, 8.0, px(cx, cy, 14.0),
                                  squash=0.8))

        def lever(state, tilt_deg):
            objs = []
            L = 30.0
            bm = bmesh.new()
            bmesh.ops.create_cone(bm, cap_ends=True, segments=64,
                                  radius1=5.4, radius2=4.0, depth=L)
            bmesh.ops.translate(bm, verts=bm.verts[:], vec=(0, 0, L / 2 + 6.0))
            mesh = bpy.data.meshes.new(f"gate_lever_{state}")
            bm.to_mesh(mesh)
            bm.free()
            rod = link_object(f"gate_lever_{state}", mesh, m.chrome)
            rod.location = px(cx, cy, 12.0)
            tilt = math.radians(tilt_deg)
            rod.rotation_euler = (tilt, 0, 0)
            shade_smooth(rod)
            objs.append(rod)
            # Tip of the local +Z axis after the X tilt, in world space.
            lz = L + 6.0
            tip = Vector(px(cx, cy, 12.0)) + Vector((0.0, -lz * math.sin(tilt),
                                                     lz * math.cos(tilt)))
            objs.append(make_sphere(f"gate_tip_{state}", m.chrome, 4.6, tuple(tip)))
            return objs

        # PRE = lever up (screen up = blender +y => negative x-rotation)
        self.assemblies["gate_pre"] = shared + lever("pre", -46.0)
        self.assemblies["gate_post"] = shared + lever("post", 46.0)

    # -- dark-mode button -------------------------------------------------------

    def _build_dark(self):
        m = self.m
        cx, cy = DARK_CENTRE
        housing = make_prism("dark_housing", m.button_black, 68, 68, 14, 0, 6.5,
                             bevel=1.6, centre=px(cx, cy)[:2],
                             well=(4.5, 4.5))

        def cap(state, size, z0, z1, text_z):
            capobj = make_prism(f"dark_cap_{state}", m.button_black, size, size,
                                11, z0, z1, bevel=2.6, centre=px(cx, cy)[:2])
            text = make_text(f"dark_text_{state}", m.button_text, "DARK\nMODE",
                             px(cx, cy + 1.0, text_z), size=15.5, extrude=0.7)
            return [capobj, text]

        self.assemblies["dark_off"] = [housing] + cap("off", 60, 3.0, 19.0, 18.6)
        self.assemblies["dark_on"] = [housing] + cap("on", 54, 1.5, 8.5, 8.1)

    # -- footswitch --------------------------------------------------------------

    def _build_foot(self):
        m = self.m
        cx, cy = FOOT_CENTRE
        base = make_prism("foot_base", m.chassis, 108, 148, 12, 0, 5.0,
                          bevel=2.0, centre=px(cx, cy)[:2])
        hinge_y_editor = cy + 66.0  # screen-bottom edge of the treadle

        def treadle(state, lift_deg):
            pad = make_prism(f"foot_pad_{state}", m.rubber, 92, 130, 10,
                             0, 12.0, bevel=3.4, centre=(0.0, 0.0))
            # Shift the pad in local space so its hinge edge sits on the object
            # origin: the pad extends +y (screen-up) from the hinge.
            for v in pad.data.vertices:
                v.co.y += 65.0
            # Pivot at the screen-bottom edge; positive x-rotation lifts the
            # far (screen-top, local +y) edge toward the key light.
            pad.location = px(cx, hinge_y_editor, 5.0)
            pad.rotation_euler = (math.radians(lift_deg), 0, 0)
            return [pad]

        self.assemblies["foot_up"] = [base] + treadle("up", 10.0)
        self.assemblies["foot_down"] = [base] + treadle("down", 3.0)

    # -- clip lens -----------------------------------------------------------------

    def _build_clip(self):
        m = self.m
        cx, cy = CLIP_CENTRE
        bezel = make_prism("clip_bezel", m.button_black, 31, 31, 7, 0, 4.0,
                           bevel=1.4, centre=px(cx, cy)[:2])
        dome_off = make_sphere("clip_dome_off", m.lens_off, 11.0,
                               px(cx, cy, 2.0), squash=0.55)
        dome_on = make_sphere("clip_dome_on", m.lens_on, 11.0,
                              px(cx, cy, 2.0), squash=0.55)
        self.assemblies["clip_off"] = [bezel, dome_off]
        self.assemblies["clip_on"] = [bezel, dome_on]

    # -- visibility helpers ------------------------------------------------------

    def all_moving(self):
        objs = []
        for pair in self.knobs["large"] + self.knobs["small"]:
            objs += pair
        for group in self.assemblies.values():
            objs += group
        return objs

    def knob_objects(self):
        objs = []
        for pair in self.knobs["large"] + self.knobs["small"]:
            objs += pair
        return objs


# ----------------------------------------------------------------------------
# Render passes
# ----------------------------------------------------------------------------

def set_border(scn, rect_px):
    x, y, w, h = rect_px
    rx, ry = scn.render.resolution_x, scn.render.resolution_y
    scn.render.use_border = True
    scn.render.use_crop_to_border = True
    scn.render.border_min_x = (x * SCALE) / rx
    scn.render.border_max_x = ((x + w) * SCALE) / rx
    scn.render.border_max_y = 1.0 - (y * SCALE) / ry
    scn.render.border_min_y = 1.0 - ((y + h) * SCALE) / ry


def clear_border(scn):
    scn.render.use_border = False
    scn.render.use_crop_to_border = False


def render_to(scn, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    scn.render.filepath = str(path)
    t0 = time.time()
    bpy.ops.render.render(write_still=True)
    dt = time.time() - t0
    size = path.stat().st_size
    print(f"[render_ui] {path.name}: {dt:.1f}s, {size / 1024:.0f} KB")
    return dt


def show_only_states(pedal, active_keys):
    """hide_render for every moving assembly except the named ones. Assemblies
    share their fixed parts (nut, housing, base...), so visibility is computed
    across all groups first — a shared object stays if ANY active state has it."""
    visible = set()
    for key in active_keys:
        visible.update(obj.name for obj in pedal.assemblies[key])
    for group in pedal.assemblies.values():
        for obj in group:
            obj.hide_render = obj.name not in visible


def restore_all(pedal):
    for obj in pedal.all_moving():
        obj.hide_render = False
        obj.visible_camera = True
    for proxy in pedal.proxies:
        proxy.hide_render = True
    pedal.plate.is_shadow_catcher = False


DEFAULT_STATES = {"gate_pre", "dark_off", "foot_up", "clip_off"}


def pass_background(scn, pedal):
    """Bare plate + static decor. Knobs are camera-invisible shadow casters;
    the state assemblies are absent entirely (their shadows ship with the
    overlays)."""
    restore_all(pedal)
    show_only_states(pedal, set())          # no state assemblies at all
    for obj in pedal.knob_objects():
        obj.hide_render = True              # fluted heroes out entirely...
    for proxy in pedal.proxies:
        proxy.hide_render = False           # ...smooth proxies cast instead
        proxy.visible_camera = False        # shadows + AO only
    scn.render.film_transparent = False
    clear_border(scn)
    render_to(scn, OUT / "pedal_background.png")
    post_background(OUT / "pedal_background.png", OUT / "pedal_background.jpg")


def post_background(png_path, jpg_path, sigma=2.6 / 255.0):
    """Photographic grade over the background render: film grain and a gentle
    vignette. Both are authored at 2-px correlation / large radius so they
    SURVIVE the 2x -> 1x downscale the editor applies — per-pixel effects
    average out to nothing at display size."""
    img = bpy.data.images.load(str(png_path))
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    buf = np.zeros(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    buf = buf.reshape(h, w, 4)

    # 2-px correlated grain: generated at 1x and doubled, so downscaling
    # returns it to exactly per-pixel grain instead of cancelling it.
    rng = np.random.default_rng(7)
    grain = rng.normal(0.0, sigma, size=((h + 1) // 2, (w + 1) // 2, 1))
    grain = np.repeat(np.repeat(grain, 2, axis=0), 2, axis=1)[:h, :w]
    buf[..., :3] = np.clip(buf[..., :3] + grain.astype(np.float32), 0.0, 1.0)

    # Photographic vignette, centred on the plate's upper third where the key
    # pools: ~2% at the plate corners, ~6% at the frame corners.
    yy, xx = np.mgrid[0:h, 0:w]
    cx, cy = w / 2.0, h * 0.62            # buffer rows run bottom-up
    r = np.sqrt(((xx - cx) / (w * 0.75)) ** 2 + ((yy - cy) / (h * 0.72)) ** 2)
    fall = np.clip((r - 0.42) / 0.65, 0.0, 1.0)
    fall = fall * fall * (3 - 2 * fall)
    buf[..., :3] *= (1.0 - 0.13 * fall)[..., None].astype(np.float32)
    bpy.data.images.remove(img)

    out = bpy.data.images.new("bg_post", width=w, height=h, alpha=True)
    out.colorspace_settings.name = "Non-Color"
    out.pixels.foreach_set(buf.ravel())
    out.filepath_raw = str(png_path)
    out.file_format = "PNG"
    out.save()
    out.file_format = "JPEG"
    try:
        out.save(filepath=str(jpg_path), quality=92)
    except TypeError:  # older Image.save() without kwargs
        out.filepath_raw = str(jpg_path)
        out.save()
    bpy.data.images.remove(out)
    print(f"[render_ui] pedal_background.jpg: "
          f"{jpg_path.stat().st_size / 1024:.0f} KB (grained)")


def pass_knob_strips(scn, pedal):
    """One knob of each size, rendered alone with alpha, one frame per pointer
    angle, stitched into a vertical strip (frame 0 at the top)."""
    for size_key, diameter, flutes in (("large", KNOB_LARGE_D, 18),
                                       ("small", KNOB_SMALL_D, 16)):
        restore_all(pedal)
        show_only_states(pedal, set())
        body, bar = pedal.knobs[size_key][0]

        # Park the hero knob at the plate's horizontal centre at its row's
        # height so the fill gradient it bakes is the row average.
        original = tuple(body.location)
        row_y = body.location.y
        body.location = (EDITOR_W / 2, row_y, 0.0)

        for obj in bpy.context.scene.objects:
            if obj.type in {"MESH", "CURVE", "FONT"}:
                obj.visible_camera = obj in (body, bar)
        for other_pair in pedal.knobs["large"] + pedal.knobs["small"]:
            for obj in other_pair:
                if obj not in (body, bar):
                    obj.hide_render = True

        scn.render.film_transparent = True
        frame_px = int(diameter)
        rect = (EDITOR_W / 2 - diameter / 2, -row_y - diameter / 2,
                diameter, diameter)
        set_border(scn, rect)

        frame_paths = []
        for i in range(KNOB_FRAMES):
            t = i / (KNOB_FRAMES - 1)
            angle = SWEEP_START_DEG + SWEEP_DEG * t
            body.rotation_euler.z = -math.radians(angle)
            path = TMP / f"{size_key}_{i:03d}.png"
            render_to(scn, path)
            frame_paths.append(path)

        stitch_strip(frame_paths, OUT / f"knob_{size_key}_strip.png",
                     frame_px * SCALE)

        body.location = original
        body.rotation_euler.z = -math.radians(SWEEP_START_DEG)
        for obj in bpy.context.scene.objects:
            if obj.type in {"MESH", "CURVE", "FONT"}:
                obj.visible_camera = True
                obj.hide_render = False
    clear_border(scn)


def stitch_strip(frame_paths, out_path, frame_size):
    """Stack frames vertically, frame 0 at the top. Loads/stores raw bytes
    (colorspace Non-Color) so pixel values pass through untouched."""
    frames = []
    for path in frame_paths:
        img = bpy.data.images.load(str(path))
        img.colorspace_settings.name = "Non-Color"
        w, h = img.size
        assert (w, h) == (frame_size, frame_size), f"{path}: {w}x{h}"
        arr = np.array(img.pixels[:], dtype=np.float32).reshape(h, w, 4)
        frames.append(arr)  # row 0 = image bottom
        bpy.data.images.remove(img)

    # Blender pixel rows run bottom-up: frame 0 must land in the topmost
    # block, i.e. at the END of the buffer; reverse the stack.
    strip = np.concatenate(list(reversed(frames)), axis=0)
    strip[strip[..., 3] == 0, :3] = 0.0   # deterministic bytes under alpha=0
    out = bpy.data.images.new("strip", width=frame_size,
                              height=frame_size * len(frames), alpha=True)
    out.colorspace_settings.name = "Non-Color"
    out.pixels.foreach_set(strip.ravel())
    out.filepath_raw = str(out_path)
    out.file_format = "PNG"
    out.save()
    bpy.data.images.remove(out)
    print(f"[render_ui] {out_path.name}: {len(frames)} frames, "
          f"{out_path.stat().st_size / 1024:.0f} KB")


def feather_alpha(path, feather=14):
    """Fade the overlay's alpha to zero over the outer `feather` render px.
    The parts' soft ambient shadows reach past the art rect; without this the
    crop boundary shows as a faint box on the plate. Every part's silhouette
    keeps a wider margin than the feather, so only shadow gradient is touched."""
    img = bpy.data.images.load(str(path))
    img.colorspace_settings.name = "Non-Color"
    w, h = img.size
    buf = np.zeros(w * h * 4, dtype=np.float32)
    img.pixels.foreach_get(buf)
    buf = buf.reshape(h, w, 4)
    ys, xs = np.mgrid[0:h, 0:w]
    dist = np.minimum.reduce([xs, ys, w - 1 - xs, h - 1 - ys]) / float(feather)
    ramp = np.clip(dist, 0.0, 1.0)
    ramp = ramp * ramp * (3.0 - 2.0 * ramp)          # smoothstep
    buf[..., 3] *= ramp
    img.pixels.foreach_set(buf.ravel())
    img.filepath_raw = str(path)
    img.file_format = "PNG"
    img.save()
    bpy.data.images.remove(img)


def pass_states(scn, pedal):
    """Each two-state assembly alone over a shadow-catcher plate: the overlay
    PNG carries the part and its own cast shadow, registered to its art rect."""
    jobs = [
        ("gate", "gate_pre", "gate_pre.png"),
        ("gate", "gate_post", "gate_post.png"),
        ("foot", "foot_up", "footswitch_up.png"),
        ("foot", "foot_down", "footswitch_down.png"),
        ("dark", "dark_off", "dark_off.png"),
        ("dark", "dark_on", "dark_on.png"),
        ("clip", "clip_off", "clip_off.png"),
        ("clip", "clip_on", "clip_on.png"),
    ]
    scn.render.film_transparent = True
    for rect_key, state_key, filename in jobs:
        restore_all(pedal)
        for obj in pedal.knob_objects():
            obj.hide_render = True          # keep knob shadows out of catchers
        show_only_states(pedal, {state_key})
        pedal.plate.is_shadow_catcher = True
        set_border(scn, ART_RECTS[rect_key])
        render_to(scn, OUT / filename)
        feather_alpha(OUT / filename)
    restore_all(pedal)
    show_only_states(pedal, DEFAULT_STATES)
    clear_border(scn)
    scn.render.film_transparent = False


# ----------------------------------------------------------------------------
# Entry
# ----------------------------------------------------------------------------

def main():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    targets = [a for a in argv if not a.startswith("--")] or ["all"]
    preview = "--preview" in argv

    t0 = time.time()
    scn = reset_scene()
    if preview:
        scn.cycles.samples = PREVIEW_SAMPLES
    add_camera(scn)
    add_lights(scn)
    mats = Mats()
    pedal = Pedal(mats)
    print(f"[render_ui] scene built in {time.time() - t0:.1f}s")

    if targets == ["all"] or "background" in targets:
        pass_background(scn, pedal)
    if targets == ["all"] or "knobs" in targets:
        pass_knob_strips(scn, pedal)
    if targets == ["all"] or "states" in targets:
        pass_states(scn, pedal)

    print(f"[render_ui] total {time.time() - t0:.1f}s")


if __name__ == "__main__":
    main()
