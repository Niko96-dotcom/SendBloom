# Why the reverb is an allpass ring

**Status:** implemented, v1.x; the 2026 two-allpass density candidate was rejected by interactive listening and is not the shipping topology
**Supersedes the tank design in:** ADR-002 (8-line FDN / Schroeder tank)

## Evidence correction — 2026-09-05

The ring is SendBloom's chosen reverb architecture. The previous explanation
incorrectly treated a chip vendor's design example as proof of the pedal's
internal topology. A limited RAM budget and a long decay do **not** force an
allpass ring, and the older comb topology cannot be ruled out from those facts.
Earlier ripple measurements characterize our earlier engine only.

The FV-1 attribution for the reference product is secondary reporting. Spin's
[demo-board documentation](https://www.spinsemi.com/knowledge_base/demo_board.html)
describes both 32.768 kHz and 46.6084 kHz crystals. Its recommended watch crystal
therefore does not establish the pedal's clock, firmware, or analog response.
SendBloom chooses 32,768 Hz and a nominal 32,768-word delay-memory budget; it
uses floating point and does not emulate the chip instruction set or converters.

Spin's [Effects article](https://www.spinsemi.com/knowledge_base/effects.html)
describes a ring example with two allpasses per block, but explicitly permits
one, three, or more, and different block counts. Its advice about loop length,
diffusion, shelving and modulation guides this design; it is neither a unique
solution nor a measurement of the original pedal. The 2026 two-allpass candidate
was rejected in listening, so the accepted single-allpass voicing remains.

See [the current evidence map](fidelity-20260905/RESEARCH.md) for sourced behavior,
implementation assumptions, and the independently reproduced pressure/Post gate
correction. No quantitative hardware-equivalence claim follows from this tank's
memory use, response measurements, or passing regression tests.

## What is implemented

`Fv1RingTank` / `Fv1RingTankTable`, at the fixed 32,768 Hz internal rate:

```
in ─┬─────────────────────────────┐  (Dark crossfades to a 55 ms predelay)
    └─ predelay ──────────────────┤
                                  ▼
                            ×0.25 headroom
                                  │
                    4 series allpasses (g = 0.6)
                       241, 443, 863, 1097
                                  │
   ┌──────────────────────────────┴─────────────────────────────┐
   │  ring, 4 blocks; input injected at blocks 1..3             │
   │                                                            │
   │   allpass(g=0.6) → delay                                  │
   │                  → LF-loss shelf → HF-loss shelf → × krt   │
   │                                                            │
   │   allpass   2311, 2909, 3167, 2417                        │
   │   delays    3623, 4597, 4391, 3671                         │
   └────────────────────────────────────────────────────────────┘
                                  │
        output = 0.6·del0[+1201] + 0.8·del1[+211]
               + 0.7·del2[+897]  + 0.5·del3[+1780]
```

The four plain ring delays remain prime. The ring allpass lengths are asymmetric
and non-uniform, with modulation distributed across all four loop blocks.

| | |
|---|---|
| Plain loop delay | 16,282 samples = **496.9 ms** (floor: 200 ms) |
| Full loop incl. allpasses | 27,086 samples = **826.6 ms** |
| Delay RAM used | 31,532 / 32,768 words = **96.2%** |

The RAM budget and the 200 ms floor are both `static_assert`ed in
`Fv1RingTankTable.h`, so a future retune cannot quietly reintroduce the old
failure.

### Decay

`krt` multiplies once per block, so one traversal costs `krt⁴`. Solving for a
target RT60 and correcting for in-loop damping loss gives the `krtForRT60`
mapping. The current ring has a practical short-decay floor: requested targets
of 0.25/0.40/0.60 s measure about 0.90/0.90/0.94 s Bright and
0.88/0.88/0.90 s Dark. Below roughly 0.9 s, feed-forward and allpass energy
dominate and lower feedback targets become nearly the same sound. A genuinely
shorter room would require a size-dependent diffuser or a separate topology,
with different density and headroom tradeoffs.

`ParameterCurves::sizeToRT60` therefore spans **1.2 s to 6.0 s** exponentially:
1.2 s is a SendBloom control-design choice that avoids a dead lower knob region,
while 6.0 s matches the reference manual's *"up to a maximum of 5 or 6
seconds"*. Public Reverb-X material does not establish its minimum decay.

### Damping

Each block carries two first-order shelves inside the loop:

- **LF loss**, corner 105 Hz. The old tank had none, so long settings piled up
  low end. This is what keeps a 6 s tail from turning to mud.
- **HF loss**, corner 11.5 kHz (Bright) down to 2.1 kHz (Dark), depth 0.45 → 0.88.

Dark additionally crossfades in a 55 ms predelay: *"bright and immediate, or dark
with pre-delay"*.

### Modulation

Two sine LFOs at 0.48 Hz and 0.60 Hz, sin and cos each, sweep the four ring
allpasses by ±4.6 / ±4.1 samples. The modulation is spread over four points to
break up ringing without audible pitch wobble.

## Measured, old vs new

Built binaries, impulse into each tank at 32,768 Hz (`tools/RenderTank`):

| | old (comb tank) | new (allpass ring) |
|---|---|---|
| Tail ripple, RT60 1.2 s | 28.2 Hz | 0.7 Hz |
| Tail ripple, RT60 6 s | 28.2 Hz | 6.9 Hz |
| T30 @ target 3.0 s | 2.87 s | 3.00 s |
| T30 @ target 6.0 s | 5.73 s | 5.66 s |
| Dark HF decay (T30 @ 4 kHz, RT 6 s) | 4.82 s | 1.89 s |
| Dark spectral tilt (2–6 k vs 200–700 Hz) | −67.6 dB | −9.9 dB |
| Predelay, Dark − Bright | 55 ms | 55.05 ms |

The old Dark was not dark, it was a lowpass filter — 68 dB down at 2–6 kHz left
almost nothing above a few hundred Hz.

`kOutputNormalisation` is calibrated so the wet return matches the previous
tank's level at RT60 3 s, so the Level curve, factory presets and clip LED
thresholds carry over unchanged.

### 2026-07 density candidate — rejected by listening

An offline candidate split each former allpass delay into an asymmetric serial
pair while preserving the loop total, RAM usage, taps, shelves, gain, and
predelay. Deterministic 32,768 Hz impulse renders at a 3.0 s target produced the
following objective direction:

| Metric | Single allpass/block | Two allpasses/block |
|---|---:|---:|
| Bright crest, 100–250 ms | 7.330 | 6.804 |
| Bright kurtosis, 100–250 ms | 11.575 | 6.063 |
| Dark crest, 100–250 ms | 7.047 | 6.925 |
| Dark kurtosis, 100–250 ms | 16.573 | 9.341 |
| Bright T30 | 3.002 s | 2.910 s |
| Dark T30 | 2.666 s | 2.578 s |
| Bright onset | 6.409 ms | 6.409 ms |
| Dark onset | 61.462 ms | 61.462 ms |
| Bright 0–4 s RMS change | baseline | −0.045 dB |
| Dark 0–4 s RMS change | baseline | −0.023 dB |

Lower early-window kurtosis was the intended directional density result, but the
interactive level-matched A/B screen preferred the single-allpass baseline in
both Bright and Dark cells. The candidate is therefore reverted; these remain
historical regression measurements, not measurements of Reverb-X hardware.

## The overdrive

Same source, same reasoning. Spin's notes name the nonlinearity for this
hardware class and say why:

> If X<1 then Y=X / If X>1 then Y=2 − 1/X ... The sound however, especially for
> a guitar and keyboard instruments, is very nice, and aliasing is largely
> avoided. This concept delivers *nice* distortion, that is, lower level signals
> are clean, and only 'break up' on transients and emphasized instrumental notes.

The shipping curve is flat at gain 1.10 up to |x| = 0.30, then bends: 0.93 at
0.5, 0.68 at 0.8, and 0.31 at 2.0. An offline 5× branch-normalized candidate
moved the knees to +0.182 / −0.200, but the interactive Dirt cells preferred the
baseline in both pluck and long-Dark-chord tests. That candidate is reverted.

## Provenance

Everything above comes from material Spin Semiconductor publishes for exactly
this purpose — the FV-1 datasheet, the knowledge-base design articles, and the
reference programs the datasheet offers as *"example code that may be freely used
in your product"*. Delay lengths here are our own prime values chosen inside the
ranges those references occupy and sized to the real RAM budget; no program was
copied.

No EEPROM was read, no firmware disassembled, and no specific product's program
was reverse engineered. See [CLEAN_ROOM.md](CLEAN_ROOM.md).

## Sources

- Spin Semiconductor, *FV-1 Reverb IC* datasheet, SPN1001-DS-120310
- Spin Semiconductor knowledge base, *Effects* — "Reverberation",
  "Considerations when building a reverb", "Gated reverb and dynamic effects",
  "Distortion" <http://www.spinsemi.com/knowledge_base/effects.html>
- Spin Semiconductor knowledge base, *Coding examples* (LFO setup, delay memory
  resolution) <http://www.spinsemi.com/knowledge_base/coding_examples.html>
- Spin Semiconductor reference programs `rom_rev1.spn`, `rev_pl_1.spn`
  (topology and coefficient ranges; read for design intent, not copied)
- Rainger FX, *Reverb-X user manual* (control behaviour and stated decay range)
