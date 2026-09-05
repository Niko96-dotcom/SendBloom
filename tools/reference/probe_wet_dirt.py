#!/usr/bin/env python3
"""Spectral measurements of the shipping wet dirt; no hardware reference claim."""
import argparse,json,hashlib,subprocess
from pathlib import Path
import numpy as np

def db(x):return float(20*np.log10(max(float(x),1e-15)))
def run(binary,out):
 out.mkdir(parents=True,exist_ok=False);rows=[]
 for rate in [44100,48000,96000,192000]:
  t=np.arange(rate+rate//4)/rate
  for freq in [997,4001,7001,10003]:
   for level in [.03,.3,.8,1.5]:
    name=f'{rate}-{freq}-{level}';x=(level*np.sin(2*np.pi*freq*t)).astype('<f4');ip=out/(name+'-in.f32');op=out/(name+'-out.f32');ip.write_bytes(x.tobytes())
    subprocess.run([str(binary.resolve()),'dirt',str(ip.resolve()),str(op.resolve()),'1',str(rate)],check=True,capture_output=True)
    y=np.fromfile(op,dtype='<f4')[-rate:].astype(np.float64);spec=np.fft.rfft(y)*2/rate
    wanted=np.zeros(len(spec),dtype=bool);wanted[0]=True
    for h in range(1,rate//(2*freq)+1):wanted[h*freq]=True
    alias=np.sqrt(np.sum(abs(spec[~wanted])**2));fund=abs(spec[freq])
    rows.append(dict(rate=rate,freq=freq,level=level,fundamental_gain_db=db(fund/level),alias_dbc=db(alias/fund),
                     alias_dbfs=db(alias),harmonics_dbfs={str(h):db(abs(spec[h*freq])) for h in range(1,rate//(2*freq)+1)},
                     input_sha256=hashlib.sha256(ip.read_bytes()).hexdigest(),output_sha256=hashlib.sha256(op.read_bytes()).hexdigest()))
    ip.unlink();op.unlink()
 (out/'metrics.json').write_text(json.dumps(dict(binary=str(binary.resolve()),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
    protocol='1.25s sine at integer Hz; discard first .25s; rectangular 1s DFT; energy outside representable integer harmonics is folded energy plus numerical residual',rows=rows),indent=2)+'\n')
 for row in rows:
  if row['rate']==48000:print(row['freq'],row['level'],'alias dBc',round(row['alias_dbc'],2),'gain dB',round(row['fundamental_gain_db'],2))
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('binary',type=Path);p.add_argument('out',type=Path);a=p.parse_args();run(a.binary,a.out)
