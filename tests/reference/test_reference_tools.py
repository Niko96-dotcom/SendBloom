import hashlib, json, math, subprocess, sys, tempfile, unittest, wave, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]; sys.path.insert(0,str(ROOT/"tools/reference"))
import analyze_reference as ar

class ReferenceToolsTest(unittest.TestCase):
    def test_known_dc_predelay_and_harmonics(self):
        rate=8000; x=[0.0]*800+[.2+0.5*math.sin(2*math.pi*400*n/rate) for n in range(rate)]
        m=ar.analyze(x,rate,400)
        self.assertAlmostEqual(m["predelay_ms"],100,delta=1); self.assertGreater(m["dc_offset"],.17)
        self.assertLess(m["harmonics_db"]["2"],-20)

    def test_generator_is_byte_deterministic(self):
        with tempfile.TemporaryDirectory() as td:
            a,b=Path(td)/"a",Path(td)/"b"
            for out in (a,b): subprocess.run([sys.executable,str(ROOT/"tools/reference/generate_sources.py"),str(out),"--sample-rate","8000"],check=True)
            self.assertEqual(json.loads((a/"manifest.json").read_text())["files"],json.loads((b/"manifest.json").read_text())["files"])

    def test_cli_preserves_metadata_and_emits_csv(self):
        with tempfile.TemporaryDirectory() as td:
            d=Path(td); wav=d/"x.wav"; rate=8000; x=[0]*80+[round(12000*math.sin(2*math.pi*400*n/rate)) for n in range(800)]
            with wave.open(str(wav),"wb") as w: w.setparams((1,2,rate,len(x),"NONE","")); w.writeframes(struct.pack("<"+"h"*len(x),*x))
            meta=d/"m.json"; meta.write_text(json.dumps({"capture_id":"c1","capture_metadata":{"operator":"Niko"},"settings":{"size_pct":50}}))
            j,c=d/"out.json",d/"out.csv"; subprocess.run([sys.executable,str(ROOT/"tools/reference/analyze_reference.py"),str(wav),str(meta),"--json",str(j),"--csv",str(c),"--fundamental","400"],check=True)
            out=json.loads(j.read_text()); self.assertEqual(out["settings"]["size_pct"],50); self.assertIn("spectral_centroid_hz",out); self.assertTrue(c.read_text().startswith("schema_version,"))

if __name__=="__main__": unittest.main()
