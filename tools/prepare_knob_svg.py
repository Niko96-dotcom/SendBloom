#!/usr/bin/env python3
"""Turn a delivered knob SVG into one PedalKnob can rotate.

    prepare_knob_svg.py <in.svg> <out.svg> [--mirror]

Four things have to change before the art can spin:

  * Pattern fills go. JUCE has no <pattern> and paints every one solid black —
    on these knobs that is a ~45% wash over the whole cap. The grain they were
    meant to carry never renders either way, so dropping them is pure gain.
  * Groups that must not turn go. PedalKnob rotates the entire image, so a
    fixed plate marker or a cast shadow would orbit with the cap. Lettering
    goes too: the host draws its own captions.
  * The viewBox gets centred on the outer rim. The art draws rim and face on
    slightly different centres (a drawn perspective), so rotating about either
    makes the other orbit. The rim is the silhouette — the edge read against a
    light plate — so centring on it is what keeps the outline still.
  * The indicator gets rotated to 12 o'clock. PedalKnob maps value 0 to the
    start angle and turns the art by that much, which assumes the art points
    straight up at rest. This art does not, so every reading was off by the
    difference.

--mirror flips the art horizontally. These knobs are lit from the upper right;
a plate lit from the upper left cannot seat an object lit from the other side,
and that mismatch is what reads as the knob floating above the panel. Mirroring
flips every baked cue at once — gradients and the hand-drawn highlight arcs —
which editing gradients one by one would not. It costs nothing, because a knob
is radially symmetric apart from the pointer, and the pointer gets rotated back
to vertical anyway.
"""

import math
import re
import sys

# Belongs to the plate or the scene, not to the part that turns.
STATIC_GROUPS = ("outer-stationary-marker", "base-shadow")

# Lettering the host draws itself.
LABEL_GROUPS = ("top-label", "bottom-label")


def drop_group(text, gid):
    """Remove a <g id="gid"> and its whole subtree, respecting <g> nesting."""
    m = re.search(r'<g id="' + re.escape(gid) + r'"[^>]*>', text)
    if not m:
        return text, False

    start, pos, depth = m.start(), m.end(), 1
    while depth:
        o, c = text.find("<g", pos), text.find("</g>", pos)
        if c == -1:
            raise ValueError(f"unbalanced <g> while scanning {gid!r}")
        if o != -1 and o < c:
            pos = text.index(">", o) + 1
            if text[pos - 2] != "/":
                depth += 1
        else:
            depth -= 1
            pos = c + 4
    return text[:start] + text[pos:], True


def strip_pattern_fills(text):
    """Drop elements painted with a <pattern>, then the now-orphaned defs."""
    n = 0
    for pid in set(re.findall(r'<pattern id="([^"]+)"', text)):
        text, hits = re.subn(
            r'<(circle|rect|path|ellipse)\b[^>]*fill="url\(#' + re.escape(pid) + r'\)"[^>]*/>',
            "", text)
        n += hits
    return re.sub(r"<pattern\b.*?</pattern>", "", text, flags=re.S), n


def rim_centre(text):
    """Centre of the outer rim, which is the knob's silhouette."""
    m = re.search(r'<g id="outer-rim">(.*?)</g>', text, flags=re.S)
    if not m:
        return None
    c = re.search(r'<circle cx="([-\d.]+)" cy="([-\d.]+)" r="([-\d.]+)"', m.group(1))
    return (float(c.group(1)), float(c.group(2))) if c else None


def indicator_angle(text, centre):
    """Where the pointer rests, in degrees clockwise from 12 o'clock."""
    m = re.search(r'<g id="indicator-slot[s]?">\s*<path d="([^"]+)"', text)
    if not m:
        return None
    pts = [(float(a), float(b)) for a, b in re.findall(r"([-\d.]+)\s+([-\d.]+)", m.group(1))]
    if not pts:
        return None
    ax = sum(p[0] for p in pts) / len(pts)
    ay = sum(p[1] for p in pts) / len(pts)
    return math.degrees(math.atan2(ax - centre[0], centre[1] - ay))


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    mirror = "--mirror" in sys.argv
    src, dst = args[0], args[1]
    text = open(src).read()

    text, n_patterns = strip_pattern_fills(text)

    removed = []
    for gid in STATIC_GROUPS + LABEL_GROUPS:
        text, ok = drop_group(text, gid)
        if ok:
            removed.append(gid)

    centre = rim_centre(text)
    rest = indicator_angle(text, centre) if centre else None

    # Mirroring negates the pointer's angle; then rotate whatever remains back to
    # vertical so value 0 lands on the start angle PedalKnob expects.
    after_mirror = -rest if (mirror and rest is not None) else rest
    spin = -after_mirror if after_mirror is not None else 0.0

    if centre:
        parts = []
        if abs(spin) > 0.01:
            parts.append(f"rotate({spin:.2f} {centre[0]:g} {centre[1]:g})")
        if mirror:
            parts.append(f"translate({2 * centre[0]:g} 0) scale(-1 1)")
        if parts:
            m = re.search(r'(<g id="[^"]*button"[^>]*?)>', text)
            text = text[: m.end(1)] + f' transform="{" ".join(parts)}"' + text[m.end(1):]

    box = re.search(r'viewBox="([-\d.]+) ([-\d.]+) ([-\d.]+) ([-\d.]+)"', text)
    new = None
    if centre and box:
        w, h = float(box.group(3)), float(box.group(4))
        new = f'viewBox="{centre[0] - w / 2:g} {centre[1] - h / 2:g} {w:g} {h:g}"'
        text = text[: box.start()] + new + text[box.end():]

    open(dst, "w").write(text)
    print(f"{dst}")
    print(f"  pattern-filled elements removed: {n_patterns}")
    print(f"  groups removed: {', '.join(removed) if removed else 'none'}")
    print(f"  rotation axis: rim centre {centre} -> {new or 'viewBox unchanged'}")
    if rest is not None:
        print(f"  pointer rest {rest:+.1f} deg"
              + (f" -> mirrored {after_mirror:+.1f}" if mirror else "")
              + f" -> spun {spin:+.2f} to reach 12 o'clock")
    print(f"  lighting: {'mirrored to upper-left' if mirror else 'left as delivered'}")


if __name__ == "__main__":
    main()
