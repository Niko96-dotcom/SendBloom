# ADR-002: Reverb Engine Strategy

**Status:** PROPOSED  
**Date:** 2026-07-06

## Context

Hardware Reverb-X uses Spin FV-1 + custom EEPROM (not available). We need algorithmic reverb supporting:
- Bright (immediate) / Dark (55 ms predelay, darker damping)
- Size → RT60 up to 6 s
- Mono guitar pedal simplicity

## Decision

**Implement custom `ReverbEngine` class** using an **8-line FDN** design.

**Primary MIT reference:** [GhostNoteAudio/CloudSeedCore](https://github.com/GhostNoteAudio/CloudSeedCore) — modulated parallel lines, explicit T60 mapping in `UpdateLines()`.

**Secondary references:** chowdsp `SimpleReverb` / Jatin `Feedback-Delay-Networks` `shared/FDN.h` for `calcGainForT60` math (study/port, mind GPL on chowdsp module).

Do **not** fork RSAlgorithmicVerb (GPL-3, too broad).

## Rationale

| Option | Pros | Cons |
|--------|------|------|
| Fork RSAlgorithmicVerb | Many algorithms | GPL; scope creep |
| Submodule chowdsp_reverb | Battle-tested FDN | License mix in monorepo |
| Greenfield FDN | Full control, clean license | More work |
| Convolution IR | Accurate room | Wrong pedal; no FV-1 IR |

## Technical spec

```
Input → [optional predelay 0 or 55ms] → diffusion allpasses → 8x FDN → LPF damping → out
```

**Engine strategy (two-track):**
- **MVP:** 8-line FDN at host rate (48 kHz) — density + RT60 control
- **Character match (Phase 8):** Optional Schroeder/FV-1-style compact tank or 32 kHz coloration layer — see user dossier FV-1 topology (4 APF → 4 comb → modulated tank). Faust `dattorro_rev` spike via `reverb_trickery` before committing.

Do **not** read Reverb-X EEPROM or decompile FV-1 programs (legal; see `RESEARCH_ADDENDUM_USER_DOSSIER.md`).

- `size` → `RT60 = 0.25 + 5.75 * norm^2.4` seconds
- `dark` → enable predelay + lower damping cutoff (~3.2 kHz vs ~8 kHz)
- Modulation: slow LFO on delay lines (0.4–0.8 Hz, ±0.5 ms)

## References

- `chowdsp_utils/examples/SimpleReverb/SimpleReverbPlugin.h`
- `reillypascal-RSAlgorithmicVerb/Source/FDNs.h` (study only, GPL)
- Spin FV-1 datasheet (hardware context only)

## Acceptance

MB-032 RT60 test passes ±15% before chaining OD/gate.
