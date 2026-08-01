#!/usr/bin/env python3
"""Measure deterministic objective metrics from a mono raw float32 IR."""

from __future__ import annotations

import argparse
import hashlib
import json
import math
import struct
import sys
from pathlib import Path
from typing import Sequence


SCHEMA_VERSION = 1


def _positive_int(value: str) -> int:
    try:
        parsed = int(value)
    except ValueError as exc:
        raise argparse.ArgumentTypeError("must be a positive integer") from exc
    if parsed <= 0:
        raise argparse.ArgumentTypeError("must be a positive integer")
    return parsed


def read_raw_float32(path: Path) -> tuple[bytes, list[float]]:
    """Read strict mono little-endian IEEE-754 float32 sample data."""
    try:
        payload = path.read_bytes()
    except OSError as exc:
        raise ValueError(f"cannot read input {path}: {exc}") from exc

    if not payload:
        raise ValueError("input is empty; expected mono raw little-endian float32 samples")
    if len(payload) % 4 != 0:
        raise ValueError(
            f"input byte length {len(payload)} is not a multiple of 4 for float32 data"
        )

    samples: list[float] = []
    for index, (sample,) in enumerate(struct.iter_unpack("<f", payload)):
        if not math.isfinite(sample):
            raise ValueError(f"non-finite sample at index {index}")
        samples.append(sample)
    return payload, samples


def _window_metrics(
    samples: Sequence[float], start: int, end: int
) -> tuple[float | None, float | None]:
    window = samples[start:end]
    if not window:
        return None, None

    count = len(window)
    peak = max(abs(sample) for sample in window)
    rms = math.sqrt(math.fsum(sample * sample for sample in window) / count)
    crest_factor = peak / rms if rms > 0.0 else 0.0

    mean = math.fsum(window) / count
    centered_squares = [(sample - mean) ** 2 for sample in window]
    second_moment = math.fsum(centered_squares) / count
    if second_moment > 0.0:
        fourth_moment = math.fsum(value * value for value in centered_squares) / count
        kurtosis = fourth_moment / (second_moment * second_moment)
    else:
        kurtosis = 0.0
    return crest_factor, kurtosis


def _edc_metrics(
    samples: Sequence[float], sample_rate: int
) -> tuple[float | None, float | None, int, float | None]:
    """Return T30, fit R-squared/count, and one-second remaining energy."""
    edc = [0.0] * len(samples)
    remaining = 0.0
    for index in range(len(samples) - 1, -1, -1):
        sample = samples[index]
        remaining += sample * sample
        edc[index] = remaining

    total = edc[0]
    if not math.isfinite(total):
        raise ValueError("sample energy overflowed while measuring the IR")
    if total <= 0.0:
        return None, None, 0, None

    point_count = 0
    sum_time = 0.0
    sum_db = 0.0
    sum_time_squared = 0.0
    sum_db_squared = 0.0
    sum_time_db = 0.0

    for index, energy in enumerate(edc):
        if energy <= 0.0:
            continue
        db = 10.0 * math.log10(energy / total)
        if -35.0 <= db <= -5.0:
            time = index / sample_rate
            point_count += 1
            sum_time += time
            sum_db += db
            sum_time_squared += time * time
            sum_db_squared += db * db
            sum_time_db += time * db

    t30_seconds: float | None = None
    fit_r2: float | None = None
    if point_count >= 2:
        count = float(point_count)
        centered_time = sum_time_squared - (sum_time * sum_time / count)
        centered_db = sum_db_squared - (sum_db * sum_db / count)
        covariance = sum_time_db - (sum_time * sum_db / count)
        if centered_time > 0.0 and centered_db > 0.0:
            slope = covariance / centered_time
            if slope < 0.0:
                t30_seconds = -60.0 / slope
            fit_r2 = max(
                0.0,
                min(1.0, covariance * covariance / (centered_time * centered_db)),
            )

    one_second_index = sample_rate
    energy_at_one_second: float | None = None
    if one_second_index < len(edc) and edc[one_second_index] > 0.0:
        energy_at_one_second = 10.0 * math.log10(edc[one_second_index] / total)

    return t30_seconds, fit_r2, point_count, energy_at_one_second


def analyze(
    payload: bytes,
    samples: Sequence[float],
    sample_rate: int,
    window_start_seconds: float = 0.100,
    window_end_seconds: float = 0.250,
    onset_threshold: float = 1.0e-4,
) -> dict[str, int | float | str | None]:
    """Calculate the stable metric schema used by the command-line tool."""
    count = len(samples)
    peak = max(abs(sample) for sample in samples)
    rms = math.sqrt(math.fsum(sample * sample for sample in samples) / count)
    onset_index = next(
        (index for index, sample in enumerate(samples) if abs(sample) >= onset_threshold),
        None,
    )

    requested_window_start = math.floor(window_start_seconds * sample_rate + 0.5)
    requested_window_end = math.floor(window_end_seconds * sample_rate + 0.5)
    window_start = min(requested_window_start, count)
    window_end = min(requested_window_end, count)
    crest_factor, kurtosis = _window_metrics(samples, window_start, window_end)
    t30_seconds, fit_r2, fit_points, energy_at_one_second = _edc_metrics(
        samples, sample_rate
    )

    result: dict[str, int | float | str | None] = {
        "duration_seconds": count / sample_rate,
        "energy_remaining_db_at_1s": energy_at_one_second,
        "onset_ms": None if onset_index is None else onset_index * 1000.0 / sample_rate,
        "onset_threshold": onset_threshold,
        "peak": peak,
        "rms": rms,
        "sample_count": count,
        "sample_rate_hz": sample_rate,
        "schema_version": SCHEMA_VERSION,
        "source_sha256": hashlib.sha256(payload).hexdigest(),
        "t30_fit_point_count": fit_points,
        "t30_fit_r2": fit_r2,
        "t30_seconds": t30_seconds,
        "window_crest_factor": crest_factor,
        "window_end_seconds": window_end_seconds,
        "window_kurtosis": kurtosis,
        "window_sample_count": max(0, window_end - window_start),
        "window_start_seconds": window_start_seconds,
    }
    for key, value in result.items():
        if isinstance(value, float) and not math.isfinite(value):
            raise ValueError(f"metric {key} is non-finite")
    return result


def _validate_arguments(args: argparse.Namespace) -> None:
    for name in ("window_start", "window_end", "onset_threshold"):
        value = getattr(args, name)
        if not math.isfinite(value):
            raise ValueError(f"--{name.replace('_', '-')} must be finite")
    if args.window_start < 0.0:
        raise ValueError("--window-start must be non-negative")
    if args.window_end <= args.window_start:
        raise ValueError("--window-end must be greater than --window-start")
    if args.onset_threshold <= 0.0:
        raise ValueError("--onset-threshold must be greater than zero")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Measure a mono raw little-endian float32 impulse response."
    )
    parser.add_argument("input", type=Path, help="mono raw little-endian float32 input")
    parser.add_argument("--sample-rate", required=True, type=_positive_int)
    parser.add_argument("--json", required=True, type=Path, help="output JSON path")
    parser.add_argument("--window-start", type=float, default=0.100, metavar="SECONDS")
    parser.add_argument("--window-end", type=float, default=0.250, metavar="SECONDS")
    parser.add_argument("--onset-threshold", type=float, default=1.0e-4)
    return parser


def main(argv: Sequence[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        _validate_arguments(args)
        payload, samples = read_raw_float32(args.input)
        result = analyze(
            payload,
            samples,
            args.sample_rate,
            args.window_start,
            args.window_end,
            args.onset_threshold,
        )
        encoded = json.dumps(result, indent=2, sort_keys=True, allow_nan=False) + "\n"
        args.json.parent.mkdir(parents=True, exist_ok=True)
        args.json.write_text(encoded, encoding="utf-8")
    except (OSError, OverflowError, ValueError) as exc:
        parser.error(str(exc))
    return 0


if __name__ == "__main__":
    sys.exit(main())
