#!/usr/bin/env python3
"""Deterministic PCM WAV metrics for SendBloom clean-room captures."""
from __future__ import annotations
import argparse, csv, hashlib, json, math, struct, wave
from pathlib import Path

def _decode_pcm_le(frames: bytes, sample_width: int):
    if sample_width == 2:
        return [sample[0] for sample in struct.iter_unpack("<h", frames)]
    if sample_width == 3:
        samples=[]
        for i in range(0,len(frames),3):
            sample=frames[i] | (frames[i+1]<<8) | (frames[i+2]<<16)
            if sample & 0x800000: sample-=1<<24
            samples.append(sample)
        return samples
    raise ValueError("only 16-bit or packed 24-bit PCM WAV is supported")

def read_wav(path: Path):
    try:
        with wave.open(str(path),"rb") as w:
            if w.getcomptype()!="NONE": raise ValueError("only uncompressed PCM WAV is supported")
            sample_width=w.getsampwidth()
            if sample_width not in (2,3): raise ValueError("only 16-bit or packed 24-bit PCM WAV is supported")
            rate, channels, declared_frames=w.getframerate(),w.getnchannels(),w.getnframes()
            if channels != 1: raise ValueError("capture WAV must be mono")
            frames=w.readframes(declared_frames)
    except wave.Error as exc:
        raise ValueError(f"unsupported or invalid PCM WAV: {exc}") from exc
    bytes_per_frame=sample_width*channels
    if len(frames) != declared_frames*bytes_per_frame: raise ValueError("truncated PCM WAV frame data")
    if len(frames)%bytes_per_frame: raise ValueError("truncated PCM WAV frame data")
    ints=_decode_pcm_le(frames,sample_width); scale=float(1<<(sample_width*8-1))
    return rate,[sum(ints[i:i+channels])/(channels*scale) for i in range(0,len(ints),channels)]

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

def first_sustained_close(db, start, threshold=-40.0, frames=5):
    for index in range(start, len(db)-frames+1):
        if all(value <= threshold for value in db[index:index+frames]):
            return index
    return None

def analyze(samples, rate, fundamental=440.0):
    if not samples: raise ValueError("capture audio is empty")
    peak=max(abs(x) for x in samples)
    if peak <= 0: raise ValueError("capture audio is silent")
    threshold=peak*10**(-40/20)
    onset=next((i for i,x in enumerate(samples) if abs(x)>=threshold),None)
    dc=sum(samples)/len(samples) if samples else 0
    hop=max(1,round(rate*.01)); rms=[]; times=[]; cent=[]
    for start in range(0,len(samples),hop):
        frame=samples[start:start+hop]; e=math.sqrt(sum(x*x for x in frame)/max(1,len(frame))); rms.append(e); times.append(start/rate)
        powers=[goertzel(frame,rate,k*rate/len(frame)) for k in range(1,max(2,len(frame)//2))] if frame else []
        total=sum(powers); cent.append(sum((k+1)*rate/len(frame)*p for k,p in enumerate(powers))/total if total else 0)
    mx=max(rms,default=0); db=[20*math.log10(max(v,1e-12)/mx) if mx else -240 for v in rms]
    peak_i=max(range(len(rms)),key=rms.__getitem__) if rms else 0
    close_i=first_sustained_close(db,peak_i)
    h1=goertzel(samples,rate,fundamental); harmonics={}
    for h in range(2,6):
        p=goertzel(samples,rate,fundamental*h); harmonics[str(h)]=10*math.log10(max(p,1e-30)/max(h1,1e-30))
    return {"predelay_ms":None if onset is None else onset*1000/rate,"edt_s":regression_time(times,db,0,-10),
      "rt20_s":regression_time(times,db,-5,-25),"rt30_s":regression_time(times,db,-5,-35),
      "spectral_centroid_hz":cent,"gate_envelope_db":db,"harmonics_db":harmonics,"dc_offset":dc,
      "gate_close_ms":None if close_i is None else (times[close_i]-times[peak_i])*1000,"peak":peak}

def main():
    p=argparse.ArgumentParser(); p.add_argument("wav",type=Path); p.add_argument("metadata",type=Path); p.add_argument("--json",type=Path,required=True); p.add_argument("--csv",type=Path,required=True); p.add_argument("--fundamental",type=float,default=440); a=p.parse_args()
    meta=json.loads(a.metadata.read_text()); rate,x=read_wav(a.wav)
    capture_metadata=dict(meta.get("capture_metadata",{}))
    declared_hash=capture_metadata.get("capture_sha256")
    if not isinstance(declared_hash,str) or len(declared_hash)!=64 or any(ch not in "0123456789abcdefABCDEF" for ch in declared_hash):
        raise ValueError("capture_metadata.capture_sha256 must be a 64-character hexadecimal SHA-256")
    actual_hash=hashlib.sha256(a.wav.read_bytes()).hexdigest()
    if declared_hash.lower()!=actual_hash:
        raise ValueError(f"capture SHA-256 mismatch: declared {declared_hash}, actual {actual_hash}")
    result={"schema_version":1,"capture_id":meta["capture_id"],"sample_rate":rate,"source_wav":a.wav.name,
            "source_wav_sha256":actual_hash,"capture_metadata":capture_metadata,"settings":meta.get("settings",{}),**analyze(x,rate,a.fundamental)}
    a.json.parent.mkdir(parents=True,exist_ok=True); a.json.write_text(json.dumps(result,indent=2,sort_keys=True)+"\n")
    row={k:(json.dumps(v,sort_keys=True) if isinstance(v,(dict,list)) else v) for k,v in result.items()}
    with a.csv.open("w",newline="") as f: w=csv.DictWriter(f,fieldnames=list(row)); w.writeheader(); w.writerow(row)
    return 0
if __name__=="__main__": raise SystemExit(main())
