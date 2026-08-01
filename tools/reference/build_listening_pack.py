#!/usr/bin/env python3
"""Build a deterministic blind SendBloom engine-listening pack.

This tool deliberately generates its own stimuli.  It does not download,
decode, or redistribute any public Reverb-X recording.  Public examples remain
external listening references; the A/B files produced here compare SendBloom
renderer builds under repeatable settings.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import math
import os
import random
import struct
import subprocess
import tempfile
import wave
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Sequence


SAMPLE_RATE = 32768
RANDOM_SEED = 20260730
REQUESTED_RMS_DBFS = -20.0
MAX_PEAK_DBFS = -1.0
PCM24_SCALE = 1 << 23
PCM24_MIN = -PCM24_SCALE
PCM24_MAX = PCM24_SCALE - 1


@dataclass(frozen=True)
class Cell:
    cell_id: str
    comparison: str
    stimulus: str
    rt60_seconds: float
    dark_mix: float
    distn: float
    total_seconds: float
    active_seconds: float
    baseline_role: str


CELLS = (
    Cell("density-bright", "density_topology", "density_burst", 3.0, 0.0, 0.0,
         8.0, 0.32, "density_baseline"),
    Cell("density-dark", "density_topology", "density_burst", 3.0, 1.0, 0.0,
         8.0, 0.32, "density_baseline"),
    Cell("dirt-pluck", "wet_dirt", "guitar_pluck", 3.0, 0.0, 1.0,
         8.0, 0.90, "dirt_baseline"),
    Cell("dirt-chord-long-dark", "wet_dirt", "sustained_chord", 5.5, 1.0, 1.0,
         11.0, 1.70, "dirt_baseline"),
)


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def _ensure_finite(samples: Sequence[float], description: str) -> None:
    if not samples:
        raise ValueError(f"{description} is empty")
    for index, sample in enumerate(samples):
        if not math.isfinite(sample):
            raise ValueError(f"{description} contains a non-finite sample at index {index}")


def _rms(samples: Sequence[float]) -> float:
    if not samples:
        return 0.0
    return math.sqrt(math.fsum(sample * sample for sample in samples) / len(samples))


def _peak(samples: Sequence[float]) -> float:
    return max((abs(sample) for sample in samples), default=0.0)


def _linear_to_dbfs(value: float) -> float | None:
    return 20.0 * math.log10(value) if value > 0.0 else None


def _metrics(samples: Sequence[float]) -> dict[str, float | None | int]:
    rms = _rms(samples)
    peak = _peak(samples)
    return {
        "sample_count": len(samples),
        "rms": rms,
        "rms_dbfs": _linear_to_dbfs(rms),
        "peak": peak,
        "peak_dbfs": _linear_to_dbfs(peak),
    }


def _write_raw_f32(path: Path, samples: Sequence[float]) -> str:
    _ensure_finite(samples, "raw f32 source")
    payload = struct.pack(f"<{len(samples)}f", *samples)
    path.write_bytes(payload)
    return _sha256_bytes(payload)


def _read_raw_f32(path: Path, expected_samples: int | None = None) -> tuple[list[float], str]:
    try:
        payload = path.read_bytes()
    except OSError as exc:
        raise ValueError(f"could not read renderer output {path}: {exc}") from exc
    if not payload:
        raise ValueError(f"renderer output {path} is empty")
    if len(payload) % 4:
        raise ValueError(
            f"renderer output {path} is malformed: {len(payload)} bytes is not packed f32"
        )
    samples = [item[0] for item in struct.iter_unpack("<f", payload)]
    if expected_samples is not None and len(samples) != expected_samples:
        raise ValueError(
            f"renderer output {path} has {len(samples)} samples; expected {expected_samples}"
        )
    _ensure_finite(samples, f"renderer output {path}")
    return samples, _sha256_bytes(payload)


def _quantize_pcm24(samples: Sequence[float]) -> list[int]:
    _ensure_finite(samples, "PCM source")
    quantized: list[int] = []
    for sample in samples:
        integer = round(sample * PCM24_SCALE)
        quantized.append(max(PCM24_MIN, min(PCM24_MAX, integer)))
    return quantized


def _pack_pcm24(integers: Iterable[int]) -> bytes:
    payload = bytearray()
    for integer in integers:
        if not PCM24_MIN <= integer <= PCM24_MAX:
            raise ValueError(f"24-bit PCM integer out of range: {integer}")
        payload.extend(int(integer).to_bytes(3, "little", signed=True))
    return bytes(payload)


def _write_pcm24_wav(path: Path, samples: Sequence[float]) -> dict[str, object]:
    integers = _quantize_pcm24(samples)
    payload = _pack_pcm24(integers)
    with wave.open(str(path), "wb") as output:
        output.setparams((1, 3, SAMPLE_RATE, len(integers), "NONE", "not compressed"))
        output.writeframes(payload)
    decoded = [integer / PCM24_SCALE for integer in integers]
    return {
        "path": path.name,
        "sha256": _sha256_file(path),
        "format": "mono packed 24-bit PCM WAV",
        "sample_rate_hz": SAMPLE_RATE,
        **_metrics(decoded),
    }


def _level_match_pair(
    baseline: Sequence[float],
    candidate: Sequence[float],
    requested_rms_dbfs: float = REQUESTED_RMS_DBFS,
    max_peak_dbfs: float = MAX_PEAK_DBFS,
) -> tuple[list[float], list[float], dict[str, float]]:
    """Apply independent gains to one shared RMS target with shared headroom.

    The returned floating-point sequences have the same RMS (up to the final
    correctly-rounded binary operation).  A one-LSB guard below the requested
    peak ceiling keeps subsequent 24-bit quantisation inside that ceiling.
    """
    _ensure_finite(baseline, "baseline render")
    _ensure_finite(candidate, "candidate render")
    if len(baseline) != len(candidate):
        raise ValueError("A/B renders must contain the same number of samples")

    baseline_rms = _rms(baseline)
    candidate_rms = _rms(candidate)
    if baseline_rms <= 0.0 or candidate_rms <= 0.0:
        raise ValueError("A/B renders must both contain non-zero signal")

    requested_rms = 10.0 ** (requested_rms_dbfs / 20.0)
    peak_limit = 10.0 ** (max_peak_dbfs / 20.0)
    guarded_peak_limit = peak_limit - (1.0 / PCM24_SCALE)
    crest = max(_peak(baseline) / baseline_rms, _peak(candidate) / candidate_rms)
    target_rms = min(requested_rms, guarded_peak_limit / crest)

    matched_baseline = [sample * (target_rms / baseline_rms) for sample in baseline]
    matched_candidate = [sample * (target_rms / candidate_rms) for sample in candidate]
    baseline_matched_rms = _rms(matched_baseline)
    candidate_matched_rms = _rms(matched_candidate)

    # A second, tiny correction removes accumulated summation/scale rounding.
    # It does not alter the target or the spectral/time relationship.
    common_rms = min(baseline_matched_rms, candidate_matched_rms)
    matched_baseline = [sample * (common_rms / baseline_matched_rms) for sample in matched_baseline]
    matched_candidate = [sample * (common_rms / candidate_matched_rms) for sample in matched_candidate]

    return matched_baseline, matched_candidate, {
        "requested_rms_dbfs": requested_rms_dbfs,
        "applied_target_rms": common_rms,
        "applied_target_rms_dbfs": 20.0 * math.log10(common_rms),
        "max_peak_dbfs": max_peak_dbfs,
        "baseline_gain": common_rms / baseline_rms,
        "candidate_gain": common_rms / candidate_rms,
    }


def _assignments(cell_ids: Sequence[str], seed: int = RANDOM_SEED) -> dict[str, dict[str, str]]:
    rng = random.Random(seed)
    shuffled = list(cell_ids)
    rng.shuffle(shuffled)
    baseline_as_a = set(shuffled[: (len(shuffled) + 1) // 2])
    result: dict[str, dict[str, str]] = {}
    for cell_id in cell_ids:
        a_role = "baseline" if cell_id in baseline_as_a else "candidate"
        b_role = "candidate" if a_role == "baseline" else "baseline"
        result[cell_id] = {"A": a_role, "B": b_role}
    return result


def _density_burst(total_samples: int, active_samples: int) -> list[float]:
    rng = random.Random(RANDOM_SEED + 101)
    samples = [0.0] * total_samples
    low_state = 0.0
    impulses = {0: 0.48, round(0.047 * SAMPLE_RATE): -0.34,
                round(0.113 * SAMPLE_RATE): 0.29, round(0.219 * SAMPLE_RATE): -0.22}
    for index in range(min(active_samples, total_samples)):
        white = rng.uniform(-1.0, 1.0)
        low_state = 0.89 * low_state + 0.11 * white
        envelope = math.exp(-index / (0.105 * SAMPLE_RATE))
        pitched = (
            math.sin(2.0 * math.pi * 173.0 * index / SAMPLE_RATE)
            + 0.55 * math.sin(2.0 * math.pi * 421.0 * index / SAMPLE_RATE)
        )
        samples[index] = envelope * (0.34 * low_state + 0.13 * pitched)
        samples[index] += impulses.get(index, 0.0)
    return samples


def _guitar_pluck(total_samples: int, active_samples: int) -> list[float]:
    samples = [0.0] * total_samples
    for index in range(min(active_samples, total_samples)):
        seconds = index / SAMPLE_RATE
        envelope = math.exp(-seconds / 0.24)
        pick = math.exp(-seconds / 0.013) * math.sin(2.0 * math.pi * 2150.0 * seconds)
        body = sum(
            weight * math.sin(2.0 * math.pi * 110.0 * harmonic * seconds)
            for harmonic, weight in ((1, 0.62), (2, 0.23), (3, 0.11), (5, 0.05))
        )
        samples[index] = envelope * body + 0.16 * pick
    peak = _peak(samples)
    return [sample * (0.78 / peak) for sample in samples]


def _sustained_chord(total_samples: int, active_samples: int) -> list[float]:
    samples = [0.0] * total_samples
    frequencies = (110.0, 138.591, 164.814, 220.0)
    release_samples = round(0.32 * SAMPLE_RATE)
    attack_samples = round(0.018 * SAMPLE_RATE)
    for index in range(min(active_samples, total_samples)):
        attack = min(1.0, index / max(1, attack_samples))
        release = min(1.0, (active_samples - index) / max(1, release_samples))
        envelope = attack * release
        seconds = index / SAMPLE_RATE
        body = math.fsum(
            math.sin(2.0 * math.pi * frequency * seconds + voice * 0.31)
            + 0.16 * math.sin(4.0 * math.pi * frequency * seconds + voice * 0.17)
            for voice, frequency in enumerate(frequencies)
        )
        samples[index] = envelope * body
    peak = _peak(samples)
    return [sample * (0.78 / peak) for sample in samples]


def _stimulus(cell: Cell) -> list[float]:
    total_samples = round(cell.total_seconds * SAMPLE_RATE)
    active_samples = round(cell.active_seconds * SAMPLE_RATE)
    if cell.stimulus == "density_burst":
        samples = _density_burst(total_samples, active_samples)
    elif cell.stimulus == "guitar_pluck":
        samples = _guitar_pluck(total_samples, active_samples)
    elif cell.stimulus == "sustained_chord":
        samples = _sustained_chord(total_samples, active_samples)
    else:  # pragma: no cover - Cell definitions are static and audited.
        raise ValueError(f"unknown stimulus: {cell.stimulus}")
    _ensure_finite(samples, f"{cell.cell_id} stimulus")
    if _peak(samples) > 10.0 ** (MAX_PEAK_DBFS / 20.0):
        raise ValueError(f"{cell.cell_id} dry stimulus exceeds the pack headroom ceiling")
    return samples


def _format_number(number: float) -> str:
    return format(number, ".9g")


def _logical_command(renderer_role: str, cell: Cell, output_role: str) -> list[str]:
    return [
        f"{{{renderer_role}_renderer}}",
        "stream",
        "{source_raw_f32}",
        f"{{{output_role}_output_raw_f32}}",
        _format_number(cell.rt60_seconds),
        _format_number(cell.dark_mix),
        _format_number(cell.distn),
        "ring",
    ]


def _run_renderer(
    renderer: Path,
    source: Path,
    output: Path,
    cell: Cell,
    expected_samples: int,
) -> tuple[list[float], str]:
    command = [
        str(renderer), "stream", str(source), str(output),
        _format_number(cell.rt60_seconds), _format_number(cell.dark_mix),
        _format_number(cell.distn), "ring",
    ]
    try:
        completed = subprocess.run(command, check=False, capture_output=True, text=True)
    except OSError as exc:
        raise RuntimeError(f"could not launch renderer {renderer}: {exc}") from exc
    if completed.returncode != 0:
        detail = completed.stderr.strip() or completed.stdout.strip() or "no diagnostic output"
        raise RuntimeError(
            f"renderer {renderer} failed for {cell.cell_id} with exit "
            f"{completed.returncode}: {detail}"
        )
    return _read_raw_f32(output, expected_samples)


def _validate_renderer(path: Path, role: str) -> Path:
    resolved = path.expanduser().resolve()
    if not resolved.is_file():
        raise ValueError(f"{role} renderer does not exist: {resolved}")
    if not os.access(resolved, os.X_OK):
        raise ValueError(f"{role} renderer is not executable: {resolved}")
    return resolved


def _scorecard() -> str:
    sections = [
        "# SendBloom blind listening scorecard",
        "",
        "Human verdict: ____________________  (status: human_needed)",
        "",
        "These files compare SendBloom builds. They are not captures of public hardware, "
        "and no third-party audio is embedded.",
        "This is a focused density/dirt change screen; it does not satisfy the full "
        "gate/send/dry hardware-approval matrix in the public-reference catalogue.",
        "",
        "## Instructions",
        "",
        "1. Do not open `manifest.json` until every cell is scored; it contains the answer key.",
        "2. Keep monitor level fixed. Loop the onset, then hear the complete tail. Switch A/B often.",
        "3. Alongside the separately catalogued public references, score similarity from 0 "
        "(not similar) to 4 (strongly similar). Add short, concrete observations.",
        "4. Preference uses 0 = strongly A, 1 = slightly A, 2 = none, 3 = slightly B, "
        "4 = strongly B. It is not itself evidence of reference similarity.",
        "5. Replay each cell at least once before recording a score.",
        "",
        "## Public anchors (listen separately; not golden audio)",
        "",
        "- [Knobs](https://www.youtube.com/watch?v=QrPpxCW4EzI): trails 1:53-2:52, "
        "gate 2:53-4:14, Igor 4:15-4:43.",
        "- [Get Offset](https://www.youtube.com/watch?v=fT_9XnoZ4I0): Size 5:12.4-5:57.7, "
        "Distn 6:35.6-7:31.6, Dark/Bright 11:20.2-11:56.7.",
        "- [TUNNEL OF REVERB](https://www.youtube.com/watch?v=iagH2FIFI6A): reamped "
        "acoustic 4:37-7:20, drums 9:47-11:09.",
        "",
        "The public examples use different sources, settings, rooms, recording chains, and "
        "stream codecs. Scores against them are directional observations, not a hardware-match "
        "verdict.",
        "",
    ]
    for cell in CELLS:
        sections.extend([
            f"## {cell.cell_id}",
            "",
            "| Dimension | A (0-4) | B (0-4) | Observation |",
            "|---|---:|---:|---|",
            "| Density |  |  |  |",
            "| Grain |  |  |  |",
            "| Onset |  |  |  |",
            "| Spectrum |  |  |  |",
            "| Decay |  |  |  |",
            "| Dirt |  |  |  |",
            "",
            "Preference (0-4): ____",
            "",
        ])
    sections.extend([
        "## Overall",
        "",
        "Most reference-like rendition(s): ____________________",
        "",
        "Remaining mismatch and confidence: ____________________",
        "",
    ])
    return "\n".join(sections)


def _expected_artifact_names() -> set[str]:
    names = {"SCORECARD.md", "manifest.json"}
    for cell in CELLS:
        names.update(f"{cell.cell_id}-{suffix}.wav" for suffix in ("dry", "A", "B"))
    return names


def _build_pack_into(
    density_baseline_renderer: Path,
    dirt_baseline_renderer: Path,
    candidate_renderer: Path,
    output: Path,
) -> dict[str, object]:
    renderers = {
        "density_baseline": _validate_renderer(density_baseline_renderer, "density baseline"),
        "dirt_baseline": _validate_renderer(dirt_baseline_renderer, "dirt baseline"),
        "candidate": _validate_renderer(candidate_renderer, "candidate"),
    }
    output.mkdir(parents=True, exist_ok=False)

    assignments = _assignments([cell.cell_id for cell in CELLS])
    manifest: dict[str, object] = {
        "schema_version": 1,
        "generator": "tools/reference/build_listening_pack.py",
        "generator_seed": RANDOM_SEED,
        "sample_rate_hz": SAMPLE_RATE,
        "comparison_scope": "SendBloom build-to-build blind A/B",
        "public_hardware_audio_embedded": False,
        "notice": (
            "These A/B files compare SendBloom builds, not public Reverb-X hardware. "
            "No downloaded or third-party audio is included."
        ),
        "human_verdict": {"status": "human_needed", "verdict": None},
        "renderers": {
            role: {"binary_name": path.name, "sha256": _sha256_file(path)}
            for role, path in sorted(renderers.items())
        },
        "answer_key": {},
        "cells": [],
    }

    with tempfile.TemporaryDirectory(prefix="sendbloom-listening-pack-") as temporary:
        work = Path(temporary)
        for cell in CELLS:
            source = _stimulus(cell)
            source_raw = work / f"{cell.cell_id}-source.f32"
            source_raw_sha = _write_raw_f32(source_raw, source)
            dry_path = output / f"{cell.cell_id}-dry.wav"
            dry_artifact = _write_pcm24_wav(dry_path, source)

            baseline_raw = work / f"{cell.cell_id}-baseline.f32"
            candidate_raw = work / f"{cell.cell_id}-candidate.f32"
            baseline, baseline_raw_sha = _run_renderer(
                renderers[cell.baseline_role], source_raw, baseline_raw, cell, len(source)
            )
            candidate, candidate_raw_sha = _run_renderer(
                renderers["candidate"], source_raw, candidate_raw, cell, len(source)
            )
            matched_baseline, matched_candidate, level_match = _level_match_pair(
                baseline, candidate
            )

            rendered = {
                "baseline": {
                    "samples": matched_baseline,
                    "renderer_role": cell.baseline_role,
                    "raw_sha256": baseline_raw_sha,
                    "raw_metrics": _metrics(baseline),
                },
                "candidate": {
                    "samples": matched_candidate,
                    "renderer_role": "candidate",
                    "raw_sha256": candidate_raw_sha,
                    "raw_metrics": _metrics(candidate),
                },
            }
            assignment = assignments[cell.cell_id]
            files: dict[str, object] = {"dry": dry_artifact}
            answer: dict[str, str] = {}
            for label in ("A", "B"):
                build_role = assignment[label]
                artifact_path = output / f"{cell.cell_id}-{label}.wav"
                artifact = _write_pcm24_wav(artifact_path, rendered[build_role]["samples"])
                artifact["build_role"] = build_role
                artifact["renderer_role"] = rendered[build_role]["renderer_role"]
                artifact["raw_render_sha256"] = rendered[build_role]["raw_sha256"]
                files[label] = artifact
                answer[label] = str(rendered[build_role]["renderer_role"])

            a_rms = float(files["A"]["rms"])  # type: ignore[index]
            b_rms = float(files["B"]["rms"])  # type: ignore[index]
            a_peak = float(files["A"]["peak"])  # type: ignore[index]
            b_peak = float(files["B"]["peak"])  # type: ignore[index]
            level_match.update({
                "post_pcm24_a_rms": a_rms,
                "post_pcm24_b_rms": b_rms,
                "post_pcm24_rms_delta": abs(a_rms - b_rms),
                "post_pcm24_rms_delta_db": abs(20.0 * math.log10(a_rms / b_rms)),
                "post_pcm24_max_peak": max(a_peak, b_peak),
                "post_pcm24_max_peak_dbfs": 20.0 * math.log10(max(a_peak, b_peak)),
            })

            manifest["answer_key"][cell.cell_id] = answer  # type: ignore[index]
            manifest["cells"].append({  # type: ignore[index]
                "id": cell.cell_id,
                "comparison": cell.comparison,
                "stimulus": {
                    "kind": cell.stimulus,
                    "repository_owned": True,
                    "active_seconds": cell.active_seconds,
                    "tail_silence_seconds": cell.total_seconds - cell.active_seconds,
                    "total_seconds": cell.total_seconds,
                    "raw_f32_sha256": source_raw_sha,
                    "raw_metrics": _metrics(source),
                },
                "settings": {
                    "rt60_seconds": cell.rt60_seconds,
                    "dark_mix": cell.dark_mix,
                    "distn": cell.distn,
                    "engine": "ring",
                },
                "commands": {
                    "baseline": _logical_command(cell.baseline_role, cell, "baseline"),
                    "candidate": _logical_command("candidate", cell, "candidate"),
                },
                "unmatched_render_metrics": {
                    "baseline": rendered["baseline"]["raw_metrics"],
                    "candidate": rendered["candidate"]["raw_metrics"],
                },
                "level_matching": level_match,
                "assignment": answer,
                "files": files,
            })

    scorecard_path = output / "SCORECARD.md"
    scorecard_path.write_text(_scorecard(), encoding="utf-8")
    manifest["scorecard"] = {
        "path": scorecard_path.name,
        "sha256": _sha256_file(scorecard_path),
    }
    manifest_path = output / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    return manifest


def build_pack(
    density_baseline_renderer: Path,
    dirt_baseline_renderer: Path,
    candidate_renderer: Path,
    output: Path,
) -> dict[str, object]:
    target = output.expanduser().resolve()
    if target.exists():
        raise ValueError(f"output path already exists; choose a fresh directory: {target}")
    target.parent.mkdir(parents=True, exist_ok=True)

    with tempfile.TemporaryDirectory(
        prefix=f".{target.name}-staging-", dir=target.parent
    ) as temporary:
        staging = Path(temporary) / "pack"
        manifest = _build_pack_into(
            density_baseline_renderer,
            dirt_baseline_renderer,
            candidate_renderer,
            staging,
        )
        actual = {path.name for path in staging.iterdir() if path.is_file()}
        unexpected_entries = [path.name for path in staging.iterdir() if not path.is_file()]
        expected = _expected_artifact_names()
        if actual != expected or unexpected_entries:
            raise RuntimeError(
                f"staged pack contents mismatch: expected {sorted(expected)}, "
                f"actual {sorted(actual)}, non-files {sorted(unexpected_entries)}"
            )
        os.replace(staging, target)
        return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--density-baseline-renderer", required=True, type=Path)
    parser.add_argument("--dirt-baseline-renderer", required=True, type=Path)
    parser.add_argument("--candidate-renderer", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    try:
        build_pack(
            args.density_baseline_renderer,
            args.dirt_baseline_renderer,
            args.candidate_renderer,
            args.output,
        )
    except (ValueError, RuntimeError) as exc:
        parser.exit(1, f"error: {exc}\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
