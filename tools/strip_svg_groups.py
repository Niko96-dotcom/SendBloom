#!/usr/bin/env python3
"""Remove whole <g> subtrees from an SVG by id.

    strip_svg_groups.py <in.svg> <out.svg> <group-id>...

The asset generator emits semantic groups (`...-external-label`, `...-lettering`),
so lettering that the plugin draws itself can be lifted out cleanly rather than
masked over. Removes each named group and everything inside it, matching <g>
nesting rather than trusting the first closing tag.
"""

import re
import sys

OPEN_G = re.compile(r"<g\b")
CLOSE_G = re.compile(r"</g\s*>")
SELF_CLOSING = re.compile(r"/>\s*$")


def find_group_span(text, group_id):
    """Return (start, end) of the <g id="group_id"> subtree, or None."""
    m = re.search(r'<g\b[^>]*\bid="' + re.escape(group_id) + r'"', text)
    if not m:
        return None

    start = m.start()
    tag_end = text.index(">", m.start())

    # An empty group may be written <g id="x" ... />
    if text[tag_end - 1] == "/":
        return start, tag_end + 1

    depth = 1
    pos = tag_end + 1
    while depth:
        nxt_open = OPEN_G.search(text, pos)
        nxt_close = CLOSE_G.search(text, pos)
        if not nxt_close:
            raise ValueError(f"unbalanced <g> while scanning {group_id!r}")
        if nxt_open and nxt_open.start() < nxt_close.start():
            open_tag_end = text.index(">", nxt_open.start())
            if text[open_tag_end - 1] != "/":
                depth += 1
            pos = open_tag_end + 1
        else:
            depth -= 1
            pos = nxt_close.end()
    return start, pos


def main():
    if len(sys.argv) < 4:
        sys.exit(__doc__)

    src, dst, ids = sys.argv[1], sys.argv[2], sys.argv[3:]
    text = open(src).read()

    removed = []
    for gid in ids:
        span = find_group_span(text, gid)
        if span is None:
            print(f"  ! {gid}: not found", file=sys.stderr)
            continue
        text = text[: span[0]] + text[span[1] :]
        removed.append(gid)

    open(dst, "w").write(text)
    print(f"{dst}: removed {', '.join(removed) if removed else 'nothing'}")


if __name__ == "__main__":
    main()
