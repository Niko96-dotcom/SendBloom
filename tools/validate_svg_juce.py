#!/usr/bin/env python3
"""
Validate SVG files against the subset understood by JUCE 8.0.12's SVG parser
(JUCE/modules/juce_gui_basics/drawables/juce_SVGParser.cpp).

Every rule below is derived from reading that parser, not from the SVG spec.
JUCE fails silently: unsupported constructs render as black, or vanish. This
script surfaces what would break before anything is wired into the build.
"""

import re
import sys
from pathlib import Path
from collections import Counter

# Elements parseSubElement()/parsePathElement() actually dispatch on.
SHAPES = {"path", "rect", "circle", "ellipse", "line", "polyline", "polygon", "use"}
CONTAINERS = {"g", "svg", "switch", "a", "defs", "style"}
PAINT = {"linearGradient", "radialGradient", "stop", "clipPath"}
METADATA = {"title", "desc", "metadata"}
SUPPORTED = SHAPES | CONTAINERS | PAINT | METADATA | {"text", "image", "tspan"}

TAG_RE = re.compile(r"<([a-zA-Z][a-zA-Z0-9]*)\b")
ID_RE = re.compile(r'\bid\s*=\s*"([^"]*)"')
HREF_RE = re.compile(r'\b(?:xlink:)?href\s*=\s*"#([^"]*)"')
URLREF_RE = re.compile(r'\b([a-zA-Z-]+)\s*=\s*"url\(#([^)]*)\)"')


class Finding:
    __slots__ = ("sev", "kind", "detail")

    def __init__(self, sev, kind, detail):
        self.sev, self.kind, self.detail = sev, kind, detail


def element_index(text):
    """Map id -> tag name for every element carrying an id.

    First occurrence wins, mirroring JUCE: applyOperationToChildWithID walks
    depth-first and returns on the first id match. Ids are supposed to be
    unique, but when they are not, this is the one JUCE actually resolves to.
    """
    index, duplicates = {}, set()
    for m in re.finditer(r"<([a-zA-Z][a-zA-Z0-9]*)\b([^>]*)>", text):
        tag, attrs = m.group(1), m.group(2)
        mid = ID_RE.search(attrs)
        if mid:
            key = mid.group(1)
            if key in index:
                duplicates.add(key)
            else:
                index[key] = tag
    return index, duplicates


def analyse(path):
    text = path.read_text(errors="replace")
    findings = []
    tags = Counter(TAG_RE.findall(text))
    index, duplicate_ids = element_index(text)

    # Duplicate ids are invalid SVG and render-order-dependent: JUCE takes the
    # first in document order, other renderers may not. Whatever it resolves to
    # today, it is one reordering away from resolving to something else.
    for key in sorted(duplicate_ids):
        findings.append(Finding("WARN", "duplicate-id",
                                f'id="{key}" declared more than once; JUCE binds the first '
                                f"(<{index.get(key)}>) — fragile"))

    # 1. Unsupported elements: parseSubElement() returns nullptr -> dropped.
    #    A <symbol>/<pattern> sitting in <defs> is inert and harmless on its own
    #    — the fault is the <use>/fill that references it, reported below. Only
    #    flag such definitions as INFO so the blocker list stays honest.
    DEFINITIONAL = {"symbol", "pattern", "mask", "marker", "filter"}
    for tag, n in tags.items():
        if tag not in SUPPORTED:
            sev = "INFO" if tag in DEFINITIONAL else "BLOCK"
            note = ("definition only; inert unless referenced"
                    if sev == "INFO" else "not in parser dispatch; renders nothing")
            findings.append(Finding(sev, "unsupported-element", f"<{tag}> x{n} — {note}"))

    # 2. Paint servers pointing at non-gradients.
    #    getPathFillType() -> GetFillTypeOp accepts only linear/radialGradient.
    #    Anything else falls through to parseColour(), which cannot parse
    #    "url(#...)" and returns the default: BLACK for a closed path.
    for attr, ref in URLREF_RE.findall(text):
        target = index.get(ref)
        if attr in ("fill", "stroke"):
            if target is None:
                findings.append(Finding("BLOCK", "dangling-paint-ref",
                                        f'{attr}="url(#{ref})" — target missing; falls back to black'))
            elif target not in ("linearGradient", "radialGradient"):
                sev = "BLOCK" if attr == "fill" else "WARN"
                findings.append(Finding(sev, "unsupported-paint",
                                        f'{attr}="url(#{ref})" -> <{target}>; renders SOLID BLACK'))
        elif attr == "mask":
            findings.append(Finding("BLOCK", "mask-ignored",
                                    f'mask="url(#{ref})" — masks unsupported; renders unmasked'))
        elif attr == "filter":
            findings.append(Finding("BLOCK", "filter-ignored",
                                    f'filter="url(#{ref})" — filters unsupported'))
        elif attr == "clip-path" and target != "clipPath":
            findings.append(Finding("WARN", "bad-clip-ref",
                                    f'clip-path -> <{target}>; expected <clipPath>'))

    # 3. <use> resolution. parseUsePath() -> UsePathOp -> parsePathElement(),
    #    which only accepts single shape primitives. parseUseOther() only tries
    #    text/image. So <use> -> <symbol> or <g> renders NOTHING.
    for m in re.finditer(r"<use\b([^>]*)>", text):
        mh = HREF_RE.search(m.group(1))
        if not mh:
            findings.append(Finding("WARN", "use-no-href", "<use> without a local href"))
            continue
        ref = mh.group(1)
        target = index.get(ref)
        if target is None:
            findings.append(Finding("BLOCK", "dangling-use", f'<use href="#{ref}"> — target missing'))
        elif target in ("symbol", "g"):
            findings.append(Finding("BLOCK", "use-of-group",
                                    f'<use href="#{ref}"> -> <{target}>; JUCE renders nothing'))
        elif target not in SHAPES:
            findings.append(Finding("BLOCK", "use-of-nonshape",
                                    f'<use href="#{ref}"> -> <{target}>; not a shape primitive'))

    # 4. CSS <style> blocks: parseCSSStyle handles only simple rules; class
    #    selectors resolved via getStyleAttribute are fragile.
    if tags.get("style"):
        findings.append(Finding("WARN", "css-style",
                                f"<style> x{tags['style']} — JUCE CSS support is minimal"))

    return findings, tags


def main():
    root = Path(sys.argv[1] if len(sys.argv) > 1 else ".")
    files = sorted(root.rglob("*.svg"))
    if not files:
        print(f"No SVGs under {root}")
        return 1

    clean, broken = [], []
    kind_totals = Counter()

    for f in files:
        findings, _ = analyse(f)
        blocks = [x for x in findings if x.sev == "BLOCK"]
        for x in findings:
            kind_totals[x.kind] += 1
        (broken if blocks else clean).append((f, findings))

    print("=" * 78)
    print("JUCE 8.0.12 SVG COMPATIBILITY REPORT")
    print("=" * 78)
    print(f"\nScanned {len(files)} files -> {len(clean)} usable, {len(broken)} with blockers\n")

    if broken:
        print("-" * 78)
        print("FILES WITH BLOCKERS")
        print("-" * 78)
        for f, findings in broken:
            rel = f.relative_to(root)
            print(f"\n  {rel}")
            grouped = Counter((x.sev, x.kind, x.detail.split(";")[-1].strip()) for x in findings)
            for (sev, kind, gist), n in grouped.most_common():
                tag = f"x{n}" if n > 1 else "  "
                print(f"      [{sev}] {kind:22} {tag:4} {gist}")

    print("\n" + "-" * 78)
    print("CLEAN FILES (safe to ship as-is)")
    print("-" * 78)
    for f, _ in clean:
        print(f"  OK  {f.relative_to(root)}")

    print("\n" + "-" * 78)
    print("ISSUE TOTALS")
    print("-" * 78)
    for kind, n in kind_totals.most_common():
        print(f"  {n:5}  {kind}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
