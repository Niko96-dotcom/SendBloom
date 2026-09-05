#!/usr/bin/env python3
"""Compare preserved processor renders across a declared PDC change.

Usage: python3 tools/reference/compare_wet_antialias.py RUN_DIRECTORY
RUN_DIRECTORY comes from fidelity_compare.py prepare/render (both builds).
Requires numpy/scipy. Does not modify sources or normalize raw renders.
"""
import argparse
import json
from pathlib import Path
import wave
import numpy as np
from scipy.io import wavfile
from fidelity_compare import db, dump, rms, sha


def compare(root):
    dest = root / 'listening'
    dest.mkdir(exist_ok=False)
    receipts = [json.loads((root / label / 'render-receipt.json').read_text())
                for label in ['baseline', 'candidate']]
    corpus = json.loads((root / 'corpus.json').read_text())
    maps = [{c['id']: c for c in receipt['cells']} for receipt in receipts]
    assert set(maps[0]) == set(maps[1]) == {c['id'] for c in corpus['cells']}
    selected = {
        'guitar-techs-p3-fast-strums__dark-long-dirty',
        'guitar-techs-p3-dense-barre-chord__dark-long-dirty',
        'guitar-techs-p3-slow-arpeggio__small-post',
        'quiet-decay__dark-long-dirty',
        'sustained-tone__bright-clean',
    }
    rows, packs, sections = [], [], []
    for cell in corpus['cells']:
        name = cell['id']
        settings_bytes = Path(cell['settings']).read_bytes()
        assert sha(settings_bytes) == cell['settings_sha256']
        config = json.loads(settings_bytes)
        rate = config['rate']
        # All supplied cells have unity master and the unchanged direct path.
        assert config['parameters']['output_gain'] == 0
        assert config['parameters']['bypass'] == 0
        raw_input = Path(cell['input']).read_bytes()
        assert sha(raw_input) == cell['input_sha256']
        dry = np.frombuffer(raw_input, dtype='<f4')
        rendered, delays = [], []
        for label, ledger in zip(['baseline', 'candidate'], maps):
            entry = ledger[name]
            data = (root / label / 'renders' / (name + '.f32')).read_bytes()
            assert sha(data) == entry['sha256']
            y = np.frombuffer(data, dtype='<f4')
            assert len(y) == len(dry) and np.isfinite(y).all()
            assert entry['receipt']['rate'] == rate
            delay = entry['receipt']['latency_samples']
            delays.append(delay)
            rendered.append(y[delay:].astype(np.float64))
        count = min(map(len, rendered))
        x, y = [z[:count] for z in rendered]
        wx, wy = x-dry[:count], y-dry[:count]
        rows.append(dict(id=name,latency_before=delays[0],latency_after=delays[1],
                         full_before_dbfs=db(rms(x)),full_after_dbfs=db(rms(y)),
                         wet_before_dbfs=db(rms(wx)),wet_after_dbfs=db(rms(wy)),
                         full_peak_before_dbfs=db(max(abs(x))),full_peak_after_dbfs=db(max(abs(y))),
                         wet_rms_change_db=db(rms(wy))-db(rms(wx)),
                         aligned_max_abs_change=float(max(abs(x-y)))))
        if name not in selected:
            continue
        sections.append('<section><h2>'+name+'</h2>')
        for route, pair in [('plugin output', (x,y)), ('isolated wet', (wx,wy))]:
            a,b = [z[:12*rate] for z in pair]
            gains = [.1/max(rms(z),1e-15) for z in [a,b]]
            peak = max(max(abs(a*gains[0])), max(abs(b*gains[1])))
            headroom = min(1.,10**(-1/20)/max(peak,1e-15))
            sections.append('<details'+(' open' if route == 'plugin output' else '')+'><summary>'+route+'</summary>')
            for version, z, gain in zip(['before','after'], [a,b], gains):
                for mode, g in [('matched',gain*headroom),('absolute',1.)]:
                    filename = name+'__'+route.replace(' ','-')+'__'+version+'__'+mode+'.wav'
                    file = dest / filename
                    if mode == 'matched':
                        pcm = np.rint(z*g*8388608).astype(np.int32)
                        assert max(abs(pcm)) < 8388608
                        payload = ((pcm[:,None] >> np.array([0,8,16])) & 255).astype(np.uint8).tobytes()
                        with wave.open(str(file),'wb') as w:
                            w.setnchannels(1); w.setsampwidth(3); w.setframerate(rate); w.writeframes(payload)
                        measured = pcm/8388608
                    else:
                        measured = z.astype(np.float32)
                        wavfile.write(file,rate,measured)
                    packs.append(dict(file=filename,route=route,version=version,mode=mode,
                                      gain_db=db(g),rms_dbfs=db(rms(measured)),
                                      peak_dbfs=db(max(abs(measured))),sha256=sha(file.read_bytes())))
                    sections.append(f'<p>{version.title()} / {mode}: {db(g):+.2f} dB trim</p><audio controls preload="none" src="{filename}"></audio>')
            sections.append('</details>')
        sections.append('</section>')
    dump(root/'comparison.json',dict(cells=rows,listening=packs,
        alignment='Drop each reported PDC; retain common length; no phase or tonal correction',
        matching='First 12s RMS at -20dBFS with common peak cap -1dBFS; PCM24. Absolute float WAV gain=1.',
        wet_extraction='Subtract unchanged unity direct input after individual PDC alignment; includes LEVEL.'))
    (dest/'index.html').write_text('''<!doctype html><meta charset="utf-8"><title>SendBloom distortion comparison</title>
<style>body{font:16px system-ui;max-width:960px;margin:40px auto;background:#f4f3ef;color:#202830}section{padding:18px;margin:20px 0;background:white;border-radius:12px}audio{width:100%}p{line-height:1.5}summary{cursor:pointer;font-weight:600;margin:16px 0}</style>
<h1>SendBloom — distortion reconstruction</h1>
<p>Before: pressure-corrected first pass. After: same tank, drive curve and controls, with 4x wet oversampling. These are plugin comparisons, not hardware recordings. No amp, cabinet, limiter or mastering.</p>
<p>Each version is aligned using its own reported latency. Start with matched plugin output; expand isolated wet to hear the return without dry masking. Absolute versions retain original gain. Matching is RMS over the displayed excerpt, not a perceptual loudness claim.</p>
<p>Dark/long/dirty examples use SIZE and DISTN maximum, Dark on. Small/Post uses SIZE minimum, DISTN 0.6, Post. Bright/clean sustained tone is a reconstruction control at DISTN zero. All settings are preserved in corpus.json and its linked settings files.</p>
'''+'\n'.join(sections)+'''
<p>Real DI attribution: Guitar-TECHS authors / P3 performer, CC BY 4.0, https://zenodo.org/records/14963133. Input provenance, trims and hashes are retained in ../corpus.json. Other inputs are generated test signals. No hardware demo was fitted or auditioned for this comparison.</p>''')
    print(json.dumps(dict(cells=len(rows),audio_files=len(packs),latency_change=rows[0]['latency_after']-rows[0]['latency_before'])))


if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('root',type=Path)
    compare(p.parse_args().root)
