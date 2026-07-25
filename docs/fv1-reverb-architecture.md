# Why the reverb is an allpass ring

**Status:** implemented, v1.x
**Supersedes the tank design in:** ADR-002 (8-line FDN / Schroeder tank)

## The problem

SendBloom's reverb was a Freeverb-style tank: four series allpasses into four
parallel damped combs into one modulated allpass, at 32,768 Hz. Its longest comb
was 1,202 samples — **36.7 ms**. Everything else about the plugin (parallel dry,
wet-only overdrive, dual gate placement, pressure send) matched the effect it was
aiming at, but the reverb itself did not, and no amount of tuning damping or
decay time was going to close that gap, because the gap was structural.

## What the hardware class actually constrains

The reference hardware is built on the Spin Semiconductor FV-1. Three numbers
from its datasheet (SPN1001, 12 March 2010) fix the entire design space:

| | |
|---|---|
| Sample rate | 32,768 Hz (standard watch crystal on X1/X2) |
| Delay RAM | 32,768 words — *"Total internal memory delay: 1.0 seconds"* |
| Converters | −3 dB at 14.5–15.5 kHz |
| Coefficients | S1.14 (16-bit) |
| Accumulator | S.23 (24-bit) |
| Instructions | 128 per sample |

One second of delay memory *for the whole program* is the binding constraint. A
5–6 second decay is therefore not made from long delays — it is made from a
modest amount of memory with high recirculation gain. That forces a specific
topology, and Spin documents it.

## The topology Spin documents

From Spin's knowledge base ("Effects → Reverberation", Keith Barr):

> When multiple delays and all pass filters are placed into a loop, sound
> injected into the loop will recirculate, and the density of any impulse will
> increase as the signal passes successively through the allpass filters.

The described loop is **four blocks of "2 allpass filters and a delay"** in a
ring, with a reverb-time coefficient applied once per block, plus:

- input diffusion — *"add a few series allpass filters in the input signal
  path, so that the signal inserted into the loop has a higher initial density"*
- in-loop shelving — *"Shelving high pass and low pass filters may be added to
  the loop to control the decay of high and low frequencies"*
- modulation — *"slowly modulate some of the delay lengths within the reverb
  loop... using say, the SIN output in one place and COS in another"*
- output as taps from inside the ring delays, summed in different proportions

And, decisively, from "Considerations when building a reverb":

> **The total delay (excluding allpass filter delays) in the loop should be at
> least 200ms.** Shorter delay time will lead to *flutter*, a repeating quality
> in the tail. The human ear is very sensitive to flutter in the 4 to 8 Hz
> range; **very short delays will cause a tinny sound**, moderately short delays
> will cause a noticeable flutter.

The old tank's loop was 36.7 ms — about a fifth of the stated floor. Measured on
the built binary, its tail rippled at **28.2 Hz at every decay setting**, locked
to the comb length, which is exactly the failure mode described.

Barr also pins the allpass coefficient: 0.5 makes the reverb "build", 0.6 builds
quicker but is "fat" during the initial sound, and **0.7 and above "sound more
immediate, but can have the tendency to produce a 'ringing' sound."** The old
tank used 0.7.

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
   │   allpass(g=0.6) → delay → LF-loss shelf → HF-loss shelf   │
   │                                          → × krt           │
   │                                                            │
   │   allpasses 2311, 2909, 3167, 2417                         │
   │   delays    3623, 4597, 4391, 3671                         │
   └────────────────────────────────────────────────────────────┘
                                  │
        output = 0.6·del0[+1201] + 0.8·del1[+211]
               + 0.7·del2[+897]  + 0.5·del3[+1780]
```

All twelve lengths are prime, so ring modes coincide as little as possible.

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
mapping. This has a consequence worth stating plainly: **the ring cannot produce
a short reverb.** At the lowest usable loop gain it still decays over roughly a
second. That is a property of the hardware class, not a limitation to engineer
around — Barr: *"Do not expect a given reverb structure, with delays and
coefficients well chosen, to sound good at extremes of reverb time."*

`ParameterCurves::sizeToRT60` therefore spans **1.2 s to 6.0 s**, exponentially,
matching the reference manual's *"up to a maximum of 5 or 6 seconds"*. The old
curve started at 0.25 s, which the architecture cannot reach and the hardware
never did.

### Damping

Each block carries two first-order shelves inside the loop:

- **LF loss**, corner 105 Hz. The old tank had none, so long settings piled up
  low end. This is what keeps a 6 s tail from turning to mud.
- **HF loss**, corner 11.5 kHz (Bright) down to 2.1 kHz (Dark), depth 0.45 → 0.88.

Dark additionally crossfades in a 55 ms predelay: *"bright and immediate, or dark
with pre-delay"*.

### Modulation

Two sine LFOs at 0.48 Hz and 0.60 Hz, sin and cos each, sweeping the four ring
allpasses by ±4.6 / ±4.1 samples. Roughly a third of the old tank's depth and
spread over four points instead of one — enough to break up ringing without
audible pitch wobble.

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

## The overdrive

Same source, same reasoning. Spin's notes name the nonlinearity for this
hardware class and say why:

> If X<1 then Y=X / If X>1 then Y=2 − 1/X ... The sound however, especially for
> a guitar and keyboard instruments, is very nice, and aliasing is largely
> avoided. This concept delivers *nice* distortion, that is, lower level signals
> are clean, and only 'break up' on transients and emphasized instrumental notes.

Measured on the built binary, the curve is flat at gain 1.10 up to |x| = 0.30,
then bends: 0.93 at 0.5, 0.68 at 0.8, 0.31 at 2.0. The tanh curve it replaces
had no straight region at all — its gain fell continuously from 1.95× at silence
to 0.92× at full scale, squashing the whole reverb tail uniformly rather than
leaving quiet trails clean and biting only on the blooms.

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
