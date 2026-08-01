import hashlib
import json
import math
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
TOOL = ROOT / "tools" / "reference" / "measure_ir.py"


def encode_float32(samples):
    return b"".join(struct.pack("<f", sample) for sample in samples)


class MeasureIrTest(unittest.TestCase):
    def run_tool(self, source, output, *extra):
        return subprocess.run(
            [
                sys.executable,
                str(TOOL),
                str(source),
                "--sample-rate",
                "8000",
                "--json",
                str(output),
                *extra,
            ],
            capture_output=True,
            text=True,
            check=False,
        )

    def test_exponential_ir_has_known_t60_and_deterministic_metrics(self):
        sample_rate = 8000
        expected_t60 = 1.2
        duration_seconds = 6.0
        samples = [
            10.0 ** (-3.0 * index / (sample_rate * expected_t60))
            for index in range(round(sample_rate * duration_seconds))
        ]
        payload = encode_float32(samples)

        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source = directory / "exponential.f32"
            first_output = directory / "first.json"
            second_output = directory / "second.json"
            source.write_bytes(payload)

            first = self.run_tool(source, first_output)
            second = self.run_tool(source, second_output)

            self.assertEqual(first.returncode, 0, first.stderr)
            self.assertEqual(second.returncode, 0, second.stderr)
            self.assertEqual(first_output.read_bytes(), second_output.read_bytes())

            metrics = json.loads(first_output.read_text(encoding="utf-8"))
            self.assertEqual(metrics["schema_version"], 1)
            self.assertEqual(metrics["source_sha256"], hashlib.sha256(payload).hexdigest())
            self.assertEqual(metrics["sample_rate_hz"], sample_rate)
            self.assertEqual(metrics["sample_count"], len(samples))
            self.assertEqual(metrics["duration_seconds"], duration_seconds)
            self.assertEqual(metrics["peak"], 1.0)
            self.assertEqual(metrics["onset_ms"], 0.0)
            self.assertGreater(metrics["rms"], 0.0)
            self.assertAlmostEqual(metrics["t30_seconds"], expected_t60, delta=0.002)
            self.assertGreater(metrics["t30_fit_r2"], 0.999999)
            self.assertGreater(metrics["t30_fit_point_count"], 1000)
            self.assertAlmostEqual(metrics["energy_remaining_db_at_1s"], -50.0, delta=0.01)
            self.assertEqual(metrics["window_sample_count"], 1200)
            self.assertGreater(metrics["window_crest_factor"], 1.0)
            self.assertGreater(metrics["window_kurtosis"], 1.0)

    def test_rejects_malformed_and_nonfinite_raw_input(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            output = directory / "metrics.json"

            malformed = directory / "malformed.f32"
            malformed.write_bytes(b"\x00\x01\x02")
            malformed_result = self.run_tool(malformed, output)
            self.assertNotEqual(malformed_result.returncode, 0)
            self.assertIn("not a multiple of 4", malformed_result.stderr)
            self.assertFalse(output.exists())

            for name, value in (
                ("nan", float("nan")),
                ("positive-infinity", float("inf")),
                ("negative-infinity", float("-inf")),
            ):
                with self.subTest(value=name):
                    nonfinite = directory / f"{name}.f32"
                    nonfinite.write_bytes(encode_float32([1.0, value]))
                    nonfinite_result = self.run_tool(nonfinite, output)
                    self.assertNotEqual(nonfinite_result.returncode, 0)
                    self.assertIn("non-finite sample at index 1", nonfinite_result.stderr)
                    self.assertFalse(output.exists())

    def test_rejects_invalid_metric_arguments(self):
        with tempfile.TemporaryDirectory() as temporary_directory:
            directory = Path(temporary_directory)
            source = directory / "impulse.f32"
            output = directory / "metrics.json"
            source.write_bytes(encode_float32([1.0, 0.0]))

            bad_rate = subprocess.run(
                [
                    sys.executable,
                    str(TOOL),
                    str(source),
                    "--sample-rate",
                    "0",
                    "--json",
                    str(output),
                ],
                capture_output=True,
                text=True,
                check=False,
            )
            self.assertNotEqual(bad_rate.returncode, 0)
            self.assertIn("positive integer", bad_rate.stderr)

            bad_window = self.run_tool(
                source, output, "--window-start", "0.25", "--window-end", "0.1"
            )
            self.assertNotEqual(bad_window.returncode, 0)
            self.assertIn(
                "--window-end must be greater than --window-start", bad_window.stderr
            )
            self.assertFalse(output.exists())


if __name__ == "__main__":
    unittest.main()
