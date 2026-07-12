#!/usr/bin/env python3
"""Reproducibly extract the supplied SendBloom hardware sheets into alpha PNGs."""

from __future__ import annotations

import math
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageOps


ROOT = Path(__file__).resolve().parents[1]
SOURCE = Path("/Users/nikolaymohr/Dropbox/UI SENDBLOOM")
OUTPUT = ROOT / "resources" / "ui"


def crop(source: str, box: tuple[int, int, int, int]) -> Image.Image:
    return Image.open(SOURCE / source).convert("RGB").crop(box)


def save_rgba(image: Image.Image, alpha: Image.Image, name: str) -> None:
    image = image.convert("RGBA")
    image.putalpha(alpha)
    bbox = alpha.getbbox()
    if bbox is not None:
        image = image.crop(tuple(max(0, value - 3) if i < 2 else min(image.size[i - 2], value + 3)
                                 for i, value in enumerate(bbox)))
    image.save(OUTPUT / name, optimize=True)


def soft_ellipse(size: tuple[int, int], inset: int = 8, blur: float = 2.0) -> Image.Image:
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).ellipse((inset, inset, size[0] - inset - 1, size[1] - inset - 1), fill=255)
    return mask.filter(ImageFilter.GaussianBlur(blur))


def soft_circle(size: tuple[int, int], inset: int = 5, y_bias: int = 0, blur: float = 1.2) -> Image.Image:
    diameter = min(size) - inset * 2
    left = (size[0] - diameter) // 2
    top = (size[1] - diameter) // 2 + y_bias
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).ellipse((left, top, left + diameter, top + diameter), fill=255)
    return mask.filter(ImageFilter.GaussianBlur(blur))


def dark_subject_alpha(image: Image.Image, low: int, high: int) -> Image.Image:
    """Matte a dark/metal subject from a light studio field without altering RGB pixels."""
    grey = ImageOps.grayscale(image)
    lut = [255 if v <= low else 0 if v >= high else round(255 * (high - v) / (high - low))
           for v in range(256)]
    alpha = grey.point(lut)
    # Close tiny holes in specular metal, then soften only the outer antialiasing fringe.
    alpha = alpha.filter(ImageFilter.MaxFilter(5)).filter(ImageFilter.MinFilter(3))
    return alpha.filter(ImageFilter.GaussianBlur(0.65))


def rounded_mask(size: tuple[int, int], inset: int, radius: int, blur: float = 1.5) -> Image.Image:
    mask = Image.new("L", size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (inset, inset, size[0] - inset - 1, size[1] - inset - 1),
        radius=radius,
        fill=255,
    )
    return mask.filter(ImageFilter.GaussianBlur(blur))


def extract_background() -> None:
    shell = Image.open(SOURCE / "shell.png").convert("RGBA")
    texture = Image.open(SOURCE / "back plate.png").convert("RGB")
    # The photographed inset is x=129..664, y=140..1112. Preserve its rounded lip and
    # replace only the inner field, leaving a small margin so no compositing seam reaches it.
    field = (145, 157, 648, 1093)
    panel = ImageOps.fit(texture, (field[2] - field[0], field[3] - field[1]), method=Image.Resampling.LANCZOS)
    # Remove the small generator sparkle baked into the supplied texture by cloning a
    # neighbouring patch of the same leather grain with a feathered blend.
    patch = panel.crop((238, 812, 358, 922))
    clone_mask = rounded_mask(patch.size, 10, 24, 16.0)
    panel.paste(patch, (373, 802), clone_mask)
    mask = rounded_mask(panel.size, 0, 27, 2.0)
    shell.alpha_composite(Image.merge("RGBA", (*panel.split(), mask)), (field[0], field[1]))
    shell.save(OUTPUT / "pedal_background.png", optimize=True)


def extract_knobs() -> None:
    boxes = [
        (42, 246, 307, 523),
        (311, 245, 566, 523),
        (570, 246, 824, 522),
        (835, 244, 1087, 525),
        (1093, 242, 1345, 526),
    ]
    names = ["knob_input.png", "knob_size.png", "knob_level.png", "knob_distortion.png", "knob_output.png"]
    for box, name in zip(boxes, names):
        image = crop("buttons.png", box)
        # Intersect the circular silhouette with a luminance matte to exclude the white
        # studio field between the irregular worn rim and its analytic boundary.
        alpha = ImageChops.multiply(soft_circle(image.size, inset=5, y_bias=-7, blur=1.1),
                                    dark_subject_alpha(image, 178, 218))
        save_rgba(image, alpha, name)


def extract_dark_states() -> None:
    for box, name in [
        ((87, 104, 594, 621), "dark_off.png"),
        ((783, 104, 1269, 621), "dark_on.png"),
    ]:
        image = crop("dark mode image.png", box)
        save_rgba(image, dark_subject_alpha(image, 70, 208), name)


def extract_gate_states() -> None:
    for box, name, is_post in [
        ((355, 226, 641, 527), "gate_pre.png", False),
        ((735, 244, 1027, 576), "gate_post.png", True),
    ]:
        image = crop("flip switchimage.png", box)
        edge = dark_subject_alpha(image, 192, 229)
        outer = Image.new("L", image.size, 0)
        core = Image.new("L", image.size, 0)
        od = ImageDraw.Draw(outer)
        cd = ImageDraw.Draw(core)
        if not is_post:
            od.ellipse((7, 35, image.width - 7, image.height - 5), fill=255)
            od.rounded_rectangle((111, 0, 177, 176), radius=28, fill=255)
            cd.ellipse((19, 48, image.width - 19, image.height - 18), fill=255)
            cd.rounded_rectangle((121, 0, 167, 167), radius=22, fill=255)
        else:
            od.ellipse((8, 0, image.width - 8, 276), fill=255)
            od.line((145, 134, 165, 326), fill=255, width=74)
            cd.ellipse((20, 12, image.width - 20, 263), fill=255)
            cd.line((145, 138, 165, 326), fill=255, width=47)
        outer = outer.filter(ImageFilter.GaussianBlur(1.1))
        core = core.filter(ImageFilter.GaussianBlur(0.8))
        alpha = ImageChops.lighter(ImageChops.multiply(edge, outer), core)
        save_rgba(image, alpha, name)


def extract_footswitch_states() -> None:
    source = Image.open(SOURCE / "footpreassure button.png").convert("RGB")
    for box, name in [
        ((50, 91, 643, 697), "footswitch_up.png"),
        ((731, 93, 1322, 698), "footswitch_down.png"),
    ]:
        image = source.crop(box)
        alpha = soft_circle(image.size, inset=4, y_bias=-1, blur=1.1)
        save_rgba(image, alpha, name)


def extract_logo_and_presets() -> None:
    logo = crop("LOGO.png", (59, 139, 1980, 521))
    logo_alpha = ImageChops.multiply(rounded_mask(logo.size, 0, 72, 1.2),
                                     dark_subject_alpha(logo, 195, 230))
    save_rgba(logo, logo_alpha, "brand_logo.png")

    # The selector sheet bakes a sample preset name underneath its frame. Build the live
    # selector from the supplied plate material at the exact frame proportions, avoiding
    # any covered-but-still-present glyph pixels at the antialiased lower seam.
    preset = ImageOps.fit(Image.open(SOURCE / "back plate.png").convert("RGB"),
                          (1354, 186), method=Image.Resampling.LANCZOS)
    draw = ImageDraw.Draw(preset)
    draw.rounded_rectangle((2, 2, preset.width - 3, preset.height - 3),
                           radius=34, outline=(118, 92, 68), width=6)
    draw.rounded_rectangle((28, 17, preset.width - 29, preset.height - 18),
                           radius=25, outline=(30, 28, 25), width=8)
    arrow_x = preset.width - 91
    draw.polygon(((arrow_x - 15, 63), (arrow_x + 15, 63), (arrow_x, 89)), fill=(35, 32, 28))
    save_rgba(preset, rounded_mask(preset.size, 0, 32, 1.1), "preset_field.png")
    # Full plate crops — the sheet bakes a checkerboard around each plate, so the
    # rounded mask must sit on the plate edge itself, not the crop edge.
    load = crop("PRESET ASSETS.png", (1717, 196, 1914, 412))
    save_rgba(load, rounded_mask(load.size, 3, 34, 1.1), "preset_load.png")
    save = crop("PRESET ASSETS.png", (1948, 196, 2126, 412))
    save_rgba(save, rounded_mask(save.size, 3, 34, 1.1), "preset_save.png")


def extract_clip_states() -> None:
    # The clip sheet photographs the same square LED lens dark (leftmost) and lit
    # (third). Crop each lens on its physical edge; the editor paints the halo.
    for box, name in [
        ((133, 262, 357, 508), "clip_off.png"),
        ((719, 262, 947, 512), "clip_on.png"),
    ]:
        image = crop("clip.png", box)
        save_rgba(image, rounded_mask(image.size, 3, 52, 1.2), name)


def main() -> None:
    OUTPUT.mkdir(parents=True, exist_ok=True)
    extract_background()
    extract_knobs()
    extract_dark_states()
    extract_gate_states()
    extract_footswitch_states()
    extract_logo_and_presets()
    extract_clip_states()


if __name__ == "__main__":
    main()
