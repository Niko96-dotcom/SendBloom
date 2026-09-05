#!/usr/bin/env python3
"""Compare BenchmarkProcessor CSVs and (optionally) exact rendered output."""

import argparse
import csv
import math
from pathlib import Path


def read_results(path):
    with path.open() as stream:
        rows = list(csv.DictReader(line for line in stream if not line.startswith("#")))
    results = {row["scenario"]: row for row in rows}
    if not rows or len(results) != len(rows):
        raise ValueError(f"{path}: empty results or duplicate scenarios")
    for row in rows:
        for column in ("min_core_percent", "median_core_percent", "max_core_percent"):
            value = float(row[column])
            if not math.isfinite(value) or value <= 0:
                raise ValueError(f"{path}: invalid {column}")
    return results


def compare(before, after, before_audio=None, after_audio=None):
    if before.keys() != after.keys():
        raise ValueError("scenario sets differ")
    lines = ["scenario,before_core_percent,after_core_percent,reduction_percent,audio"]
    for name, old in before.items():
        new = after[name]
        if any(old[key] != new[key] for key in ("rate", "block", "instances")):
            raise ValueError(f"{name}: workload configuration differs")
        status = "unchecked"
        if before_audio is not None:
            old_audio = (before_audio / f"{name}.f32").read_bytes()
            new_audio = (after_audio / f"{name}.f32").read_bytes()
            if not old_audio or old_audio != new_audio:
                raise ValueError(f"{name}: rendered audio differs or is empty")
            status = "byte-identical"
        old_time = float(old["median_core_percent"])
        new_time = float(new["median_core_percent"])
        lines.append(f"{name},{old_time:.7g},{new_time:.7g},"
                     f"{100 * (old_time - new_time) / old_time:.2f},{status}")
    return "\n".join(lines)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("before", type=Path)
    parser.add_argument("after", type=Path)
    parser.add_argument("--before-audio", type=Path)
    parser.add_argument("--after-audio", type=Path)
    args = parser.parse_args()
    if (args.before_audio is None) != (args.after_audio is None):
        parser.error("supply both audio directories or neither")
    try:
        print(compare(read_results(args.before), read_results(args.after),
                      args.before_audio, args.after_audio))
    except (OSError, ValueError, KeyError) as error:
        parser.exit(1, f"comparison failed: {error}\n")


if __name__ == "__main__":
    main()
