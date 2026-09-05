#!/usr/bin/env python3
"""Build an immutable local corpus, render production builds, and prepare A/B audio.
Requires numpy/scipy. No hardware reference audio or model fitting is involved.
"""
import argparse
import hashlib
import io
import json
from pathlib import Path
import subprocess
import wave
import numpy as np
from scipy.io import wavfile

RATE = 48000
PARAMS = dict(input_gain=.5, input_threshold=.5, size=.7, level=.8, distn=.6,
              output_gain=0, dark_mode=0, gate_pre_post=0, send_connected=0,
              send_amount=0, send_feel=0, extended_stereo=0, bypass=0)

def sha(data):
    return hashlib.sha256(data).hexdigest()

def dump(path, data):
    path.write_text(json.dumps(data, indent=2) + '\n')

def rms(x):
    return float(np.sqrt(np.mean(np.square(x.astype(np.float64)))))

def db(x):
    return float(20 * np.log10(max(float(x), 1e-15)))

def prepare(root, manifest):
    dest = root / 'inputs'
    dest.mkdir(parents=True, exist_ok=False)
    sources = []
    def add(name, x, provenance):
        # 250ms lead-in settles the gate; eight seconds captures the long tail.
        x = np.concatenate([np.zeros(RATE // 4), x, np.zeros(8 * RATE)]).astype('<f4')
        path = dest / (name + '.f32')
        path.write_bytes(x.tobytes())
        sources.append(dict(name=name, path=str(path.resolve()), sha256=sha(path.read_bytes()),
                            samples=len(x), rate=RATE, channels=1, provenance=provenance))
    t = np.arange(4 * RATE) / RATE
    chord = sum(np.sin(2*np.pi*f*t) for f in [110, 164.81, 220, 277.18, 329.63]) / 5
    add('sustained-chord', .4 * chord, {'kind': 'generated', 'frequencies_hz': [110,164.81,220,277.18,329.63]})
    add('quiet-decay', .012 * np.sin(2*np.pi*110*t) * np.exp(-t), {'kind':'generated', 'peak':.012})
    pulse = np.zeros(4 * RATE)
    for start in [0, 1, 2, 3]:
        n = int(.12*RATE)
        p = np.arange(n)/RATE
        pulse[start*RATE:start*RATE+n] = .7*np.exp(-35*p)*(np.sin(2*np.pi*82.41*p)+.25*np.sin(2*np.pi*1236*p))/1.25
    add('transients', pulse, {'kind':'generated', 'peak_ceiling':.7})
    impulse = np.zeros(2*RATE); impulse[0] = .5
    add('impulse', impulse, {'kind':'generated', 'amplitude':.5})
    add('sustained-tone', .25*np.sin(2*np.pi*220*t), {'kind':'generated', 'frequency_hz':220})
    if manifest:
        ledger = json.loads(manifest.read_text())
        names = ['freepats-soft-single-notes-low-mid-high', 'guitar-techs-p3-open-chords',
                 'guitar-techs-p3-dense-barre-chord', 'guitar-techs-p3-slow-arpeggio', 'guitar-techs-p3-fast-strums']
        for name in names:
            entry = next(e for e in ledger['entries'] if e['clipId'] == name)
            path = Path(entry['audioPath']); raw = path.read_bytes()
            assert sha(raw) == entry['audioIntegrity']['sha256'], path
            sr, x = wavfile.read(io.BytesIO(raw))
            assert sr == RATE
            if np.issubdtype(x.dtype, np.integer):
                x = x.astype(np.float64)/2**(x.dtype.itemsize*8-1)
            if x.ndim > 1:
                x = x.mean(axis=1)
            add(name, x, dict(kind='licensed-real-di', source_path=str(path), source_sha256=sha(raw),
                             source_inode=path.stat().st_ino, license=entry['licenseTag'],
                             details=entry['provenance'], processing='channel mean if stereo; original gain; 250ms lead and 8s silence'))
    scenarios = [
        ('bright-clean', dict(size=.5, distn=0), []),
        ('dark-long-dirty', dict(size=1, dark_mode=1, distn=1), []),
        ('small-post', dict(size=0, gate_pre_post=1), []),
        ('pre-pressure-release', dict(send_connected=1), [{'sample':12000,'value':127},{'sample':int(1.75*RATE),'value':0}]),
        ('post-pressure-release', dict(send_connected=1,gate_pre_post=1), [{'sample':12000,'value':127},{'sample':int(1.75*RATE),'value':0}]),
        ('post-soft-repress', dict(send_connected=1,gate_pre_post=1,send_feel=1,dark_mode=1),
         [{'sample':12000,'value':127},{'sample':int(1.75*RATE),'value':0},{'sample':int(3*RATE),'value':127}]),
    ]
    configs = root / 'settings'; configs.mkdir()
    cells = []
    for source in sources:
        for name, overrides, events in scenarios:
            config = dict(rate=RATE, block=128, parameters=PARAMS | overrides, cc1=events)
            key = source['name'] + '__' + name
            settings = configs / (key + '.json'); dump(settings, config)
            cells.append(dict(id=key, input=source['path'], input_sha256=source['sha256'], settings=str(settings.resolve()),
                              settings_sha256=sha(settings.read_bytes())))
    dump(root / 'corpus.json', dict(sources=sources,cells=cells,truth='Local test inputs; no hardware wet references'))

def render(root, binary, label):
    dest = root / label / 'renders'; dest.mkdir(parents=True,exist_ok=False)
    rows = []
    for cell in json.loads((root/'corpus.json').read_text())['cells']:
        assert sha(Path(cell['input']).read_bytes()) == cell['input_sha256']
        assert sha(Path(cell['settings']).read_bytes()) == cell['settings_sha256']
        output = dest/(cell['id']+'.f32')
        cmd = [str(binary.resolve()), cell['input'], str(output.resolve()), cell['settings']]
        run = subprocess.run(cmd,check=True,capture_output=True,text=True)
        data = output.read_bytes(); x = np.frombuffer(data,dtype='<f4'); assert np.isfinite(x).all()
        repeat = dest/(cell['id']+'.repeat.f32')
        subprocess.run([str(binary.resolve()),cell['input'],str(repeat.resolve()),cell['settings']],check=True,capture_output=True)
        assert data == repeat.read_bytes(), cell['id']
        repeat.unlink()
        rows.append(dict(id=cell['id'],command=cmd,sha256=sha(data),rms_dbfs=db(rms(x)),peak_dbfs=db(np.max(np.abs(x))),
                         deterministic=True,repeat_count=2,receipt=json.loads(run.stdout)))
    dump(root/label/'render-receipt.json',dict(binary=str(binary.resolve()),binary_sha256=sha(binary.read_bytes()),cells=rows))

def compare(root):
    dest = root/'listening'; dest.mkdir(exist_ok=False)
    before = json.loads((root/'baseline/render-receipt.json').read_text())
    after = json.loads((root/'candidate/render-receipt.json').read_text())
    assert len(before['cells']) == len(after['cells'])
    rows = []
    corpus = {c['id']: c for c in json.loads((root/'corpus.json').read_text())['cells']}
    for a,b in zip(before['cells'],after['cells']):
        assert a['id'] == b['id']
        name=a['id']
        before_bytes=(root/'baseline/renders'/(name+'.f32')).read_bytes()
        after_bytes=(root/'candidate/renders'/(name+'.f32')).read_bytes()
        assert sha(before_bytes)==a['sha256'] and sha(after_bytes)==b['sha256']
        x=np.frombuffer(before_bytes,dtype='<f4'); y=np.frombuffer(after_bytes,dtype='<f4')
        assert x.shape == y.shape and np.isfinite(x).all() and np.isfinite(y).all()
        cell = corpus[name]
        input_bytes=Path(cell['input']).read_bytes()
        assert sha(input_bytes)==cell['input_sha256']
        dry_input = np.frombuffer(input_bytes, dtype='<f4')
        latency = a['receipt']['latency_samples']
        assert latency == b['receipt']['latency_samples']
        dry = np.concatenate([np.zeros(latency, dtype=np.float32), dry_input])[:len(x)]
        wet_before, wet_after = x-dry, y-dry
        release_window = slice(int(1.95*RATE), int(2.95*RATE))
        rows.append(dict(id=name,byte_identical=a['sha256']==b['sha256'],max_abs_change=float(np.max(np.abs(x-y))),
                         before_rms_dbfs=a['rms_dbfs'],after_rms_dbfs=b['rms_dbfs'],
                         before_peak_dbfs=a['peak_dbfs'],after_peak_dbfs=b['peak_dbfs'],
                         wet_rms_200_to_1200ms_after_release_before_dbfs=db(rms(wet_before[release_window])),
                         wet_rms_200_to_1200ms_after_release_after_dbfs=db(rms(wet_after[release_window]))))
        if name == 'sustained-tone__post-pressure-release':
            # Last non-null sample in a continuously played, single-release case.
            nz = np.flatnonzero(np.abs(wet_after) > 1e-7)
            rows[-1]['post_release_last_wet_ms'] = float(nz[-1]/RATE-1.75)*1000
    # Full-clip RMS matched examples plus unchanged-gain pairs.  Matching a gated
    # excerpt changes its retained attacks' gain, so absolute pairs are essential.
    selected=['sustained-chord__post-pressure-release','guitar-techs-p3-slow-arpeggio__post-pressure-release',
              'guitar-techs-p3-fast-strums__post-soft-repress','freepats-soft-single-notes-low-mid-high__pre-pressure-release']
    packs=[]
    for name in selected:
        if not any(r['id']==name for r in rows): continue
        x=np.fromfile(root/'baseline/renders'/(name+'.f32'),dtype='<f4')
        y=np.fromfile(root/'candidate/renders'/(name+'.f32'),dtype='<f4')
        # Retain first 7s for quick listening; original full-length raw renders persist.
        x=x[:7*RATE];y=y[:7*RATE]
        target=.1; gains=[target/rms(x),target/rms(y)]
        peak=max(np.max(np.abs(x*gains[0])),np.max(np.abs(y*gains[1])))
        headroom=min(1.,10**(-1/20)/max(peak,1e-15))
        for version,z,g in [('before',x,gains[0]),('after',y,gains[1])]:
            for mode,gain in [('matched',g*headroom),('absolute',1.)]:
                file=dest/(name+'__'+version+'__'+mode+'.wav')
                if mode == 'matched':
                    pcm=np.rint(z.astype(np.float64)*gain*8388608).astype(np.int32)
                    assert np.max(np.abs(pcm)) < 8388608
                    payload=((pcm[:,None] >> np.array([0,8,16])) & 255).astype(np.uint8).tobytes()
                    with wave.open(str(file),'wb') as w:
                        w.setnchannels(1);w.setsampwidth(3);w.setframerate(RATE);w.writeframes(payload)
                    measured=pcm/8388608
                else:
                    # Float WAV keeps original gain intact, including peaks above 0dBFS.
                    measured=(z*gain).astype(np.float32)
                    wavfile.write(file,RATE,measured)
                packs.append(dict(file=file.name,version=version,mode=mode,gain_db=db(gain),rms_dbfs=db(rms(measured)),peak_dbfs=db(np.max(np.abs(measured))),sha256=sha(file.read_bytes())))
    dump(root/'comparison.json',dict(cells=rows,listening=packs,matching='7s full-clip RMS; common attenuation if needed for -1dBFS; absolute copies gain=1'))
    html=['<!doctype html><meta charset="utf-8"><title>SendBloom fidelity comparison</title>',
          '<style>body{font:16px system-ui;max-width:960px;margin:40px auto;background:#f4f3ef;color:#202830}section{padding:18px;margin:20px 0;background:white;border-radius:12px}audio{width:100%}p{line-height:1.5}</style>',
          '<h1>SendBloom — pressure release</h1><p>Before = 95a3012 DSP. After = post-gate detector follows the pressure send. Igor is pressed at 0.25s and released at 1.75s while dry playing continues. Soft-repress presses again at 3s. Pre is an unchanged control. These are plugin comparisons, not hardware recordings.</p>',
          '<p>Start with RMS-matched pairs. Also check original gain: removing a tail lowers whole-clip RMS, so loudness matching attenuates the baseline attacks. No amp/cab processing. Playback starts only when you press play.</p>']
    for name in selected:
        matching=[p for p in packs if p['file'].startswith(name+'__')]
        if not matching: continue
        html += ['<section><h2>'+name+'</h2>']
        for p in matching:
            html += [f'<p>{p["version"].title()} — {p["mode"]}; gain {p["gain_db"]:.2f} dB</p><audio controls preload="none" src="{p["file"]}"></audio>']
        html += ['</section>']
    html+=['<p>Real DI: Guitar-TECHS authors and P3 performer, CC BY 4.0, Zenodo 14963133; FreePats FSBS recorder, CC0. Local source provenance and trims in ../corpus.json. No third-party hardware demo audio included.</p>']
    (dest/'index.html').write_text('\n'.join(html))
    print(json.dumps(dict(cells=len(rows),identical=sum(r['byte_identical'] for r in rows),changed=sum(not r['byte_identical'] for r in rows))))

if __name__=='__main__':
    p=argparse.ArgumentParser(); p.add_argument('mode',choices=['prepare','render','compare']);p.add_argument('root',type=Path)
    p.add_argument('--di-manifest',type=Path);p.add_argument('--binary',type=Path);p.add_argument('--label',choices=['baseline','candidate'])
    a=p.parse_args();a.root.mkdir(parents=True,exist_ok=True)
    if a.mode=='prepare':prepare(a.root,a.di_manifest)
    elif a.mode=='render':
        if not a.binary or not a.label:p.error('render requires --binary and --label')
        render(a.root,a.binary,a.label)
    else:compare(a.root)
