#!/usr/bin/env python3
"""Generate deterministic, repository-owned reference-capture stimuli."""
from __future__ import annotations

import argparse, hashlib, json, math, random, struct, wave
from pathlib import Path

RATE = 48000


def write_wav(path: Path, samples: list[float], rate: int) -> str:
    pcm = b"".join(struct.pack("<h", max(-32768, min(32767, round(x * 32767)))) for x in samples)
    with wave.open(str(path), "wb") as out:
        out.setparams((1, 2, rate, len(samples), "NONE", "not compressed")); out.writeframes(pcm)
    return hashlib.sha256(path.read_bytes()).hexdigest()


def sine(rate: int, seconds: float, hz: float, amp: float = .5) -> list[float]:
    return [amp * math.sin(2 * math.pi * hz * n / rate) for n in range(round(rate * seconds))]


def sources(rate: int) -> dict[str, list[float]]:
    rng = random.Random(2601)
    impulse = [0.0] * rate; impulse[round(.1 * rate)] = .8
    sweep = [0.5 * math.sin(2 * math.pi * 20 * ((1000 ** (n / (rate * 2))) - 1) / math.log(1000)) for n in range(rate * 2)]
    pinkish = [0.0] * rate; state = 0.0
    for n in range(rate):
        state = .98 * state + .02 * rng.uniform(-1, 1); pinkish[n] = state * (1 if n < rate // 4 else 0)
    stepped = [v for a in (.05, .15, .3, .6) for v in sine(rate, .25, 440, a)]
    pluck = [math.sin(2*math.pi*110*n/rate) * math.exp(-n/(rate*.18)) * .7 for n in range(rate)]
    mute = [rng.uniform(-.5,.5) * math.exp(-n/(rate*.025)) for n in range(rate//2)] + [0.0]*(rate//2)
    chord = [sum(math.sin(2*math.pi*f*n/rate) for f in (110,138.59,164.81))/6 for n in range(rate*2)]
    riff_silence = [v for f in (110,146.83,164.81,220) for v in sine(rate,.125,f,.35)] + [0.0]*(rate//2)
    windows = [(.7 if (n//(rate//4))%2 == 0 else 0.0) * math.sin(2*math.pi*220*n/rate) for n in range(rate*2)]
    return {"impulse":impulse,"sweep":sweep,"pink_noise_burst":pinkish,"stepped_sine":stepped,
            "guitar_pluck":pluck,"palm_mute":mute,"sustained_chord":chord,
            "riff_then_silence":riff_silence,"controller_windows":windows}


def main() -> int:
    p=argparse.ArgumentParser(); p.add_argument("output", type=Path); p.add_argument("--sample-rate",type=int,default=RATE); a=p.parse_args()
    a.output.mkdir(parents=True,exist_ok=True); manifest={"schema_version":1,"sample_rate":a.sample_rate,"generator_seed":2601,"files":{}}
    for name, data in sources(a.sample_rate).items():
        path=a.output/f"{name}.wav"; manifest["files"][path.name]={"frames":len(data),"sha256":write_wav(path,data,a.sample_rate)}
    (a.output/"manifest.json").write_text(json.dumps(manifest,indent=2,sort_keys=True)+"\n")
    return 0
if __name__ == "__main__": raise SystemExit(main())
