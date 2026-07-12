#!/usr/bin/env python3
"""Deterministic PCM WAV metrics for SendBloom clean-room captures."""
from __future__ import annotations
import argparse, csv, json, math, struct, wave
from pathlib import Path

def read_wav(path: Path):
    with wave.open(str(path),"rb") as w:
        if w.getsampwidth()!=2: raise ValueError("only 16-bit PCM WAV is supported")
        rate, channels, frames=w.getframerate(),w.getnchannels(),w.readframes(w.getnframes())
    ints=struct.unpack("<"+"h"*(len(frames)//2),frames)
    return rate,[sum(ints[i:i+channels])/channels/32768 for i in range(0,len(ints),channels)]

def goertzel(x, rate, hz):
    coeff=2*math.cos(2*math.pi*hz/rate); s0=s1=s2=0.0
    for v in x: s0=v+coeff*s1-s2; s2,s1=s1,s0
    return max(0.0,s1*s1+s2*s2-coeff*s1*s2)

def regression_time(times, db, lo, hi):
    pts=[(t,y) for t,y in zip(times,db) if hi<=y<=lo]
    if len(pts)<2:return None
    mt=sum(t for t,_ in pts)/len(pts); my=sum(y for _,y in pts)/len(pts)
    den=sum((t-mt)**2 for t,_ in pts); slope=sum((t-mt)*(y-my) for t,y in pts)/den if den else 0
    return (-60/slope) if slope<0 else None

def analyze(samples, rate, fundamental=440.0):
    peak=max((abs(x) for x in samples),default=0); threshold=peak*10**(-40/20)
    onset=next((i for i,x in enumerate(samples) if abs(x)>=threshold),None)
    dc=sum(samples)/len(samples) if samples else 0
    hop=max(1,round(rate*.01)); rms=[]; times=[]; cent=[]
    for start in range(0,len(samples),hop):
        frame=samples[start:start+hop]; e=math.sqrt(sum(x*x for x in frame)/max(1,len(frame))); rms.append(e); times.append(start/rate)
        powers=[goertzel(frame,rate,k*rate/len(frame)) for k in range(1,max(2,len(frame)//2))] if frame else []
        total=sum(powers); cent.append(sum((k+1)*rate/len(frame)*p for k,p in enumerate(powers))/total if total else 0)
    mx=max(rms,default=0); db=[20*math.log10(max(v,1e-12)/mx) if mx else -240 for v in rms]
    peak_i=max(range(len(rms)),key=rms.__getitem__) if rms else 0
    close=next((times[i] for i in range(peak_i,len(rms)) if db[i]<=-40),None)
    h1=goertzel(samples,rate,fundamental); harmonics={}
    for h in range(2,6):
        p=goertzel(samples,rate,fundamental*h); harmonics[str(h)]=10*math.log10(max(p,1e-30)/max(h1,1e-30))
    return {"predelay_ms":None if onset is None else onset*1000/rate,"edt_s":regression_time(times,db,0,-10),
      "rt20_s":regression_time(times,db,-5,-25),"rt30_s":regression_time(times,db,-5,-35),
      "spectral_centroid_hz":cent,"harmonics_db":harmonics,"dc_offset":dc,
      "gate_close_ms":None if close is None else (close-times[peak_i])*1000,"peak":peak}

def main():
    p=argparse.ArgumentParser(); p.add_argument("wav",type=Path); p.add_argument("metadata",type=Path); p.add_argument("--json",type=Path,required=True); p.add_argument("--csv",type=Path,required=True); p.add_argument("--fundamental",type=float,default=440); a=p.parse_args()
    meta=json.loads(a.metadata.read_text()); rate,x=read_wav(a.wav)
    result={"schema_version":1,"capture_id":meta["capture_id"],"sample_rate":rate,"source_wav":a.wav.name,
            "capture_metadata":meta.get("capture_metadata",{}),"settings":meta.get("settings",{}),**analyze(x,rate,a.fundamental)}
    a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(json.dumps(result,indent=2,sort_keys=True)+"\n")
    row={k:(json.dumps(v,sort_keys=True) if isinstance(v,(dict,list)) else v) for k,v in result.items()}
    with a.csv.open("w",newline="") as f: w=csv.DictWriter(f,fieldnames=list(row)); w.writeheader(); w.writerow(row)
    return 0
if __name__=="__main__": raise SystemExit(main())
