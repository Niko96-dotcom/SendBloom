import hashlib, json, math, subprocess, sys, tempfile, unittest, wave, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]; sys.path.insert(0,str(ROOT/"tools/reference"))
import analyze_reference as ar

def write_pcm_wav(path, frames, sample_width, rate=8000):
    channels=len(frames[0]) if frames else 1
    if sample_width==2:
        payload=struct.pack("<"+"h"*(len(frames)*channels),*(sample for frame in frames for sample in frame))
    elif sample_width==3:
        payload=b"".join((sample<<8).to_bytes(3,"little",signed=True) for frame in frames for sample in frame)
    else:
        payload=b"\0"*(len(frames)*channels*sample_width)
    with wave.open(str(path),"wb") as w:
        w.setparams((channels,sample_width,rate,len(frames),"NONE","")); w.writeframes(payload)

class ReferenceToolsTest(unittest.TestCase):
    def test_reads_16_and_packed_24_bit_mono(self):
        source=[-32768,-12000,0,12000,32767]
        with tempfile.TemporaryDirectory() as td:
            for sample_width in (2,3):
                with self.subTest(sample_width=sample_width):
                    path=Path(td)/f"{sample_width}-mono.wav"
                    frames=[(sample,) for sample in source]
                    write_pcm_wav(path,frames,sample_width)
                    rate,decoded=ar.read_wav(path)
                    self.assertEqual(rate,8000)
                    self.assertEqual(decoded,[frame[0]/32768 for frame in frames])

    def test_equivalent_16_and_24_bit_wavs_produce_sane_metrics(self):
        rate=8000
        source=[0]*800+[round(16000*math.sin(2*math.pi*400*n/rate)) for n in range(rate)]
        frames=[(sample,) for sample in source]
        with tempfile.TemporaryDirectory() as td:
            paths=[Path(td)/"capture-16.wav",Path(td)/"capture-24.wav"]
            write_pcm_wav(paths[0],frames,2,rate); write_pcm_wav(paths[1],frames,3,rate)
            decoded=[ar.read_wav(path)[1] for path in paths]
            metrics=[ar.analyze(samples,rate,400) for samples in decoded]
            self.assertEqual(decoded[0],decoded[1])
            self.assertAlmostEqual(metrics[0]["predelay_ms"],100,delta=1)
            self.assertGreater(metrics[0]["peak"],.1)
            self.assertLess(abs(metrics[0]["dc_offset"]),1e-3)
            self.assertAlmostEqual(metrics[0]["predelay_ms"],metrics[1]["predelay_ms"],delta=.01)
            self.assertAlmostEqual(metrics[0]["peak"],metrics[1]["peak"],delta=1e-9)
            self.assertAlmostEqual(metrics[0]["harmonics_db"]["2"],metrics[1]["harmonics_db"]["2"],delta=1e-9)

    def test_rejects_unsupported_pcm_widths_cleanly(self):
        with tempfile.TemporaryDirectory() as td:
            for sample_width in (1,4):
                with self.subTest(sample_width=sample_width):
                    path=Path(td)/f"unsupported-{sample_width}.wav"
                    write_pcm_wav(path,[(0,)],sample_width)
                    with self.assertRaisesRegex(ValueError,"only 16-bit or packed 24-bit PCM WAV is supported"):
                        ar.read_wav(path)
            float_wav=Path(td)/"unsupported-float.wav"
            fmt=struct.pack("<HHIIHH",3,1,8000,32000,4,32)
            samples=struct.pack("<f",0.0)
            float_wav.write_bytes(b"RIFF"+struct.pack("<I",36+len(samples))+b"WAVEfmt "+struct.pack("<I",len(fmt))+fmt+b"data"+struct.pack("<I",len(samples))+samples)
            with self.assertRaisesRegex(ValueError,"unsupported or invalid PCM WAV"):
                ar.read_wav(float_wav)

    def test_rejects_multichannel_and_whole_frame_truncation(self):
        with tempfile.TemporaryDirectory() as td:
            directory=Path(td)
            stereo=directory/"stereo.wav"
            write_pcm_wav(stereo,[(1000,-1000)]*20,2)
            with self.assertRaisesRegex(ValueError,"must be mono"):
                ar.read_wav(stereo)

            truncated=directory/"truncated.wav"
            write_pcm_wav(truncated,[(1000,)]*20,2)
            truncated.write_bytes(truncated.read_bytes()[:-2])
            with self.assertRaisesRegex(ValueError,"truncated PCM WAV frame data"):
                ar.read_wav(truncated)

    def test_known_dc_predelay_and_harmonics(self):
        rate=8000; x=[0.0]*800+[.2+0.5*math.sin(2*math.pi*400*n/rate) for n in range(rate)]
        m=ar.analyze(x,rate,400)
        self.assertAlmostEqual(m["predelay_ms"],100,delta=1); self.assertGreater(m["dc_offset"],.17)
        self.assertLess(m["harmonics_db"]["2"],-20)

    def test_rejects_silence_and_requires_sustained_gate_close(self):
        with self.assertRaisesRegex(ValueError,"silent"):
            ar.analyze([0.0]*100,1000,100)

        signal=[0.5]*100+[0.0]*10+[0.5]*100+[0.0]*100
        metrics=ar.analyze(signal,1000,100)
        self.assertGreaterEqual(metrics["gate_close_ms"],200.0)

    def test_generator_is_byte_deterministic(self):
        with tempfile.TemporaryDirectory() as td:
            a,b=Path(td)/"a",Path(td)/"b"
            for out in (a,b): subprocess.run([sys.executable,str(ROOT/"tools/reference/generate_sources.py"),str(out),"--sample-rate","8000"],check=True)
            self.assertEqual(json.loads((a/"manifest.json").read_text())["files"],json.loads((b/"manifest.json").read_text())["files"])

    def test_cli_preserves_metadata_and_emits_csv(self):
        with tempfile.TemporaryDirectory() as td:
            d=Path(td); wav=d/"x.wav"; rate=8000; x=[0]*80+[round(12000*math.sin(2*math.pi*400*n/rate)) for n in range(800)]
            with wave.open(str(wav),"wb") as w: w.setparams((1,2,rate,len(x),"NONE","")); w.writeframes(struct.pack("<"+"h"*len(x),*x))
            wav_hash=hashlib.sha256(wav.read_bytes()).hexdigest()
            meta=d/"m.json"; meta.write_text(json.dumps({"capture_id":"c1","capture_metadata":{"operator":"Niko","capture_sha256":wav_hash},"settings":{"size_pct":50}}))
            j,c=d/"out.json",d/"out.csv"; subprocess.run([sys.executable,str(ROOT/"tools/reference/analyze_reference.py"),str(wav),str(meta),"--json",str(j),"--csv",str(c),"--fundamental","400"],check=True)
            out=json.loads(j.read_text()); self.assertEqual(out["settings"]["size_pct"],50); self.assertEqual(out["source_wav_sha256"],wav_hash); self.assertIn("spectral_centroid_hz",out); self.assertIn("gate_envelope_db",out); self.assertTrue(c.read_text().startswith("schema_version,"))

            meta.write_text(json.dumps({"capture_id":"c1","capture_metadata":{"operator":"Niko","capture_sha256":"0"*64}}))
            mismatch=subprocess.run([sys.executable,str(ROOT/"tools/reference/analyze_reference.py"),str(wav),str(meta),"--json",str(j),"--csv",str(c)],capture_output=True,text=True,check=False)
            self.assertNotEqual(mismatch.returncode,0)
            self.assertIn("capture SHA-256 mismatch",mismatch.stderr)

if __name__=="__main__": unittest.main()
