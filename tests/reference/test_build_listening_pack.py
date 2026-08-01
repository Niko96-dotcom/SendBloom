import math
import struct
import sys
import tempfile
import unittest
import wave
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/reference"))
import build_listening_pack as blp


class BuildListeningPackTest(unittest.TestCase):
    def test_randomization_is_deterministic_and_blind_labels_are_balanced(self):
        cell_ids = [cell.cell_id for cell in blp.CELLS]
        first = blp._assignments(cell_ids)
        second = blp._assignments(cell_ids)
        self.assertEqual(first, second)
        baseline_as_a = sum(value["A"] == "baseline" for value in first.values())
        candidate_as_a = sum(value["A"] == "candidate" for value in first.values())
        self.assertLessEqual(abs(baseline_as_a-candidate_as_a),1)
        for assignment in first.values():
            self.assertEqual(set(assignment.values()), {"baseline", "candidate"})

    def test_writes_signed_little_endian_packed_24_bit_pcm(self):
        samples = [-1.0, -0.5, 0.0, 0.5, 1.0]
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / "test.wav"
            artifact = blp._write_pcm24_wav(path, samples)
            with wave.open(str(path), "rb") as source:
                self.assertEqual(source.getnchannels(), 1)
                self.assertEqual(source.getsampwidth(), 3)
                self.assertEqual(source.getframerate(), blp.SAMPLE_RATE)
                payload = source.readframes(source.getnframes())
            decoded = [
                int.from_bytes(payload[index:index + 3], "little", signed=True)
                for index in range(0, len(payload), 3)
            ]
            self.assertEqual(
                decoded,
                [blp.PCM24_MIN, -4194304, 0, 4194304, blp.PCM24_MAX],
            )
            self.assertEqual(artifact["format"], "mono packed 24-bit PCM WAV")
            self.assertEqual(artifact["sample_count"], len(samples))

    def test_level_matching_uses_one_exact_target_and_preserves_headroom(self):
        baseline = [0.001] * 10000
        baseline[5000] = 1.0
        candidate = [0.35 * math.sin(index * 0.019) for index in range(10000)]
        matched_baseline, matched_candidate, receipt = blp._level_match_pair(
            baseline, candidate
        )
        baseline_rms = blp._rms(matched_baseline)
        candidate_rms = blp._rms(matched_candidate)
        self.assertAlmostEqual(baseline_rms, candidate_rms, places=15)
        self.assertAlmostEqual(baseline_rms, receipt["applied_target_rms"], places=15)
        peak_limit = 10.0 ** (blp.MAX_PEAK_DBFS / 20.0)
        self.assertLessEqual(blp._peak(matched_baseline), peak_limit)
        self.assertLessEqual(blp._peak(matched_candidate), peak_limit)
        self.assertLess(receipt["applied_target_rms_dbfs"], blp.REQUESTED_RMS_DBFS)

        with tempfile.TemporaryDirectory() as temporary:
            a = blp._write_pcm24_wav(Path(temporary) / "A.wav", matched_baseline)
            b = blp._write_pcm24_wav(Path(temporary) / "B.wav", matched_candidate)
        # Independent 24-bit rounding can differ by a fraction of one LSB; it
        # must not be "fixed" by injecting energy into otherwise silent samples.
        self.assertLess(abs(a["rms"] - b["rms"]), 1.0 / blp.PCM24_SCALE)
        self.assertLessEqual(a["peak"], peak_limit)
        self.assertLessEqual(b["peak"], peak_limit)

    def test_rejects_malformed_wrong_length_and_nonfinite_raw(self):
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            malformed = root / "malformed.f32"
            malformed.write_bytes(b"abc")
            with self.assertRaisesRegex(ValueError, "malformed"):
                blp._read_raw_f32(malformed)

            wrong_length = root / "wrong-length.f32"
            wrong_length.write_bytes(struct.pack("<2f", 0.0, 1.0))
            with self.assertRaisesRegex(ValueError, "expected 3"):
                blp._read_raw_f32(wrong_length, expected_samples=3)

            nonfinite = root / "nonfinite.f32"
            nonfinite.write_bytes(struct.pack("<3f", 0.0, float("nan"), 1.0))
            with self.assertRaisesRegex(ValueError, "non-finite sample at index 1"):
                blp._read_raw_f32(nonfinite)

    def test_full_build_is_published_only_when_complete_and_rejects_existing_output(self):
        with tempfile.TemporaryDirectory() as temporary:
            root=Path(temporary)
            renderer=root/"renderer.py"
            renderer.write_text(
                "#!/usr/bin/env python3\n"
                "import shutil,sys\n"
                "shutil.copyfile(sys.argv[2],sys.argv[3])\n",
                encoding="utf-8",
            )
            renderer.chmod(0o755)
            output=root/"pack"
            manifest=blp.build_pack(renderer,renderer,renderer,output)
            self.assertEqual(len(manifest["cells"]),len(blp.CELLS))
            self.assertEqual({path.name for path in output.iterdir()},blp._expected_artifact_names())
            with self.assertRaisesRegex(ValueError,"already exists"):
                blp.build_pack(renderer,renderer,renderer,output)

            failing=root/"failing.py"
            failing.write_text("#!/usr/bin/env python3\nraise SystemExit(7)\n",encoding="utf-8")
            failing.chmod(0o755)
            failed_output=root/"failed-pack"
            with self.assertRaisesRegex(RuntimeError,"failed"):
                blp.build_pack(renderer,renderer,failing,failed_output)
            self.assertFalse(failed_output.exists())


if __name__ == "__main__":
    unittest.main()
