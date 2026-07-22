"""SendBloom faceplate renderer — the source of truth for resources/ui art.

Builds the whole pedal as one procedural Blender scene, lights it once, and
path-traces every UI asset with Cycles so all shadows, AO and reflections agree:

  pedal_background.png/.jpg   840x1560 plate with baked contact shadows
  knob_large_strip.png        65-frame low pointer-knob filmstrip, 168px frames
  knob_small_strip.png        65-frame tall barrel-knob filmstrip, 132px frames
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
import random
import sys
import time
from pathlib import Path

import bpy
import numpy as np
from mathutils import Matrix, Vector

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
CAMERA_PITCH_DEG = 13.5        # physical sidewall read, without perspective convergence

# facelayout rects (x, y, w, h in editor px) — the layout contract.
PLATE = (38.0, 60.0, 345.0, 642.0)
PLATE_RADIUS = 19.0
# Hammond's 1590B is the same 1.86:1 plan ratio as this face. Its published
# 60.5 mm width, 31 mm overall depth and 4.19 mm lid thickness give us a real
# dimensional scale instead of eyeballing how tall a stompbox "should" look.
PX_PER_MM = PLATE[2] / 60.50
ENCLOSURE_DEPTH = 31.00 * PX_PER_MM
LID_DEPTH = 4.19 * PX_PER_MM
FLOOR_Z = -ENCLOSURE_DEPTH
JACK_CENTRE_Z = -11.5 * PX_PER_MM
LOGO = (85, 70, 250, 52)
PRESET_FIELD = (54, 129, 232, 42)
PRESET_LOAD = (294, 136, 30, 29)
PRESET_SAVE = (331, 136, 30, 29)
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

# Materials are tuned around a black wrinkle-powder enclosure, warm ivory
# pad-print and the established SendBloom orange accent.
COL_PLATE = (0.024, 0.021, 0.018)
COL_CHASSIS = (0.010, 0.009, 0.008)
COL_BENCH = (0.012, 0.011, 0.010)
COL_KNOB = (0.035, 0.032, 0.027)
COL_POINTER = (0.710, 0.630, 0.455)
COL_RUBBER = (0.022, 0.022, 0.024)
LOGO_RELIEF = 1.00 * PX_PER_MM   # one physical millimetre above the orange inlay


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
    # Shallow orthographic pitch: enough to reveal the front enclosure wall and
    # control sidewalls, but still free of perspective convergence. Geometry is
    # counter-scaled in Y after construction so the z=0 control plane continues
    # to land on the exact JUCE layout coordinates.
    theta = math.radians(CAMERA_PITCH_DEG)
    target = Vector(px(EDITOR_W / 2, EDITOR_H / 2, 0.0))
    distance = 1500.0
    cam.location = target + Vector((0.0, -math.sin(theta) * distance,
                                    math.cos(theta) * distance))
    cam.rotation_euler = (theta, 0.0, 0.0)
    scn.collection.objects.link(cam)
    scn.camera = cam


def pitch_compensated_world_y(editor_y):
    """World Y whose z=0 projection lands at the requested editor Y."""
    centre_y = -EDITOR_H / 2.0
    world_y = -editor_y
    return centre_y + (world_y - centre_y) / math.cos(math.radians(CAMERA_PITCH_DEG))


def compensate_scene_for_camera_pitch(scn):
    """Undo orthographic Y foreshortening on the z=0 interaction plane.

    The camera still projects height into screen Y, exposing real sidewalls,
    while every mounting centre and printed mark on the plate remains aligned
    with its existing JUCE rectangle.
    """
    centre_y = -EDITOR_H / 2.0
    sec = 1.0 / math.cos(math.radians(CAMERA_PITCH_DEG))
    transform = (Matrix.Translation((0.0, centre_y, 0.0))
                 @ Matrix.Diagonal((1.0, sec, 1.0, 1.0))
                 @ Matrix.Translation((0.0, -centre_y, 0.0)))
    for obj in scn.objects:
        if obj.parent is None and obj.type in {"MESH", "CURVE", "FONT"}:
            obj.matrix_world = transform @ obj.matrix_world


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
    # It rounds off shadows and gives the ribbed hardware readable side light.
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

    # Low frontal kicker: real product photography uses a separate grazing card
    # to keep the enclosure's near wall and chrome from collapsing to black. It
    # is weak on the lid but draws a long highlight down the physical front wall.
    kick_data = bpy.data.lights.new("front_wall_kicker", type="AREA")
    kick_data.shape = "RECTANGLE"
    kick_data.size = 520
    kick_data.size_y = 180
    kick_data.color = (1.0, 0.78, 0.62)
    kick_data.energy = 1.35e6
    kick = bpy.data.objects.new("front_wall_kicker", kick_data)
    kick.location = centre + Vector((0, -860, 190))
    aim(kick, centre + Vector((0, -250, -75)))
    scn.collection.objects.link(kick)

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


def _nodes(mat):
    return mat.node_tree.nodes, mat.node_tree.links, mat.node_tree.nodes["Principled BSDF"]


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


def make_plate_material():
    """Black wrinkle powder coat, built as material rather than a colour swap.

    Two noise scales drive the irregular cured-powder relief, albedo and dry
    roughness. The broad height variation is the pebbled wrinkle a fingertip
    would feel; the fine pass breaks up highlights without becoming sandpaper.
    """
    mat = principled("plate_black_wrinkle", COL_PLATE, 0.64)
    nodes, links, bsdf = _nodes(mat)

    wrinkle = _noise(mat, 0.47, detail=9.0, w=4.0)
    wrinkle.noise_dimensions = "4D"
    wrinkle.inputs["Roughness"].default_value = 0.82
    micro = _noise(mat, 2.1, detail=5.0, w=12.0)

    # Black powder coat still carries visible local colour: wrinkle peaks catch
    # cool light while the valleys remain warm charcoal.
    tone = nodes.new("ShaderNodeValToRGB")
    tone.color_ramp.interpolation = "EASE"
    tone.color_ramp.elements[0].position = 0.20
    tone.color_ramp.elements[0].color = (0.007, 0.006, 0.005, 1.0)
    middle = tone.color_ramp.elements.new(0.52)
    middle.color = (*COL_PLATE, 1.0)
    tone.color_ramp.elements[-1].position = 0.82
    tone.color_ramp.elements[-1].color = (0.060, 0.050, 0.040, 1.0)
    links.new(wrinkle.outputs["Fac"], tone.inputs["Fac"])
    links.new(tone.outputs["Color"], bsdf.inputs["Base Color"])

    rough = add_rough_variation(mat, 0.64, 0.10, scale=0.17)
    dry_peaks = _math(mat, "MULTIPLY_ADD", wrinkle.outputs["Fac"], 0.10)
    dry_peaks.inputs[2].default_value = -0.05
    rough_final = _math(mat, "ADD", rough, dry_peaks.outputs["Value"])
    links.new(rough_final.outputs["Value"], bsdf.inputs["Roughness"])

    panel_warp = _noise(mat, 0.004, detail=2.0)
    chain_bumps(mat, [
        (panel_warp.outputs["Fac"], 0.10, 1.1),
        (wrinkle.outputs["Fac"], 0.72, 1.45),
        (micro.outputs["Fac"], 0.20, 0.24),
    ])

    add_ao_grime(mat, tint=(0.32, 0.27, 0.21), amount=0.32, distance=10.0)
    return mat


def make_bench_material():
    """Scarred black workbench rather than an empty studio backdrop.

    The shader supplies the continuous dirty surface; larger scratches and
    rubbed-through patches are separate geometry so they remain legible after
    the UI is reduced to its 420 px display size.
    """
    mat = principled("bench_scarred_black", (0.010, 0.009, 0.008), 0.82)
    nodes, links, bsdf = _nodes(mat)
    broad = _noise(mat, 3.6, detail=7.0, w=21.0)
    fine = _noise(mat, 34.0, detail=4.0, w=8.0)
    streak = _noise(mat, 5.0, detail=5.0, stretch=(1.0, 28.0),
                    rotation=math.radians(12.0), w=3.0)

    tone = nodes.new("ShaderNodeValToRGB")
    tone.color_ramp.interpolation = "EASE"
    tone.color_ramp.elements[0].position = 0.20
    tone.color_ramp.elements[0].color = (0.003, 0.003, 0.003, 1.0)
    mid = tone.color_ramp.elements.new(0.54)
    mid.color = (0.014, 0.012, 0.010, 1.0)
    tone.color_ramp.elements[-1].position = 0.82
    tone.color_ramp.elements[-1].color = (0.040, 0.034, 0.027, 1.0)
    links.new(broad.outputs["Fac"], tone.inputs["Fac"])
    links.new(tone.outputs["Color"], bsdf.inputs["Base Color"])

    rough = _math(mat, "MULTIPLY", fine.outputs["Fac"], 0.20)
    rough = _math(mat, "ADD", rough.outputs["Value"], 0.70)
    links.new(rough.outputs["Value"], bsdf.inputs["Roughness"])
    chain_bumps(mat, [
        (broad.outputs["Fac"], 0.20, 1.5),
        (streak.outputs["Fac"], 0.30, 0.55),
        (fine.outputs["Fac"], 0.10, 0.16),
    ])
    return mat


class Mats:
    def __init__(self):
        self.plate = make_plate_material()
        self.chassis = principled("chassis", COL_CHASSIS, 0.42, metallic=0.85)
        add_rough_variation(self.chassis, 0.42, 0.12, scale=0.05)
        cgrain = _noise(self.chassis, 0.5, detail=4.0)
        chain_bumps(self.chassis, [(cgrain.outputs["Fac"], 0.18, 0.5)])
        self.bench = make_bench_material()
        self.bench_scuff = principled("bench_rubbed_scuff", (0.105, 0.083, 0.060),
                                      0.76)
        self.bench_gouge = principled("bench_gouge", (0.0025, 0.0022, 0.0020),
                                      0.38)
        self.dust = principled("bench_dust", (0.115, 0.087, 0.055), 0.96)
        # Old aluminium is oxidised and dirty, not bright silver leaf. Keeping it
        # mid-dark also stops chips from looking pasted onto a black enclosure.
        self.exposed_metal = principled("exposed_aluminium", (0.225, 0.195, 0.145),
                                        0.58, metallic=0.82)
        add_rough_variation(self.exposed_metal, 0.58, 0.14, scale=0.9)
        self.damage_primer = principled("damage_oxide_primer", (0.060, 0.026, 0.012),
                                        0.92, metallic=0.12)
        self.damage_lip = principled("damage_raised_paint_lip", (0.006, 0.005, 0.004),
                                     0.66)
        self.scratch_groove = principled("scratch_groove_shadow", (0.003, 0.0025, 0.002),
                                         0.50)
        self.old_scratch = principled("oxidised_scratch", (0.115, 0.085, 0.055),
                                      0.72, metallic=0.28)
        self.ink_loss = principled("rubbed_through_print", (0.018, 0.016, 0.014), 0.78)
        self.panel_ink_faded = principled("panel_ink_faded", (0.225, 0.190, 0.125),
                                          0.84)
        self.knob_scuff = principled("knob_hand_scuff", (0.115, 0.100, 0.078), 0.88)
        self.rubber_scuff = principled("rubber_sole_scuff", (0.095, 0.086, 0.075),
                                       0.90)
        self.rubber_rib = principled("rubber_moulded_traction", (0.012, 0.012, 0.013),
                                     0.52)
        self.cable = principled("cable_black_rubber", (0.006, 0.006, 0.007), 0.64)
        cable_grain = _noise(self.cable, 1.1, detail=4.0, stretch=(1.0, 8.0))
        chain_bumps(self.cable, [(cable_grain.outputs["Fac"], 0.13, 0.28)])
        self.heatshrink = principled("cable_orange_heatshrink", (0.42, 0.055, 0.012),
                                     0.64)
        # Injection-moulded pedal knobs are satin phenolic/ABS, not piano-black.
        # The broad, soft response and fine mould grain keep the ribs readable
        # without turning every flute into a glossy gear tooth.
        self.knob = principled("knob_body", COL_KNOB, 0.56, coat=0.05, coat_rough=0.38)
        add_rough_variation(self.knob, 0.56, 0.07, scale=0.16)
        kgrain = _noise(self.knob, 0.85, detail=3.0)
        chain_bumps(self.knob, [(kgrain.outputs["Fac"], 0.10, 0.28)])
        # A separate, perfectly planar top insert prevents the curved grip
        # normals and studio reflection from visually inflating it into a dome.
        self.knob_top = principled("knob_flat_top", (0.027, 0.025, 0.022), 0.82)
        topgrain = _noise(self.knob_top, 1.2, detail=3.0)
        chain_bumps(self.knob_top, [(topgrain.outputs["Fac"], 0.045, 0.12)])
        self.knob_recess = principled("knob_top_recess", (0.004, 0.0035, 0.003), 0.72)
        # The index is paint-filled ivory, like a physical stompbox control.
        # Metallic pointer bars read as hi-fi equipment rather than a pedal.
        self.pointer = principled("pointer_paint", COL_POINTER, 0.58)
        add_rough_variation(self.pointer, 0.58, 0.05, scale=0.4)
        self.panel_ink = principled("panel_scale_ink", (0.610, 0.535, 0.385), 0.72)
        self.knob_mount = principled("knob_mount_nickel_washer", (0.53, 0.50, 0.44),
                                     0.25, metallic=1.0)
        add_rough_variation(self.knob_mount, 0.25, 0.08, scale=0.34)
        self.chrome = principled("chrome", (0.85, 0.86, 0.88), 0.14, metallic=1.0)
        add_rough_variation(self.chrome, 0.16, 0.08, scale=0.25)
        self.steel = principled("steel", (0.62, 0.62, 0.63), 0.34, metallic=1.0)
        add_rough_variation(self.steel, 0.34, 0.12, scale=0.3)
        self.nickel = principled("nickel", (0.72, 0.71, 0.68), 0.28, metallic=1.0)
        add_rough_variation(self.nickel, 0.28, 0.10, scale=0.4)
        # The mini-toggle is touched constantly and lives beside a cable path;
        # pristine mirror chrome made it read like jewellery. Its own hardware
        # family is warmer, rougher and visibly handled.
        self.gate_nickel = principled("gate_aged_warm_nickel",
                                      (0.410, 0.365, 0.285), 0.39, metallic=0.92)
        add_rough_variation(self.gate_nickel, 0.39, 0.13, scale=0.34)
        gate_nickel_grain = _noise(self.gate_nickel, 1.35, detail=4.0)
        chain_bumps(self.gate_nickel,
                    [(gate_nickel_grain.outputs["Fac"], 0.075, 0.17)])
        add_ao_grime(self.gate_nickel, amount=0.42, distance=2.2)
        self.gate_steel = principled("gate_dark_threaded_steel",
                                     (0.115, 0.095, 0.070), 0.48, metallic=0.84)
        add_rough_variation(self.gate_steel, 0.48, 0.11, scale=0.52)
        add_ao_grime(self.gate_steel, amount=0.46, distance=1.8)
        self.gate_phenolic = principled("gate_black_phenolic_tip",
                                        (0.013, 0.011, 0.009), 0.62, coat=0.04,
                                        coat_rough=0.44)
        gate_tip_grain = _noise(self.gate_phenolic, 1.8, detail=4.0)
        chain_bumps(self.gate_phenolic,
                    [(gate_tip_grain.outputs["Fac"], 0.085, 0.15)])
        self.gate_tarnish = principled("gate_contact_tarnish",
                                       (0.055, 0.024, 0.010), 0.70, metallic=0.30)
        # The badge is now built from the exact vector paths as separate solids.
        # These materials therefore describe real surfaces rather than regions
        # of one photograph: nickel rim, black recessed bed, orange enamel and
        # blackened raised letters all respond independently to the light rig.
        self.badge_face = principled("badge_recessed_black_enamel",
                                     (0.006, 0.005, 0.004), 0.34,
                                     metallic=0.58, coat=0.16, coat_rough=0.25)
        add_rough_variation(self.badge_face, 0.34, 0.055, scale=0.62)
        badge_face_grain = _noise(self.badge_face, 1.8, detail=4.0)
        chain_bumps(self.badge_face, [(badge_face_grain.outputs["Fac"], 0.055, 0.16)])
        add_ao_grime(self.badge_face, amount=0.42, distance=3.6)
        self.badge_orange = principled("badge_orange_baked_enamel",
                                       (0.690, 0.155, 0.037), 0.37,
                                       metallic=0.08, coat=0.30, coat_rough=0.27)
        add_rough_variation(self.badge_orange, 0.37, 0.055, scale=0.75)
        badge_orange_peel = _noise(self.badge_orange, 2.4, detail=4.0)
        chain_bumps(self.badge_orange,
                    [(badge_orange_peel.outputs["Fac"], 0.075, 0.18)])
        add_ao_grime(self.badge_orange, amount=0.25, distance=2.8)
        self.badge_letter = principled("badge_blackened_raised_letters",
                                       (0.022, 0.019, 0.016), 0.43,
                                       metallic=0.52, coat=0.10, coat_rough=0.31)
        add_rough_variation(self.badge_letter, 0.43, 0.065, scale=0.95)
        badge_letter_grain = _noise(self.badge_letter, 2.9, detail=3.0)
        chain_bumps(self.badge_letter,
                    [(badge_letter_grain.outputs["Fac"], 0.050, 0.13)])
        add_ao_grime(self.badge_letter, amount=0.40, distance=2.5)
        # Preset hardware stays inside the established product palette: smoked
        # black enamel, nickel, orange inlay and warm-ivory raised markings.
        # The legacy cream extraction is archival, not a colour reference for
        # this black-wrinkle enclosure.
        self.preset_face = principled("preset_smoked_black_enamel",
                                      (0.018, 0.015, 0.012), 0.43,
                                      metallic=0.22, coat=0.14, coat_rough=0.31)
        add_rough_variation(self.preset_face, 0.43, 0.075, scale=0.72)
        preset_broad = _noise(self.preset_face, 0.42, detail=4.0, w=18.0)
        preset_grain = _noise(self.preset_face, 3.1, detail=4.0, w=5.0)
        chain_bumps(self.preset_face, [
            (preset_broad.outputs["Fac"], 0.080, 0.24),
            (preset_grain.outputs["Fac"], 0.055, 0.12),
        ])
        add_ao_grime(self.preset_face, amount=0.34, distance=2.8)
        self.preset_gasket = principled("preset_black_insulating_gasket",
                                        (0.006, 0.0055, 0.005), 0.68)
        add_rough_variation(self.preset_gasket, 0.68, 0.06, scale=0.58)
        self.preset_ink = principled("preset_modelled_ink_hardware",
                                     (0.016, 0.015, 0.013), 0.48,
                                     metallic=0.38)
        add_rough_variation(self.preset_ink, 0.48, 0.05, scale=1.1)
        self.preset_marking = principled("preset_warm_ivory_marking",
                                         (0.610, 0.535, 0.385), 0.66)
        add_rough_variation(self.preset_marking, 0.66, 0.045, scale=1.2)
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
        # Faded warm pad-print: subdued against the moulded cap, but still
        # legible at the editor's real 1x size and after the dark-room overlay.
        self.button_text = principled("button_text", (0.220, 0.180, 0.110), 0.70)
        add_rough_variation(self.button_text, 0.70, 0.045, scale=1.1)
        self.slot = principled("slot", (0.05, 0.05, 0.05), 0.5, metallic=0.6)
        self.lens_off = principled("lens_off", (0.045, 0.006, 0.005), 0.08, coat=1.0, coat_rough=0.05)
        self.lens_on = self._lens_on()

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
            # Moulded grip ribs have broad lands separated by narrow axial cuts;
            # a raw cosine makes the whole wall undulate like a melon. Sharpen the
            # groove profile so fingers meet distinct vertical flats and valleys.
            groove = (0.5 + 0.5 * math.cos(flutes * a)) ** 4 if flutes else 0.0
            rr = r - scallop * groove if flutes else r
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


def make_axis_cylinder(name, mat, radius, length, centre, axis="z", verts=96,
                       smooth=True):
    """Cylinder whose named local axis becomes a world axis."""
    obj = make_cylinder(name, mat, radius, -length / 2.0, length / 2.0,
                        centre, verts=verts, smooth=smooth)
    if axis == "x":
        obj.rotation_euler.y = math.radians(90.0)
    elif axis == "y":
        obj.rotation_euler.x = math.radians(90.0)
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


def make_polygon_prism(name, mat, points_px, z0, z1, bevel=0.0):
    """Extrude an arbitrary editor-space polygon into a small hardware solid."""
    bm = bmesh.new()
    outline = [bm.verts.new(px(x, y, z0)) for x, y in points_px]
    base = bm.faces.new(list(reversed(outline)))
    result = bmesh.ops.extrude_face_region(bm, geom=[base])
    top_verts = [element for element in result["geom"]
                 if isinstance(element, bmesh.types.BMVert)]
    bmesh.ops.translate(bm, verts=top_verts, vec=(0, 0, z1 - z0))
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    if bevel > 0:
        top_edges = [edge for edge in bm.edges
                     if all(abs(vertex.co.z - z1) < 1e-5 for vertex in edge.verts)]
        bmesh.ops.bevel(bm, geom=top_edges, offset=bevel, segments=2,
                        profile=0.65, affect="EDGES")
    mesh = bpy.data.meshes.new(name)
    bm.to_mesh(mesh)
    bm.free()
    return link_object(name, mesh, mat)


def make_curve_tube(name, mat, points, radius, resolution=4):
    """A weighty cable or hose following a smooth 3D path."""
    curve = bpy.data.curves.new(name, type="CURVE")
    curve.dimensions = "3D"
    curve.resolution_u = 10
    curve.bevel_depth = radius
    curve.bevel_resolution = resolution
    curve.resolution_u = 12
    curve.fill_mode = "FULL"
    spline = curve.splines.new("BEZIER")
    spline.bezier_points.add(len(points) - 1)
    for bp, co in zip(spline.bezier_points, points):
        bp.co = co
        bp.handle_left_type = "AUTO"
        bp.handle_right_type = "AUTO"
    obj = bpy.data.objects.new(name, curve)
    obj.data.materials.append(mat)
    bpy.context.scene.collection.objects.link(obj)
    return obj


def make_surface_stroke(name, mat, centre_px, length, width, z, angle_deg=0.0):
    """Thin top-facing mark authored in editor coordinates."""
    return make_box(name, mat, (width, length, 0.10), px(*centre_px, z),
                    rot=(0.0, 0.0, math.radians(angle_deg)))


def make_irregular_patch(name, mat, centre_px, rx, ry, z, seed, points=9,
                         angle_deg=0.0):
    """A small ragged top-surface patch for chips, rubbed print and dirt."""
    rng = random.Random(seed)
    rotation = math.radians(angle_deg)
    verts = []
    for i in range(points):
        a = math.tau * i / points
        wobble = rng.uniform(0.66, 1.22)
        lx = math.cos(a) * rx * wobble
        ly = math.sin(a) * ry * wobble
        x = centre_px[0] + lx * math.cos(rotation) - ly * math.sin(rotation)
        y = centre_px[1] + lx * math.sin(rotation) + ly * math.cos(rotation)
        verts.append(px(x, y, z))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], [list(reversed(range(points)))])
    mesh.update()
    return link_object(name, mesh, mat)


def make_front_wall_patch(name, mat, centre_x, centre_z, rx, rz, editor_y,
                          seed, points=10, angle_deg=0.0, outset=0.18):
    """Ragged patch laid on the enclosure's vertical front wall (X/Z plane)."""
    rng = random.Random(seed)
    rotation = math.radians(angle_deg)
    verts = []
    for i in range(points):
        a = math.tau * i / points
        wobble = rng.uniform(0.68, 1.20)
        lx = math.cos(a) * rx * wobble
        lz = math.sin(a) * rz * wobble
        x = centre_x + lx * math.cos(rotation) - lz * math.sin(rotation)
        z = centre_z + lx * math.sin(rotation) + lz * math.cos(rotation)
        verts.append((x, -editor_y - outset, z))
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], [list(range(points))])
    mesh.update()
    return link_object(name, mesh, mat)


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


def _append_reversed_bezier_spline(target_curve, source_spline):
    """Copy a closed SVG Bezier path with opposite winding.

    Blender's 2D curve fill uses spline winding for holes. The Niko 'o' arrives
    from the source SVG as separate outline and counter objects; combining them
    this way makes the counter a real opening with vertical inner walls.
    """
    if source_spline.type != "BEZIER":
        raise RuntimeError("SendBloom badge counter must remain a Bezier path")
    source_points = list(source_spline.bezier_points)
    target = target_curve.splines.new("BEZIER")
    target.bezier_points.add(len(source_points) - 1)
    target.use_cyclic_u = True
    target.resolution_u = source_spline.resolution_u
    for dest, source in zip(target.bezier_points, reversed(source_points)):
        dest.co = source.co
        dest.handle_left_type = "FREE"
        dest.handle_right_type = "FREE"
        # Reversing traversal also reverses the incoming/outgoing handles.
        dest.handle_left = source.handle_right
        dest.handle_right = source.handle_left


def _finish_logo_curve(obj, name, mat, scale, offset, z0, z1, bevel):
    """Turn one imported SVG path into a bevelled, dimensioned badge solid."""
    curve = obj.data
    # Do not use Curve.transform() here: Blender also scales its later bevel and
    # extrusion evaluation, turning a 1 mm relief into a metre-high object. SVG
    # control points and their handles are scaled explicitly in X/Y instead.
    for spline in curve.splines:
        if spline.type == "BEZIER":
            for point in spline.bezier_points:
                point.co.x *= scale
                point.co.y *= scale
                point.handle_left.x *= scale
                point.handle_left.y *= scale
                point.handle_right.x *= scale
                point.handle_right.y *= scale
        else:
            for point in spline.points:
                point.co.x *= scale
                point.co.y *= scale
    curve.dimensions = "2D"
    curve.fill_mode = "BOTH"
    curve.resolution_u = 16
    curve.render_resolution_u = 20
    curve.offset = 0.0
    # A 2D curve extrudes symmetrically about local z. Subtracting the bevel
    # keeps the requested finished height exact instead of growing past it.
    depth = z1 - z0
    curve.extrude = max(0.01, depth * 0.5 - bevel)
    curve.bevel_depth = bevel
    curve.bevel_resolution = 4
    curve.resolution_v = 4
    curve.materials.clear()
    curve.materials.append(mat)
    curve.name = name + "_mesh"
    obj.name = name
    obj.location = (offset[0], offset[1], (z0 + z1) * 0.5)
    return obj


def make_modelled_logo(mats, rect):
    """Build the plaque from the existing exact wordmark vector paths.

    The raster logo is deliberately not sampled here. The SVG supplies only
    silhouettes; Cycles supplies the bevel highlights, inner-wall occlusion,
    contact shadows and material response for each physical layer.
    """
    svg_path = OUT / "brand_logo.svg"
    if not svg_path.exists():
        raise RuntimeError(f"Missing modelled badge source: {svg_path}")

    before = set(bpy.context.scene.objects)
    bpy.ops.import_curve.svg(filepath=str(svg_path))
    imported = [obj for obj in bpy.context.scene.objects if obj not in before]
    by_name = {obj.name: obj for obj in imported}
    required = {
        "outer": "Curve.003",
        "face": "Curve.004",
        "inlay": "Curve.011",
        "N": "n-recess",
        "i_dot": "i-dot-recess",
        "i_stem": "i-stem-recess",
        "k": "k-recess",
        "o": "o-recess",
        "o_counter": "o-counter",
        "F": "f",
        "X_top": "x-top",
        "X_main": "x-main",
        "X_low": "x-low",
    }
    missing = [svg_name for svg_name in required.values() if svg_name not in by_name]
    if missing:
        raise RuntimeError("Badge SVG path contract changed: " + ", ".join(missing))
    paths = {key: by_name[value] for key, value in required.items()}

    # Make the O counter an actual hole before deleting the source counter path.
    _append_reversed_bezier_spline(paths["o"].data,
                                   paths["o_counter"].data.splines[0])

    # Use the exact outer-rim bounds as the registration contract. A uniform
    # scale preserves the user's wordmark proportions; any remaining vertical
    # room is centred rather than stretching the letters to fit the UI rect.
    src_x0, src_x1 = 0.01608, 0.55522
    src_y0, src_y1 = 0.24069, 0.34537
    x, y, w, h = rect
    scale = min(w / (src_x1 - src_x0), h / (src_y1 - src_y0))
    source_centre = ((src_x0 + src_x1) * 0.5,
                     (src_y0 + src_y1) * 0.5)
    target_centre = (x + w * 0.5, -(y + h * 0.5))
    offset = (target_centre[0] - source_centre[0] * scale,
              target_centre[1] - source_centre[1] * scale)

    rim_top = 4.65
    face_top = 5.65
    inlay_top = 6.35
    letter_top = inlay_top + LOGO_RELIEF
    solids = [
        _finish_logo_curve(paths["outer"], "badge_nickel_rim", mats.nickel,
                           scale, offset, 0.10, rim_top, 0.62),
        _finish_logo_curve(paths["face"], "badge_recessed_face", mats.badge_face,
                           scale, offset, rim_top - 0.38, face_top, 0.26),
        _finish_logo_curve(paths["inlay"], "badge_orange_inlay", mats.badge_orange,
                           scale, offset, face_top - 0.12, inlay_top, 0.22),
        # The O has a true open counter, but its bottom is the same orange enamel
        # as the surrounding inlay. Keeping this as a separate recessed solid
        # preserves the original wordmark while exposing real inner sidewalls.
        _finish_logo_curve(paths["o_counter"], "badge_o_counter_inlay",
                           mats.badge_orange, scale, offset,
                           face_top - 0.12, inlay_top, 0.18),
    ]

    # N/i/k/o sit through matching apertures in the orange inlay. F/X rise from
    # the black field. All tops finish in one plane, exactly 1 mm above the
    # orange surface, so every glyph casts the same physically meaningful shadow.
    for key, label, material in (
            ("N", "N", mats.badge_letter),
            ("i_dot", "i_dot", mats.badge_letter),
            ("i_stem", "i_stem", mats.badge_letter),
            ("k", "k", mats.badge_letter),
            ("o", "o", mats.badge_letter),
            ("F", "F", mats.badge_orange),
            ("X_top", "X_top", mats.badge_orange),
            ("X_main", "X_main", mats.badge_orange),
            ("X_low", "X_low", mats.badge_orange)):
        solids.append(_finish_logo_curve(
            paths[key], "badge_letter_" + label, material, scale, offset,
            face_top - 0.12, letter_top, 0.52))

    keep = set(solids)
    for obj in imported:
        if obj not in keep:
            bpy.data.objects.remove(obj, do_unlink=True)
    return solids, letter_top


def _make_preset_button(mats, name, rect, kind):
    """One complete panel pushbutton, including icon and modelled label."""
    x, y, w, h = rect
    cx, cy = x + w * 0.5, y + h * 0.5
    objects = []

    # Panel cut-out -> dark isolating well -> nickel carrier -> gasket -> cap.
    # The small z overlaps prevent light leaks between stacked solids while the
    # visible stepped edges produce real contact occlusion at plugin size.
    objects.append(make_prism(
        name + "_panel_well", mats.preset_gasket, w + 2.2, h + 2.2, 6.0,
        -0.35, 0.35, bevel=0.55, centre=px(cx, cy)[:2]))
    objects.append(make_prism(
        name + "_nickel_carrier", mats.nickel, w, h, 5.2,
        0.05, 3.35, bevel=0.82, centre=px(cx, cy)[:2]))
    objects.append(make_prism(
        name + "_black_gasket", mats.preset_gasket, w - 2.2, h - 2.2, 4.3,
        2.75, 3.85, bevel=0.42, centre=px(cx, cy)[:2]))
    cap = make_prism(
        name + "_smoked_black_cap", mats.preset_face, w - 4.1, h - 4.1, 3.4,
        3.25, 4.80, bevel=0.62, centre=px(cx, cy)[:2])
    objects.append(cap)

    # Orange side witness marks are enamel-filled grooves in the cap, not
    # decoration pasted beside the button. Their short height keeps them below
    # the icon/legend hierarchy.
    for side, sx in (("left", x + 3.0), ("right", x + w - 3.0)):
        objects.append(make_box(
            f"{name}_{side}_orange_inlay", mats.badge_orange,
            (0.85, 4.2, 0.24), px(sx, cy, 4.91)))

    icon_z0, icon_z1 = 4.84, 5.66
    icon_cy = y + 9.8
    if kind == "load":
        # Folder mechanism: back plate with tab, a pale document caught inside,
        # and a deeper front pocket. Each layer casts into the next.
        folder_back = make_polygon_prism(
            name + "_folder_back", mats.nickel,
            [(cx - 7.0, icon_cy - 4.2), (cx - 1.8, icon_cy - 4.2),
             (cx + 1.1, icon_cy - 1.9), (cx + 7.0, icon_cy - 1.9),
             (cx + 7.0, icon_cy + 3.9), (cx - 7.0, icon_cy + 3.9)],
            icon_z0, icon_z1, bevel=0.28)
        document = make_prism(
            name + "_folder_document", mats.badge_orange, 9.5, 5.3, 0.7,
            5.45, 5.84, bevel=0.16, centre=px(cx, icon_cy - 0.1)[:2])
        pocket = make_prism(
            name + "_folder_pocket", mats.nickel, 14.5, 6.2, 1.0,
            5.48, 6.12, bevel=0.30, centre=px(cx, icon_cy + 2.3)[:2])
        objects += [folder_back, document, pocket]
    elif kind == "save":
        # Disk body, upper label/shutter and lower write-label recess are all
        # separate pieces. The tiny horizontal slots remain readable at 2x.
        disk = make_prism(
            name + "_disk_body", mats.nickel, 13.4, 11.5, 1.35,
            icon_z0, icon_z1, bevel=0.36, centre=px(cx, icon_cy)[:2])
        shutter = make_prism(
            name + "_disk_shutter", mats.preset_gasket, 7.0, 3.5, 0.55,
            5.50, 5.90, bevel=0.14, centre=px(cx, icon_cy - 3.0)[:2])
        shutter_slot = make_box(
            name + "_disk_shutter_slot", mats.badge_orange,
            (1.25, 2.3, 0.22), px(cx + 1.5, icon_cy - 3.0, 6.01))
        label = make_prism(
            name + "_disk_label", mats.preset_gasket, 9.0, 4.2, 0.65,
            5.50, 5.88, bevel=0.14, centre=px(cx, icon_cy + 2.2)[:2])
        slot_a = make_box(name + "_disk_line_a", mats.badge_orange,
                          (6.5, 0.42, 0.18), px(cx, icon_cy + 1.4, 5.99))
        slot_b = make_box(name + "_disk_line_b", mats.badge_orange,
                          (6.5, 0.42, 0.18), px(cx, icon_cy + 2.9, 5.99))
        objects += [disk, shutter, shutter_slot, label, slot_a, slot_b]
    else:
        raise ValueError(f"Unknown preset button kind: {kind}")

    label = make_text(
        name + "_legend", mats.preset_marking, kind.upper(),
        px(cx, y + h - 5.2, 5.05), size=7.0, extrude=0.20)
    label.scale.x = 0.70
    label.data.bevel_depth = 0.08
    label.data.bevel_resolution = 2
    objects.append(label)
    return objects, 6.12


def make_modelled_preset_hardware(mats):
    """Build selector, LOAD and SAVE without any raster face materials."""
    x, y, w, h = PRESET_FIELD
    cx, cy = x + w * 0.5, y + h * 0.5
    objects = [
        make_prism("preset_selector_panel_well", mats.preset_gasket,
                   w + 3.0, h + 3.0, 7.2, -0.40, 0.36, bevel=0.65,
                   centre=px(cx, cy)[:2]),
        make_prism("preset_selector_nickel_bezel", mats.nickel,
                   w, h, 6.3, 0.04, 3.45, bevel=0.95,
                   centre=px(cx, cy)[:2]),
        make_prism("preset_selector_black_gasket", mats.preset_gasket,
                   w - 4.2, h - 4.2, 5.2, 2.75, 4.10, bevel=0.48,
                   centre=px(cx, cy)[:2]),
        make_prism("preset_selector_smoked_black_carrier", mats.preset_face,
                   w - 8.0, h - 8.0, 4.1, 3.42, 4.62, bevel=0.58,
                   centre=px(cx, cy)[:2]),
        make_prism("preset_selector_live_face", mats.preset_face,
                   w - 12.0, h - 12.0, 3.2, 4.25, 4.88, bevel=0.34,
                   centre=px(cx, cy)[:2]),
    ]

    # A flush black triangular inlay replaces the old baked ComboBox arrow. A
    # larger dark socket beneath it provides a fine shadow line on every side.
    arrow_cx, arrow_cy = x + w - 12.0, cy
    objects.append(make_polygon_prism(
        "preset_selector_arrow_socket", mats.preset_gasket,
        [(arrow_cx - 5.2, arrow_cy - 4.0),
         (arrow_cx + 5.2, arrow_cy - 4.0),
         (arrow_cx, arrow_cy + 4.8)],
        4.70, 4.94, bevel=0.18))
    objects.append(make_polygon_prism(
        "preset_selector_arrow_inlay", mats.badge_orange,
        [(arrow_cx - 3.9, arrow_cy - 2.9),
         (arrow_cx + 3.9, arrow_cy - 2.9),
         (arrow_cx, arrow_cy + 3.7)],
        4.88, 5.10, bevel=0.12))

    # Small upper/lower internal rails turn the long field into a manufactured
    # insert rather than an empty rectangle. They stop before live text.
    objects.append(make_box(
        "preset_selector_upper_rail", mats.nickel,
        (w - 19.0, 0.32, 0.16), px(cx - 2.0, y + 5.6, 4.94)))
    objects.append(make_box(
        "preset_selector_lower_rail", mats.badge_orange,
        (w - 19.0, 0.26, 0.12), px(cx - 2.0, y + h - 5.6, 4.94)))

    load_objects, load_top = _make_preset_button(
        mats, "preset_load", PRESET_LOAD, "load")
    save_objects, save_top = _make_preset_button(
        mats, "preset_save", PRESET_SAVE, "save")
    objects += load_objects + save_objects
    return objects, max(5.10, load_top, save_top)


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

        # Dimensioned from Hammond's 1590B drawing, not an arbitrary render-unit
        # depth: 31 mm overall with a 4.19 mm lid. At this face width that is a
        # 177-unit wall, almost five times the previous shell. The camera now has
        # enough real height to produce the front face and ground shadow a human
        # expects from a stompbox.
        bench = make_box("bench", m.bench, (2400, 4000, 8),
                         px(EDITOR_W / 2, EDITOR_H / 2, FLOOR_Z - 4.0))
        chassis = make_prism("chassis", m.chassis, w + 34, h + 34, PLATE_RADIUS + 12,
                             FLOOR_Z, -LID_DEPTH * 0.70, bevel=5.0,
                             centre=px(x + w / 2, y + h / 2)[:2])
        plate = make_prism("plate", m.plate, w, h, PLATE_RADIUS,
                           -LID_DEPTH, 0, bevel=3.0,
                           centre=px(x + w / 2, y + h / 2)[:2])
        self.plate = plate
        self.static += [bench, chassis, plate]

        self._build_floor_wear_and_cables()
        self._build_panel_wear_and_graphics()
        self._build_badge_and_preset()
        self._build_screws()
        self._build_knobs()
        self._build_gate()
        self._build_dark()
        self._build_foot()
        self._build_clip()

    def _build_floor_wear_and_cables(self):
        """Dimensioned side sockets, fully inserted right-angle plugs and leads.

        A real connection is a sequence: panel cut-out -> socket bezel/hex nut ->
        threaded nose -> inserted 6.35 mm shaft -> plug shell -> grooved strain
        relief -> 5–7 mm cable. The old scene omitted that chain and parked shiny
        bars on the lid. These assemblies cross the enclosure wall at a real side
        height and the cables rise from the floor into them under their own weight.
        """
        m = self.m
        floor_mark_z = FLOOR_Z + 0.45

        # Workbench damage is clustered where a pedal is dragged or a plug is
        # dropped; it is environment history, not decoration on the product.
        strokes = [
            ("drag_l", 28, 742, 76, 1.2, 63, m.bench_scuff),
            ("drag_r", 366, 744, 88, 1.5, -58, m.bench_scuff),
            ("drop_tl", 29, 43, 39, 0.9, -43, m.bench_gouge),
            ("drop_tr", 397, 47, 42, 1.0, 39, m.bench_gouge),
            ("cable_l", 13, 232, 52, 0.8, -5, m.bench_scuff),
            ("cable_r", 406, 545, 61, 0.8, 7, m.bench_scuff),
        ]
        for suffix, x, y, length, width, angle, mat in strokes:
            self.static.append(make_surface_stroke(
                f"floor_scratch_{suffix}", mat, (x, y), length, width,
                floor_mark_z, angle))

        for i, (x, y, rx, ry, angle) in enumerate((
                (25, 75, 13, 4, -18), (394, 93, 17, 5, 24),
                (12, 431, 11, 4, -7), (386, 748, 19, 6, -31),
                (242, 758, 10, 3.5, 8))):
            self.static.append(make_irregular_patch(
                f"floor_rub_{i}", m.dust, (x, y), rx, ry,
                floor_mark_z + 0.08, 3700 + i, points=12, angle_deg=angle))

        chassis_left = PLATE[0] - 17.0
        chassis_right = PLATE[0] + PLATE[2] + 17.0
        jack_y = 367.0

        def build_connection(side, wall_x, outward):
            shell_x = wall_x + outward * 12.5
            z = JACK_CENTRE_Z
            objs = []

            # Cable lies on the floor, then makes a short, load-bearing rise to
            # the plug boot. This vertical transition supplies the missing cable
            # weight and a visible contact shadow on the bench.
            cable = make_curve_tube(f"cable_{side}", m.cable, [
                px(shell_x, -28, FLOOR_Z + 6.0),
                px(shell_x + outward * 1.5, 92, FLOOR_Z + 6.0),
                px(shell_x - outward * 1.0, 235, FLOOR_Z + 6.4),
                px(shell_x, 294, FLOOR_Z + 16.0),
                px(shell_x, 320, z),
            ], 5.6, resolution=5)
            objs.append(cable)

            # Grooved POM/rubber strain relief, dimensionally based on a slim
            # Neutrik-style 4–7 mm cable boot, plus an orange identity ring.
            boot = make_axis_cylinder(f"plug_{side}_boot", m.cable, 7.3, 31.0,
                                      px(shell_x, 340.5, z), axis="y", verts=64)
            objs.append(boot)
            for ri, yy in enumerate((328.5, 334.5, 340.5, 346.5, 352.5)):
                objs.append(make_axis_cylinder(
                    f"plug_{side}_boot_rib_{ri}", m.cable, 8.2, 2.1,
                    px(shell_x, yy, z), axis="y", verts=64))
            objs.append(make_axis_cylinder(
                f"plug_{side}_identity_ring", m.heatshrink, 7.8, 4.0,
                px(shell_x, 324.0, z), axis="y", verts=64))

            # Diecast right-angle shell: rounded zinc body, centre seam and two
            # retained cover screws. Its horizontal nose is aligned with the
            # socket axis—not laid on the top panel.
            shell = make_prism(f"plug_{side}_shell", m.chrome, 23.0, 34.0, 7.0,
                               z - 9.0, z + 9.0, bevel=2.0,
                               centre=px(shell_x, jack_y)[:2])
            objs.append(shell)
            objs.append(make_surface_stroke(
                f"plug_{side}_shell_seam", m.slot, (shell_x, jack_y),
                27.0, 0.65, z + 9.35, 0.0))
            for si, sy in enumerate((jack_y - 8.5, jack_y + 8.5)):
                screw = make_cylinder(f"plug_{side}_cover_screw_{si}", m.nickel,
                                      1.8, 0.0, 0.8,
                                      px(shell_x - outward * 2.0, sy, z + 9.1),
                                      verts=48)
                slot = make_box(f"plug_{side}_cover_slot_{si}", m.slot,
                                (2.8, 0.45, 0.25),
                                px(shell_x - outward * 2.0, sy, z + 10.0),
                                rot=(0.0, 0.0, math.radians(35 if si else -28)))
                objs += [screw, slot]

            # The male shaft visibly disappears through the socket nose. The
            # panel hardware is ordered outside-in: chrome hex facia, black
            # insulating bezel, threaded nickel nose and slim insertion shaft.
            shaft_centre = wall_x + outward * 7.0
            objs.append(make_axis_cylinder(
                f"plug_{side}_inserted_shaft", m.chrome, 3.18, 17.0,
                px(shaft_centre, jack_y, z), axis="x", verts=64))
            objs.append(make_axis_cylinder(
                f"jack_{side}_black_bezel", m.button_black, 11.0, 2.0,
                px(wall_x + outward * 0.7, jack_y, z), axis="x", verts=64))
            objs.append(make_axis_cylinder(
                f"jack_{side}_chrome_hex", m.chrome, 9.7, 4.2,
                px(wall_x + outward * 2.6, jack_y, z), axis="x",
                verts=6, smooth=False))
            nose_x = wall_x + outward * 6.0
            objs.append(make_axis_cylinder(
                f"jack_{side}_threaded_nose", m.nickel, 6.2, 8.5,
                px(nose_x, jack_y, z), axis="x", verts=64))
            for ti in range(4):
                tx = wall_x + outward * (4.1 + ti * 1.45)
                objs.append(make_axis_cylinder(
                    f"jack_{side}_thread_{ti}", m.chrome, 6.65, 0.55,
                    px(tx, jack_y, z), axis="x", verts=64))
            objs.append(make_axis_cylinder(
                f"plug_{side}_nose_collar", m.steel, 7.7, 4.0,
                px(wall_x + outward * 10.2, jack_y, z), axis="x", verts=64))

            self.static.extend(objs)

        build_connection("left", chassis_left, -1.0)
        build_connection("right", chassis_right, 1.0)

    def _build_panel_wear_and_graphics(self):
        """Readable use-history: chipped powder coat and abraded pad print."""
        m = self.m

        # A broken warm-ivory signal frame gives the lower half the large graphic
        # gesture seen on real pedals. Missing pieces are authored as gaps, so the
        # line looks rubbed away rather than computer-perfect.
        # Every segment observes a physical keep-out. In particular, the lower
        # runs stop outside the screw-head radius and treadle carrier instead of
        # continuing underneath hardware as if the print were drawn afterward.
        frame_segments = [
            ("top_l", 79, 401, 36, 1.45, 90, m.panel_ink),
            ("top_m", 194, 401, 116, 1.45, 90, m.panel_ink_faded),
            ("top_r", 336, 401, 42, 1.45, 90, m.panel_ink),
            ("left_a", 59, 439, 62, 1.45, 0, m.panel_ink),
            ("left_b", 59, 515, 70, 1.45, 0, m.panel_ink_faded),
            ("left_c", 59, 607, 92, 1.45, 0, m.panel_ink),
            ("right_a", 361, 441, 66, 1.45, 0, m.panel_ink),
            ("right_b", 361, 526, 80, 1.45, 0, m.panel_ink_faded),
            ("right_c", 361, 607, 58, 1.45, 0, m.panel_ink),
            ("bottom_l", 111, 681, 68, 1.45, 90, m.panel_ink),
            ("bottom_r", 310, 681, 68, 1.45, 90, m.panel_ink),
        ]
        for suffix, x, y, length, width, angle, mat in frame_segments:
            self.static.append(make_surface_stroke(
                f"worn_signal_frame_{suffix}", mat, (x, y), length, width,
                0.30, angle))

        # Chips cut into the ink itself. Their dark ragged silhouettes stop the
        # surviving lines from reading like recently drawn vector rectangles.
        for i, (x, y, rx, ry, angle) in enumerate((
                (91, 401, 5.5, 1.2, -7), (165, 401, 7.5, 1.1, 3),
                (222, 401, 4.5, 1.3, -4), (343, 401, 5.0, 1.2, 6),
                (59, 463, 1.2, 6.0, -3), (59, 530, 1.2, 7.0, 4),
                (59, 637, 1.1, 5.5, -5), (361, 470, 1.2, 5.5, 5),
                (361, 545, 1.2, 7.0, -4), (361, 612, 1.2, 5.0, 3),
                (108, 681, 7.0, 1.2, 2), (312, 681, 5.0, 1.2, 4))):
            self.static.append(make_irregular_patch(
                f"signal_frame_ink_loss_{i}", m.ink_loss, (x, y), rx, ry,
                0.43, 600 + i, points=8, angle_deg=angle))

        # Paint damage is a three-layer event, never a silver sticker: raised dark
        # coating lip, rusty primer/oxide, then a smaller aluminium core. Wear is
        # concentrated at perimeter impacts, plug insertion zones and the shoe end.
        def layered_top_chip(name, x, y, rx, ry, angle, seed):
            self.static.append(make_irregular_patch(
                name + "_paint_lip", m.damage_lip, (x, y), rx * 1.24, ry * 1.24,
                0.16, seed, points=12, angle_deg=angle))
            self.static.append(make_irregular_patch(
                name + "_primer", m.damage_primer, (x, y), rx, ry,
                0.22, seed + 101, points=11, angle_deg=angle))
            offset = (math.cos(math.radians(angle)) * 0.35,
                      math.sin(math.radians(angle)) * 0.35)
            self.static.append(make_irregular_patch(
                name + "_metal", m.exposed_metal, (x + offset[0], y + offset[1]),
                rx * 0.56, ry * 0.58, 0.28, seed + 211,
                points=9, angle_deg=angle + 7.0))

        chip_sites = [
            # left/right perimeter, including repeated cable-plug handling at y367
            ("edge_l_top", 40.8, 151, 2.4, 7.5, 2),
            ("edge_l_jack", 40.6, 367, 3.2, 12.0, -3),
            ("edge_l_low", 41.2, 646, 2.6, 9.0, 2),
            ("edge_r_top", 380.2, 179, 2.5, 7.0, -2),
            ("edge_r_jack", 380.4, 367, 3.1, 11.5, 3),
            ("edge_r_low", 379.8, 625, 2.8, 10.0, -3),
            # top/bottom impacts and shoe contact around the treadle end
            ("edge_t_left", 123, 62.3, 8.0, 2.4, 4),
            ("edge_t_right", 316, 62.2, 7.0, 2.3, -5),
            ("edge_b_left", 83, 699.0, 10.5, 2.8, 2),
            ("edge_b_heel", 189, 699.2, 7.0, 2.4, -3),
            ("edge_b_right", 336, 698.8, 9.0, 2.7, 4),
        ]
        for i, (name, x, y, rx, ry, angle) in enumerate(chip_sites):
            layered_top_chip(name, x, y, rx, ry, angle, 8080 + i * 17)

        # A scratch is a dark recessed groove with a narrow aluminium shoulder on
        # the light-facing edge. These are located where tools, plugs or shoes can
        # actually reach; the previous evenly scattered decorative lines are gone.
        def groove(name, x, y, length, angle, seed):
            self.static.append(make_surface_stroke(
                name + "_recess", m.scratch_groove, (x, y), length, 1.20,
                0.17, angle))
            rad = math.radians(angle)
            hx = x + math.cos(rad) * 0.38
            hy = y + math.sin(rad) * 0.38
            self.static.append(make_surface_stroke(
                name + "_metal_edge", m.exposed_metal, (hx, hy),
                length * 0.86, 0.34, 0.25, angle))

        for i, (name, x, y, length, angle) in enumerate((
                ("jack_tool_l", 52, 370, 20, -64),
                ("jack_tool_r", 368, 372, 18, 57),
                ("footswitch_shoe_l", 146, 648, 24, -58),
                ("footswitch_shoe_r", 278, 659, 20, 53),
                ("screwdriver_tl", 72, 96, 9, -48),
                ("screwdriver_br", 344, 668, 9, 51))):
            groove(name, x, y, length, angle, 1200 + i)

        # The now-visible vertical front wall gets its own layered impacts. These
        # occupy the X/Z plane and therefore catch a different highlight than lid
        # damage—the cue missing from the previous flat render.
        front_y = PLATE[1] + PLATE[3] + 17.0
        for i, (name, x, z, rx, rz, angle) in enumerate((
                ("front_drop_l", 76, -34, 12, 4.0, 8),
                ("front_drag_mid", 174, -52, 18, 3.4, -4),
                ("front_drop_r", 302, -31, 10, 4.2, -8),
                ("front_corner_r", 358, -78, 8, 3.6, 12))):
            self.static.append(make_front_wall_patch(
                name + "_lip", m.damage_lip, x, z, rx * 1.2, rz * 1.25,
                front_y, 15000 + i, angle_deg=angle, outset=0.16))
            self.static.append(make_front_wall_patch(
                name + "_primer", m.damage_primer, x, z, rx, rz,
                front_y, 15100 + i, angle_deg=angle, outset=0.22))
            self.static.append(make_front_wall_patch(
                name + "_metal", m.exposed_metal, x + 0.4, z + 0.2,
                rx * 0.56, rz * 0.58, front_y, 15200 + i,
                angle_deg=angle + 5, outset=0.28))

    def _build_badge_and_preset(self):
        m = self.m
        badge_solids, badge_top = make_modelled_logo(m, LOGO)
        preset_solids, preset_top = make_modelled_preset_hardware(m)
        self.static += badge_solids + preset_solids

        # Raised lettering gets sparse contact wear on its own top plane. These
        # small marks sit on the N/k/F/X reach zones; the earlier bitmap-wide
        # print-loss slashes have been removed because the letters now have real
        # bevels and should age as individual objects.
        for i, (x, y, rx, ry, angle, material) in enumerate((
                (112, 84, 2.8, 0.55, -7, m.exposed_metal),
                (224, 107, 2.3, 0.48, 5, m.exposed_metal),
                (280, 83, 2.0, 0.45, -4, m.ink_loss),
                (320, 108, 2.6, 0.48, 7, m.ink_loss))):
            self.static.append(make_irregular_patch(
                f"badge_letter_contact_wear_{i}", material, (x, y), rx, ry,
                badge_top + 0.10, 1800 + i, points=9, angle_deg=angle))
        # Handling wear now lands on the actual cap/rim top rather than inside
        # the old texture slabs. Selector wear stays at its two reach corners;
        # button wear sits beside, never across, the icon or legend.
        for i, (x, y, rx, ry, angle, z) in enumerate((
                (81, 135, 2.6, 0.62, 8, 5.02),
                (271, 164, 3.1, 0.72, -5, 5.02),
                (299, 141, 1.4, 0.52, 7, preset_top + 0.08),
                (354, 159, 1.5, 0.50, -9, preset_top + 0.08))):
            self.static.append(make_irregular_patch(
                f"preset_edge_rub_{i}", m.old_scratch, (x, y), rx, ry,
                z, 1900 + i, points=8, angle_deg=angle))

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

    @staticmethod
    def _large_knob_profile(R, H):
        """Low 19 mm-class pointer knob: skirt, vertical grip wall, flat top."""
        return [
            (0.0, R * 0.94),
            (H * 0.08, R),          # low skirt lip over the mounting washer
            (H * 0.18, R),
            (H * 0.24, R * 0.855),  # hard ledge into the pinchable barrel
            (H * 0.79, R * 0.855),  # genuinely vertical ribbed grip wall
            (H * 0.91, R * 0.690),  # short hard shoulder into the top insert
            (H, R * 0.640),
            (H, 0.0),               # absolutely flat top face
        ], {3, 4}

    @staticmethod
    def _small_knob_profile(R, H):
        """Tall 12-13 mm-class barrel knob with a long pinchable sidewall."""
        return [
            (0.0, R * 0.92),
            (H * 0.10, R * 0.96),
            (H * 0.18, R * 0.82),
            (H * 0.84, R * 0.82),   # Davies-style tall cylindrical grip band
            (H * 0.94, R * 0.68),
            (H, R * 0.62),
            (H, 0.0),
        ], {2, 3}

    def _make_knob(self, name, centre_px, frame_diameter, style):
        m = self.m
        is_large = style == "large"
        # Davies-style pedal controls are tall enough to pinch: a 1510-class part
        # is about 19 mm across by 14.5 mm high. The crop limits our diameter, but
        # the ratio and grip height now remain hardware-like instead of the old
        # 11 x 4 mm pancake.
        R = frame_diameter * (0.405 if is_large else 0.340)
        H = frame_diameter * (0.550 if is_large else 0.520)
        profile, flute_st = (self._large_knob_profile(R, H) if is_large
                             else self._small_knob_profile(R, H))
        flutes = 24 if is_large else 20
        flute_depth = R * (0.045 if is_large else 0.040)
        body = make_lathe(name, m.knob, profile, px(*centre_px),
                          flutes=flutes, flute_depth=flute_depth,
                          flute_stations=flute_st)
        # Background-pass occluder: same knob without flutes, at the flute-dip
        # radius. The baked contact darkening must stay INSIDE the overlay's
        # silhouette at every rotation; a fluted occluder's scallop tips would
        # peek out from under the filmstrip at other angles.
        # 0.6px inside the flute-dip radius: the baked dark core must never
        # peek past the strip's silhouette, even by an antialiased subpixel.
        dip = [(z, max(0.0, r - flute_depth - 0.6)) for z, r in profile]
        proxy = make_lathe(name + "_proxy", m.knob, dip, px(*centre_px))
        proxy.hide_render = True
        self.proxies.append(proxy)
        top_r = R * (0.60 if is_large else 0.58)
        recess = make_cylinder(name + "_top_recess", m.knob_recess, top_r * 1.075,
                               H * 1.002, H * 1.014, (0.0, 0.0, 0.0), verts=128)
        recess.parent = body
        top = make_cylinder(name + "_flat_top", m.knob_top, top_r,
                            H * 1.015, H * 1.030, (0.0, 0.0, 0.0), verts=128)
        top.parent = body
        # The control is one moulded part. A long ivory paint-filled index makes
        # orientation readable without turning the top into a reflective dome.
        bar = make_box(name + "_ptr", m.pointer,
                       (max(1.75, R * 0.065), R * 0.48, 0.34),
                       (0.0, R * 0.31, H * 1.050))
        bar.parent = body

        # Finger and pick marks on the planar cap. These are not a uniform noise
        # layer: a handful of directional scars reads like a touched control.
        rng = random.Random(sum(ord(ch) for ch in name) + (808 if is_large else 404))
        scuffs = []
        for si in range(7 if is_large else 5):
            a = rng.uniform(0.0, math.tau)
            radial = rng.uniform(0.05, 0.64) * top_r
            sx, sy = math.cos(a) * radial, math.sin(a) * radial
            scratch = make_box(
                f"{name}_top_scratch_{si}", m.knob_scuff,
                (rng.uniform(0.34, 0.70), rng.uniform(R * 0.13, R * 0.34), 0.08),
                (sx, sy, H * 1.043),
                rot=(0.0, 0.0, rng.uniform(-1.30, 1.30)))
            scratch.parent = body
            scuffs.append(scratch)

        # Small missing sections keep the painted index from being immaculate.
        for ci, yoff in enumerate((R * 0.19, R * 0.38)):
            chip = make_box(f"{name}_ptr_chip_{ci}", m.knob_recess,
                            (max(2.0, R * 0.078), R * 0.055, 0.08),
                            (0.0, yoff, H * 1.064))
            chip.parent = body
            scuffs.append(chip)

        # Davies-style set screw: a real fastener set radially into the grip band,
        # with a recessed slot. It turns with the knob and catches a moving chrome
        # glint while the room reflection stays fixed across the filmstrip.
        setscrew = make_axis_cylinder(name + "_set_screw", m.chrome, 1.55, 4.2,
                                      (R * 0.84, 0.0, H * 0.55),
                                      axis="x", verts=48)
        setscrew.parent = body
        setslot = make_box(name + "_set_screw_slot", m.slot,
                           (0.16, 2.15, 0.48),
                           (R * 0.905, 0.0, H * 0.55),
                           rot=(0.0, 0.0, math.radians(18.0)))
        setslot.parent = body
        scuffs += [setscrew, setslot]
        return [body, recess, top, bar] + scuffs

    def _build_knob_scale(self, name, centre_px, frame_diameter, style):
        """Sparse screen-printed index marks around a physical pot.

        These are deliberately not a continuous software arc. Seven/nine
        discrete marks are common on pedals, survive 1x display, and make the
        knob look mounted to the plate instead of pasted over it.
        """
        is_large = style == "large"
        R = frame_diameter * (0.405 if is_large else 0.340)
        count = 9 if is_large else 7
        mark_radius = R + (5.8 if is_large else 4.4)
        # A narrow exposed nickel shoulder is stationary hardware, not part of
        # the rotating filmstrip. It seats the black control visibly on the
        # black enclosure and gives the fingers a clear lower boundary.
        mount = make_cylinder(name + "_mount", self.m.knob_mount, R * 1.075,
                              0.04, 0.88, px(*centre_px), verts=128)
        marks = [mount]
        for i in range(count):
            t = i / (count - 1)
            angle = math.radians(SWEEP_START_DEG + SWEEP_DEG * t)
            x = centre_px[0] + math.sin(angle) * mark_radius
            y = centre_px[1] - math.cos(angle) * mark_radius
            is_cardinal = i in (0, count // 2, count - 1)
            length = (4.8 if is_large else 3.9) if is_cardinal else (3.35 if is_large else 2.8)
            wear_code = (sum(ord(ch) for ch in name) + i * 3) % 8
            ink = self.m.panel_ink_faded if wear_code in (0, 3, 6) else self.m.panel_ink
            if wear_code == 0:
                length *= 0.42
            elif wear_code in (3, 5):
                length *= 0.68
            mark = make_box(f"{name}_scale_{i}", ink,
                            (1.10, length, 0.18), px(x, y, 0.23),
                            rot=(0.0, 0.0, -angle))
            marks.append(mark)
        return marks

    def _build_knobs(self):
        self.knobs["large"] = []
        self.knobs["small"] = []
        for i, c in enumerate(KNOBS_LARGE):
            self.knobs["large"].append(
                self._make_knob(f"knob_large_{i}", c, KNOB_LARGE_D, "large"))
            self.static += self._build_knob_scale(f"knob_large_{i}", c,
                                                   KNOB_LARGE_D, "large")
        for i, c in enumerate(KNOBS_SMALL):
            self.knobs["small"].append(
                self._make_knob(f"knob_small_{i}", c, KNOB_SMALL_D, "small"))
            self.static += self._build_knob_scale(f"knob_small_{i}", c,
                                                   KNOB_SMALL_D, "small")
        # Rest pose: value 0 -> pointer at SWEEP_START_DEG clockwise from 12.
        for pair in self.knobs["large"] + self.knobs["small"]:
            pair[0].rotation_euler.z = -math.radians(SWEEP_START_DEG)

    # -- gate toggle ----------------------------------------------------------

    def _build_gate(self):
        m = self.m
        cx, cy = GATE_CENTRE
        base = px(cx, cy)
        shared = [
            # Outside-in mounting order: recessed panel bore, insulating
            # gasket, lock washer, bevelled hex nut, threaded bushing and pivot.
            make_cylinder("gate_panel_bore", m.preset_gasket, 20.0, -0.55, 0.22, base),
            make_cylinder("gate_black_gasket", m.gate_phenolic, 18.7, 0.02, 0.72, base),
            make_cylinder("gate_lock_washer", m.gate_nickel, 17.3, 0.48, 1.35, base),
        ]

        hex_points = [
            (cx + 15.2 * math.cos(math.radians(30.0 + i * 60.0)),
             cy + 15.2 * math.sin(math.radians(30.0 + i * 60.0)))
            for i in range(6)
        ]
        shared.append(make_polygon_prism(
            "gate_bevelled_hex_nut", m.gate_nickel, hex_points,
            1.05, 5.85, bevel=0.72))

        # Tiny dark cuts in the exposed lock-washer rim hint at serration
        # without turning the part into an oversized gear at 1x.
        for i in range(8):
            angle = math.radians(i * 45.0)
            sx = cx + math.sin(angle) * 16.4
            sy = cy - math.cos(angle) * 16.4
            shared.append(make_box(
                f"gate_lock_serration_{i}", m.slot, (0.52, 2.1, 0.16),
                px(sx, sy, 1.48), rot=(0.0, 0.0, -angle)))

        shared.append(make_cylinder(
            "gate_threaded_bushing", m.gate_steel, 8.2, 5.25, 9.85, base, verts=80))
        for i, z in enumerate((6.05, 7.18, 8.31)):
            shared.append(make_cylinder(
                f"gate_thread_ring_{i}", m.gate_nickel, 8.65, z, z + 0.30,
                base, verts=80))
        shared.append(make_cylinder(
            "gate_pivot_collar", m.gate_steel, 6.9, 9.55, 12.35, base, verts=80))
        shared.append(make_sphere(
            "gate_pivot_cap", m.gate_nickel, 6.35, px(cx, cy, 12.0), squash=0.56))

        # Handling/oxidation lives on the actual nut top. Sparse marks keep the
        # age consistent with the chassis without making the switch filthy.
        for i, (x, y, rx, ry, angle) in enumerate((
                (cx - 8.2, cy - 5.5, 2.1, 0.60, -12),
                (cx + 7.3, cy + 5.0, 1.8, 0.52, 8))):
            shared.append(make_irregular_patch(
                f"gate_nut_tarnish_{i}", m.gate_tarnish, (x, y), rx, ry,
                5.98, 2400 + i, points=9, angle_deg=angle))

        def lever(state, tilt_deg):
            objs = []
            L = 25.0
            bm = bmesh.new()
            bmesh.ops.create_cone(bm, cap_ends=True, segments=64,
                                  radius1=3.15, radius2=2.65, depth=L)
            bmesh.ops.translate(bm, verts=bm.verts[:], vec=(0, 0, L / 2 + 3.0))
            mesh = bpy.data.meshes.new(f"gate_lever_{state}")
            bm.to_mesh(mesh)
            bm.free()
            rod = link_object(f"gate_lever_{state}", mesh, m.gate_nickel)
            rod.location = px(cx, cy, 11.0)
            tilt = math.radians(tilt_deg)
            rod.rotation_euler = (tilt, 0, 0)
            shade_smooth(rod)
            objs.append(rod)
            # Tip of the local +Z axis after the X tilt, in world space.
            lz = L + 3.0
            tip = Vector(px(cx, cy, 11.0)) + Vector((0.0, -lz * math.sin(tilt),
                                                     lz * math.cos(tilt)))

            # A finger meets warm, lightly grained phenolic—not a chrome ball.
            # The thin orange band is a restrained SendBloom identity cue.
            cap_profile = [
                (-3.0, 0.0), (-3.0, 3.35), (-2.1, 4.10),
                (2.1, 4.10), (3.0, 3.35), (3.0, 0.0),
            ]
            cap = make_lathe(f"gate_phenolic_tip_{state}", m.gate_phenolic,
                             cap_profile, (0.0, 0.0, 0.0), steps=96)
            cap.location = tuple(tip)
            cap.rotation_euler = (tilt, 0.0, 0.0)
            band = make_cylinder(f"gate_orange_band_{state}", m.badge_orange,
                                 4.16, -2.0, -1.25, (0.0, 0.0, 0.0), verts=96)
            band.location = tuple(tip)
            band.rotation_euler = (tilt, 0.0, 0.0)
            objs += [cap, band]
            return objs

        # PRE = lever up (screen up = blender +y => negative x-rotation)
        self.assemblies["gate_pre"] = shared + lever("pre", -42.0)
        self.assemblies["gate_post"] = shared + lever("post", 42.0)

    # -- dark-mode button -------------------------------------------------------

    def _build_dark(self):
        m = self.m
        cx, cy = DARK_CENTRE
        trim = make_prism("dark_chrome_trim", m.nickel, 72, 72, 15, 0, 2.2,
                          bevel=1.4, centre=px(cx, cy)[:2])
        housing = make_prism("dark_housing", m.button_black, 68, 68, 14, 0, 6.5,
                             bevel=1.6, centre=px(cx, cy)[:2],
                             well=(4.5, 4.5))

        def cap(state, size, z0, z1, text_z):
            capobj = make_prism(f"dark_cap_{state}", m.button_black, size, size,
                                11, z0, z1, bevel=2.6, centre=px(cx, cy)[:2])
            text = make_text(f"dark_text_{state}", m.button_text, "DARK\nMODE",
                             px(cx, cy + 1.0, text_z), size=15.5, extrude=0.7)
            return [capobj, text]

        self.assemblies["dark_off"] = [trim, housing] + cap("off", 60, 3.0, 19.0, 18.6)
        self.assemblies["dark_on"] = [trim, housing] + cap("on", 54, 1.5, 8.5, 8.1)

    # -- footswitch --------------------------------------------------------------

    def _build_foot(self):
        m = self.m
        cx, cy = FOOT_CENTRE
        # A pressure treadle still needs manufactured hardware: plated carrier,
        # black isolating tray, full-width hinge pin and retained pivot bolts.
        carrier = make_prism("foot_chrome_carrier", m.chrome, 112, 150, 13, 0, 5.0,
                             bevel=2.0, centre=px(cx, cy)[:2])
        tray = make_prism("foot_black_tray", m.chassis, 104, 142, 11, 4.8, 7.5,
                          bevel=1.6, centre=px(cx, cy)[:2])
        hinge_y_editor = cy + 66.0  # screen-bottom edge of the treadle
        hinge = make_axis_cylinder("foot_hinge_pin", m.chrome, 4.2, 104.0,
                                   px(cx, hinge_y_editor, 10.0), axis="x", verts=96)
        shared = [carrier, tray, hinge]
        for bi, bx in enumerate((cx - 51.0, cx + 51.0)):
            bolt = make_sphere(f"foot_pivot_bolt_{bi}", m.nickel, 5.0,
                               px(bx, hinge_y_editor, 10.0), squash=0.55)
            slot = make_box(f"foot_pivot_slot_{bi}", m.slot, (7.0, 1.0, 0.55),
                            px(bx, hinge_y_editor, 12.9),
                            rot=(0.0, 0.0, math.radians(-18 if bi else 24)))
            shared += [bolt, slot]

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
            detail = []
            # Shallow moulded traction ribs are part of the object design, while
            # three diagonal sole marks cluster in the actual contact zone.
            for ri, yoff in enumerate((24, 44, 64, 84, 104)):
                rib = make_box(f"foot_{state}_traction_{ri}", m.rubber_rib,
                               (72.0, 1.35, 0.32), (0.0, yoff, 12.12))
                rib.parent = pad
                detail.append(rib)
            for si, (xoff, yoff, length, angle) in enumerate((
                    (-18, 45, 27, -56), (7, 70, 37, -68),
                    (22, 91, 21, 48))):
                stroke = make_box(f"foot_{state}_scuff_{si}", m.rubber_scuff,
                                  (0.85 if si != 1 else 1.25, length, 0.10),
                                  (xoff, yoff, 12.16),
                                  rot=(0.0, 0.0, math.radians(angle)))
                stroke.parent = pad
                detail.append(stroke)
            return [pad] + detail

        self.assemblies["foot_up"] = shared + treadle("up", 10.0)
        self.assemblies["foot_down"] = shared + treadle("down", 3.0)

    # -- clip lens -----------------------------------------------------------------

    def _build_clip(self):
        m = self.m
        cx, cy = CLIP_CENTRE
        # A panel LED sits in a plated round bezel and black insulating gasket;
        # the previous square black tile was software furniture, not hardware.
        bezel = make_cylinder("clip_chrome_bezel", m.chrome, 15.0, 0.0, 3.0,
                              px(cx, cy), verts=96)
        gasket = make_cylinder("clip_black_gasket", m.button_black, 11.8, 2.9, 4.8,
                               px(cx, cy), verts=96)
        dome_off = make_sphere("clip_dome_off", m.lens_off, 11.0,
                               px(cx, cy, 4.0), squash=0.55)
        dome_on = make_sphere("clip_dome_on", m.lens_on, 11.0,
                              px(cx, cy, 4.0), squash=0.55)
        self.assemblies["clip_off"] = [bezel, gasket, dome_off]
        self.assemblies["clip_on"] = [bezel, gasket, dome_on]

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


def pass_knob_strips(scn, pedal, resume=False, preview=False):
    """One knob of each size, rendered alone with alpha, one frame per pointer
    angle, stitched into a vertical strip (frame 0 at the top)."""
    for size_key, diameter in (("large", KNOB_LARGE_D),
                               ("small", KNOB_SMALL_D)):
        restore_all(pedal)
        show_only_states(pedal, set())
        hero = pedal.knobs[size_key][0]
        body = hero[0]

        # Park the hero knob at the plate's horizontal centre at its row's
        # height so the fill gradient it bakes is the row average.
        original = tuple(body.location)
        row_editor_y = KNOBS_LARGE[0][1] if size_key == "large" else KNOBS_SMALL[0][1]
        body.location = (EDITOR_W / 2, pitch_compensated_world_y(row_editor_y), 0.0)

        for obj in bpy.context.scene.objects:
            if obj.type in {"MESH", "CURVE", "FONT"}:
                obj.visible_camera = obj in hero
        for other_pair in pedal.knobs["large"] + pedal.knobs["small"]:
            for obj in other_pair:
                if obj not in hero:
                    obj.hide_render = True

        scn.render.film_transparent = True
        frame_px = int(diameter)
        rect = (EDITOR_W / 2 - diameter / 2, row_editor_y - diameter / 2,
                diameter, diameter)
        set_border(scn, rect)

        frame_dir = TMP / ("preview" if preview else "production")
        frame_paths = []
        for i in range(KNOB_FRAMES):
            t = i / (KNOB_FRAMES - 1)
            angle = SWEEP_START_DEG + SWEEP_DEG * t
            body.rotation_euler.z = -math.radians(angle)
            path = frame_dir / f"{size_key}_{i:03d}.png"
            can_reuse = False
            if resume and path.exists():
                try:
                    cached = bpy.data.images.load(str(path))
                    can_reuse = tuple(cached.size) == (frame_px * SCALE, frame_px * SCALE)
                    bpy.data.images.remove(cached)
                except Exception:
                    can_reuse = False
            if can_reuse:
                print(f"[render_ui] resume {path.name}")
            else:
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
    resume = "--resume" in argv

    t0 = time.time()
    scn = reset_scene()
    if preview:
        scn.cycles.samples = PREVIEW_SAMPLES
    add_camera(scn)
    add_lights(scn)
    mats = Mats()
    pedal = Pedal(mats)
    compensate_scene_for_camera_pitch(scn)
    print(f"[render_ui] scene built in {time.time() - t0:.1f}s")

    if targets == ["all"] or "background" in targets:
        pass_background(scn, pedal)
    if targets == ["all"] or "knobs" in targets:
        pass_knob_strips(scn, pedal, resume=resume, preview=preview)
    if targets == ["all"] or "states" in targets:
        pass_states(scn, pedal)

    print(f"[render_ui] total {time.time() - t0:.1f}s")


if __name__ == "__main__":
    main()
