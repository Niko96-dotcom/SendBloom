# Pitfalls Research

**Domain:** Adding bandlimited hostRate ↔ 32,768 Hz SRC to an existing realtime JUCE Schroeder reverb plugin (SendBloom v2.0)
**Researched:** 2026-07-08
**Confidence:** HIGH (codebase-verified integration risks) / MEDIUM (external SRC/PDC guidance — r8brain and JUCE forum sources are authoritative but not cross-verified against SendBloom builds)

## Critical Pitfalls

### Pitfall 1: Treating Accumulator/Hold as Sample-Rate Conversion

**What goes wrong:**
The v1 `processAuthentic()` path steps the tank at `32768/hostRate` using an input accumulator, holds the last internal output, and linearly interpolates back to host rate. This is **not** bandlimited SRC. At 48 kHz host rate it produces a stable narrowband whistle in the 14–15 kHz imaging band — exactly what `HighFrequencyRingingDiagnosticsTest.cpp` catches (`14825 Hz tail RMS`, narrowband dominance ratio). Downstream `antiImageFilter` at 12 kHz is cleanup, not a fix.

**Why it happens:**
Phase 5 research explicitly documented "fractional hold" as Pattern 3 for the authentic path. It is cheap, sample-accurate for stepping, and passes finite-output tests — but it skips the anti-imaging filter required when inserting non-integer samples between host-rate ticks. Teams often mistake "runs at 32 kHz internally" for "properly resampled."

**How to avoid:**
- Extract a **true fixed-rate** `SchroederTankCore` that only ever sees 32,768 Hz samples.
- Wrap it in `FixedRateAdapter` with bandlimited upsample → core → bandlimited downsample (r8brain-first per PROJECT.md).
- Keep legacy accumulator path only under `Authentic32Mode::LegacyAccumulator` for A/B diagnostics — never ship it as the production bridge.
- Acceptance gate: pass existing HF regression tests (`kImagingBandPeakRmsMax`, `kNarrowbandDominanceMaxRatio`, `kAuthenticVsHostRmsAbove10kMaxRatio`).

**Warning signs:**
- Goertzel peak locked near `hostRate - kInternalRate/2` foldback (≈14.8 kHz at 48 kHz).
- Authentic bright tail has higher narrowband dominance than host-rate path.
- Adding steeper anti-image LPF "fixes" whistle but dulls brightness vs host-rate reference.

**Phase to address:**
Phase 2 — FixedRateAdapter + r8brain SRC prototype (before any user-facing enablement)

---

### Pitfall 2: Breaking the v1 Zero-Latency Contract Without a PDC Policy

**What goes wrong:**
v1 shipped with `setLatencySamples(0)` and `LatencyTest.cpp` asserts zero before and after `prepareToPlay`. High-quality arbitrary-ratio SRC (r8brain, polyphase FIR) introduces **non-zero group delay**. If ProperSRC is wired in but latency is still reported as 0, DAW PDC misaligns wet reverb against dry guitar, gate chop, and parallel mix — the "edited sample" feel drifts. If latency is reported correctly, v1's CHN-04 contract breaks and existing tests fail until policy is decided.

**Why it happens:**
SendBloom deliberately deferred latency policy ("measure first, decide ADR-003"). SRC libraries often auto-consume initial latency internally, making `getLatency()` return 0 even when overall input→output delay exists — easy to misread as "free." Teams also assume PDC fixes monitoring latency (it does not for live input).

**How to avoid:**
- Measure end-to-end wet-path delay at 44.1, 48, 88.2, 96 kHz with impulse or cross-correlation after ProperSRC integration.
- Call `setLatencySamples()` in `prepareToPlay()` (and on sample-rate change via `AsyncUpdater` if hosts ignore in-prepare updates).
- Record decision in **ADR-003**: (A) report SRC latency and update tests, (B) absorb latency internally to preserve zero-PDC at cost of buffer complexity, or (C) defer ProperSRC enablement until policy chosen.
- If zero-PDC is retained, budget internal delay lines equal to measured SRC latency — do not silently eat delay in the wet path while dry stays immediate.

**Warning signs:**
- `LatencyTest` still passes but offline null-test vs dry shows constant sample offset.
- Gate chop appears early/late relative to playing stop in DAW with PDC on.
- Latency changes when host rate changes but plugin does not re-report.

**Phase to address:**
Phase 6 — Latency/PDC measurement and ADR-003

---

### Pitfall 3: Heap Allocation or Resampler Construction on the Audio Thread

**What goes wrong:**
`CDSPResampler` allocates internal buffers in its constructor based on `MaxInLen`. Calling `new CDSPResampler(...)` or resizing buffers inside `processBlock()` violates CHN-05 and fails `RealtimeStressTest`. SendBloom already has a latent pattern: `PluginProcessor::processBlock()` calls `dryBuffer.setSize(...)` when host block exceeds prepared size — that is also a realtime alloc hazard SRC work must not amplify.

**Why it happens:**
r8brain docs show simple constructor usage; developers instantiate resamplers lazily on first authentic-color block or recreate on ratio change. Sample-by-sample wrappers tempt per-call temp vectors. `std::vector` growth in the pull loop is equally fatal.

**How to avoid:**
- Construct **both** upsampler and downsampler in `prepare()` only; store as member objects or `std::unique_ptr` created in `prepare()`, never in `processBlock()`.
- Pre-size all SRC scratch buffers using `getMaxOutLen(maxBlockSize)` at prepare time; add headroom for ratio worst case (48 kHz ↔ 32768 Hz).
- Fix `dryBuffer` overallocation in the same milestone — ensure `prepareToPlay(maxBlockSize)` covers any block the stress test throws (32–1024).
- Run existing `[realtime][stress]` suite with `authenticColor` toggling every 50 blocks.

**Warning signs:**
- CPU spikes or dropouts when toggling 32k Color mid-session.
- AddressSanitizer or custom allocator hits during `processBlock`.
- First block after prepare glitches (constructor on audio thread).

**Phase to address:**
Phase 2 — FixedRateAdapter prepare-time allocation; Phase 3 — block integration hardening

---

### Pitfall 4: Sample-by-Sample SRC Inside a Per-Sample Reverb Loop

**What goes wrong:**
Current architecture processes one host sample at a time through `GatedBloomChain::processSample()` → `IReverbEngine::processSample()`. Arbitrary-ratio async resamplers (r8brain) produce **variable output counts** per input chunk and maintain state between calls. Feeding one sample per `process()` call causes chronic buffer underrun/overrun, uneven 32 kHz tank stepping, and CPU blowup from per-sample filter overhead.

**Why it happens:**
v1 was designed sample-at-a-time for simplicity. SRC libraries document block "pull" loops. Teams try to wrap resampler `process(&singleSample, 1)` in the existing loop instead of refactoring to block processing.

**How to avoid:**
- Add `IReverbEngine::processBlock()` for the fixed-rate path; keep host-rate wrapper for backward compatibility.
- Batch host input into upsampler pull loop until 32 kHz block ready for `SchroederTankCore`.
- Downsample tank output back to host block in pull loop; consume r8brain leftover output samples before next block (library requirement).
- Leave per-sample path for host-rate mode only until fully migrated.

**Warning signs:**
- Tank LFO and modulated APF sound wrong only in ProperSRC mode.
- CPU usage scales with host rate much faster than block implementation.
- Output level drifts block-to-block (leftover sample accounting bug).

**Phase to address:**
Phase 3 — Block-level `IReverbEngine::processBlock()` integration

---

### Pitfall 5: Instant Engine Switch Without Crossfade (Mode-Switch Clicks)

**What goes wrong:**
Current `SchroederTank32::processSample()` switches immediately when `authenticColor != useAuthenticPath`: delay lengths reset, comb processing rate changes, accumulator clears. Toggling 32k Color mid-tail causes **discontinuities** — audible clicks, truncated reverb tail, phase jumps. `RealtimeStressTest` toggles `authenticColor` every 50 blocks but only checks `REQUIRE_NOTHROW`, not audibility.

**Why it happens:**
Boolean param maps directly to engine branch. v1 smoothed `authenticColorTarget` over 15 ms at the APVTS layer, but the engine still hard-switches on threshold crossing. Two engines (host-rate vs fixed-rate) have unrelated internal states.

**How to avoid:**
- Run **dual engines** during transition: host-rate tank + ProperSRC tank in parallel.
- Crossfade wet outputs over 20–50 ms equal-power ramp when `authenticColor` target crosses (reuse `BypassCrossfade` / `SmoothedValue` pattern from bypass).
- Do **not** reset delay lines or clear tank state on toggle — let old engine decay naturally while new engine fades in.
- Add click metric test: max sample delta on toggle block < threshold; optional sine burst spectral pop check.

**Warning signs:**
- Zipper or pop when flipping 32k Color in advanced drawer during held chord.
- Reverb tail "restarts" on toggle instead of continuing.
- HF test passes steady-state but fails on toggle-adjacent blocks.

**Phase to address:**
Phase 5 — Safe engine crossfade on 32k toggle

---

### Pitfall 6: Buffer Sizing Mismatch (MaxInLen, Block Size, Leftover Samples)

**What goes wrong:**
r8brain requires `MaxInLen` at construction; input chunks must not exceed it. Output size varies with ratio — at 48 kHz → 32 kHz upsample path, internal 32 kHz buffer may need **more samples than host block** for a given host block size. Undersized scratch buffers corrupt memory or truncate reverb tails. Failing to carry leftover resampler output between blocks causes dropouts or repeated samples.

**Why it happens:**
Host block sizes vary (32–1024 in stress test; DAWs can exceed `prepareToPlay` hint). Developers size buffers for host block only, not `getMaxOutLen()` or internal-rate equivalents. The 32768/48000 ratio is non-integer — no neat block alignment.

**How to avoid:**
- At prepare: `maxHostBlock`, compute `maxInternalBlock = ceil(maxHostBlock * 32768 / hostRate) + margin`.
- Size upsampler with `MaxInLen = maxHostBlock`; preallocate internal-rate tank buffer to `maxInternalBlock`.
- Maintain **leftover FIFOs** for both resampler directions; document invariants in `FixedRateAdapter`.
- Test full block-size matrix from `RealtimeStressTest` (`kBlockSizes`) with ProperSRC enabled.

**Warning signs:**
- Tail truncates on large blocks (1024) but not small (64).
- Different behavior at 44.1 vs 48 kHz with same block size.
- Periodic "tick" every N blocks (leftover miscount).

**Phase to address:**
Phase 2 — FixedRateAdapter buffer contract; Phase 4 — diagnostics at multiple block sizes

---

### Pitfall 7: Mixing Host-Rate and Fixed-Rate Semantics Inside One Tank Class

**What goes wrong:**
`SchroederTank32` branches on `useAuthenticPath` for delay scaling, comb `setProcessingSampleRate`, predelay rate, LFO increment, damping quantization, and anti-image filter. Extracting SRC without first splitting core invites **subtle double-scaling** — e.g., fixed delays fed through SRC *and* host-rate scaling, or RT60 feedback calibrated at 32 kHz but delay lines still at host sample period.

**Why it happens:**
Expedient to bolt resamplers onto existing class. `scaleDelay(..., authentic)` and `syncCombProcessingRate()` are deeply entangled with mode flag.

**How to avoid:**
- **Phase 1 extraction:** `SchroederTankCore` — single rate 32768 Hz, no `hostRate`/`useAuthenticPath` branches.
- `FixedRateAdapter` owns SRC + core; `HostRateReverbWrapper` scales delays for host path only.
- Delete authentic branches from core after extraction; one delay table, one LFO rate, one damping table.

**Warning signs:**
- RT60 tests pass host-rate but fail ProperSRC at same size knob.
- Dark predelay wrong duration in ProperSRC (55 ms becomes ~55 ms × ratio error).
- Comb feedback sounds metallic only in one mode.

**Phase to address:**
Phase 1 — SchroederTankCore extraction

---

### Pitfall 8: Gate/Send Timing Desync Relative to SRC Delay

**What goes wrong:**
Post gate is keyed from **input envelope**, not wet tail — by design for "edited sample" chop. If ProperSRC adds unreported wet-path delay, the player stops but wet reverb (including distorted tail) continues while gate already closed — or gate opens before bloom arrives. Pressure send scaling applies before reverb; SRC delay shifts wet bloom later relative to dry in parallel mix.

**Why it happens:**
Gate and send sit outside reverb in `GatedBloomChain`. SRC latency inserted only around tank breaks time alignment between envelope detector (host-rate, immediate) and wet return (delayed).

**How to avoid:**
- If reporting latency via PDC: wet and dry stay aligned in recorded mix; verify gate still musically correct (gate is input-keyed, so mainly affects perceived bloom lag in parallel mix).
- If zero-PDC internal compensation: delay envelope tap or advance gate sidechain consistently — **prefer PDC policy over hand-tuned gate delay hacks.**
- Integration test: pluck → stop → measure time to post-gate closure vs wet RMS decay; compare host-rate vs ProperSRC within ±1 block after PDC.

**Warning signs:**
- "Bloom then cut" feels late only with 32k Color on.
- Send pressure bloom arrives after dry pick attack mismatch.

**Phase to address:**
Phase 6 — PDC policy; Phase 7 — enablement verification in chain context

---

## Technical Debt Patterns

| Shortcut | Immediate Benefit | Long-term Cost | When Acceptable |
|----------|-------------------|----------------|-----------------|
| Keep legacy accumulator as default 32k path | Ships RC1 without SRC work | Permanent HF whistle; user distrust of "32k Color" | **Never** for user-facing enablement — RC1 only with `authentic_color=0` |
| Downstream anti-image LPF only | Quiets worst whistle | Masks SRC bug; wrong timbre vs host-rate | Never as sole fix |
| Report zero latency while SRC adds delay | Passes v1 LatencyTest | DAW misalignment; gate/mix phase errors | Never after ProperSRC wired to output |
| Per-sample SRC shim | Avoids processBlock refactor | CPU + correctness failure | Never |
| Instant mode switch (no crossfade) | Simpler code | Clicks on toggle; fails UX acceptance | Never for user-facing toggle |
| Dynamic resampler quality at runtime | CPU savings on low settings | Latency changes; host ignores mid-play update | Only if latency fixed at max + internal compensation |
| Single monolithic SchroederTank32 | Fewer files | Untestable SRC boundary; branch entropy | Never — extract core + adapter |

## Integration Gotchas

| Integration | Common Mistake | Correct Approach |
|-------------|----------------|------------------|
| r8brain `CDSPResampler` | Construct in `processBlock`; ignore leftover output | Construct in `prepare()`; pull loop; FIFO leftovers; query `getMaxOutLen()` |
| JUCE `setLatencySamples` | Set once at plugin ctor only; never update on SR change | Set in `prepareToPlay()` from measured SRC delay; `AsyncUpdater` on change if needed |
| JUCE `AudioProcessor::processBlock` | Assume `numSamples <=` prepared block always | Preallocate for max; avoid `dryBuffer.setSize` on audio thread |
| `IReverbEngine` interface | Only `processSample()` | Add `processBlock()` for SRC path; default impl delegates for host-rate |
| `SmoothedParameterBank` | Boolean threshold triggers immediate engine reset | Smoothed target drives crossfade mixer, not hard switch |
| Catch2 HF diagnostics | Test only legacy path | Three-path harness: HostRate / LegacyAccumulator / ProperSRC |
| pluginval / realtime tests | Enable ProperSRC before alloc audit | Run stress + pluginval after Phase 3 integration |

## Performance Traps

| Trap | Symptoms | Prevention | When It Breaks |
|------|----------|------------|----------------|
| Per-sample r8brain calls | CPU >15% one instance | Block pull processing | Always — immediate |
| Oversized r8brain quality preset | CPU spikes at 96 kHz | Start r8brain default; profile; tune only if HF tests fail | Low-latency sessions / many instances |
| Dual-engine crossfade always on | 2× reverb CPU when idle | Only run dual during 20–50 ms fade window | After Phase 5 if not gated |
| Internal 32 kHz block >> host block | L1 cache misses | Chunk internal processing to reasonable frame (e.g., 64–256 @ 32 kHz) | Large host buffers (1024+) |
| Re-preparing filters on mode switch | `dampingFilter.prepare()` in hot path | Prepare both rate configs in `prepare()`; switch coeffs only | Every authentic toggle (current `setProcessingSampleRate`) |

## Security Mistakes

| Mistake | Risk | Prevention |
|---------|------|------------|
| GPL SRC library (libsamplerate etc.) | License contamination in portfolio/commercial path | r8brain MIT per PROJECT.md; verify headers in CI |
| Copying FV-1/EEPROM delay tables | Legal boundary violation | Keep synthesized coprime table; document clean-room in ADR-003 |

*(SendBloom v2 SRC work is DSP/integration risk dominated — no network attack surface.)*

## UX Pitfalls

| Pitfall | User Impact | Better Approach |
|---------|-------------|-----------------|
| Enabling 32k Color before ProperSRC passes gates | "Glass whistle" on bright presets — users blame entire plugin | RC1: `authentic_color=0` all presets; enable only after HF acceptance |
| Audible click on 32k toggle | Breaks trust in advanced drawer | 20–50 ms crossfade; optional UI hint during morph |
| Latency jump when enabling 32k | Monitoring feels "off" in low-latency DAW buffer | ADR-003 documents behavior; consider keeping 32k off in low-latency mode later |
| 32k Color sounds dull after LPF band-aid | "Fixed ringing but lost air" | Proper SRC + compare `rmsAbove10k` to host-rate cap (1.4×) |

## "Looks Done But Isn't" Checklist

- [ ] **ProperSRC path:** Often missing bandlimited **upsample** — verify impulse response shows no energy at imaging foldback freqs, not just lower 14 kHz peak.
- [ ] **Latency:** Often missing `setLatencySamples()` update — verify impulse peak offset matches reported latency in Logic/Reaper insert tooltip.
- [ ] **Realtime safety:** Often passes finite checks but allocates — verify with stress test + Instruments/ASan build; no `setSize`/`new` in `processBlock`.
- [ ] **Mode toggle:** Often passes steady-state HF tests — verify toggle during sustained tail (click + spectrum pop).
- [ ] **Block sizes:** Often tested at 512 only — verify 32, 64, 1024 blocks at 44.1 and 48 kHz.
- [ ] **Host-rate regression:** Often broken while fixing authentic — verify configs A vs B in HF matrix still pass after refactor.
- [ ] **Gate/send:** Often verified only host-rate — verify pluck-stop chop timing with ProperSRC + PDC on.
- [ ] **RC1 safety:** Often regressed by preset edit — verify all factory presets `authentic_color=0` until explicit enablement phase.

## Recovery Strategies

| Pitfall | Recovery Cost | Recovery Steps |
|---------|---------------|----------------|
| Shipped accumulator as "32k Color" | LOW (RC1 mitigated) | Keep off by default; branch `feature/proper-32k-src`; do not patch accumulator further |
| HF imaging persists after SRC | MEDIUM | Capture three-path CSV from diagnostics test; swap upsample/downsample order; verify ratio direction; increase r8brain quality one step |
| Latency misreport | MEDIUM | Measure impulse offset; update ADR-003; fix `prepareToPlay` + tests; re-run DAW null test |
| Realtime alloc regression | HIGH if in release | Move all resizer to prepare; add CI guard with alloc-tracking build |
| Mode-switch clicks | LOW–MEDIUM | Add dual-engine crossfade; no state reset on toggle |
| Entangled tank/core | HIGH | Stop feature work; complete SchroederTankCore extraction before more SRC glue |

## Pitfall-to-Phase Mapping

| Pitfall | Prevention Phase | Verification |
|---------|------------------|--------------|
| Accumulator mistaken for SRC | Phase 2 — FixedRateAdapter + r8brain | HF regression tests; imaging band RMS; auth vs host 10k ratio |
| Zero-PDC contract break | Phase 6 — Latency/PDC + ADR-003 | Updated `LatencyTest`; impulse offset; DAW null test |
| Heap alloc in audio thread | Phase 2–3 — prepare-time sizing | `RealtimeStressTest` with authentic toggling; no `setSize` in processBlock |
| Per-sample SRC | Phase 3 — `processBlock()` path | CPU profile; block pull correctness |
| Mode-switch clicks | Phase 5 — engine crossfade | Toggle during tail; max delta test; audible DAW check |
| Buffer underrun/overrun | Phase 2 — buffer contract | Block size matrix 32–1024; 44.1/48/96 kHz |
| Host/fixed rate entanglement | Phase 1 — SchroederTankCore extraction | Core unit tests without hostRate; RT60 at 32 kHz only |
| Gate/send desync | Phase 6–7 — PDC + chain verify | Pluck-stop gate timing test with PDC |
| RC1 preset regression | Phase 0/RC1 safety freeze | `ReleaseTruthTest` / preset scan `authentic_color=0` |
| Missing diagnostics | Phase 4 — three-path harness | CSV matrix Host/Legacy/Proper; legacy fails imaging, proper passes |

### Suggested v2 Phase Order (pitfall-driven)

1. **RC1 safety freeze** — prevents user exposure to accumulator HF bug.
2. **SchroederTankCore extraction** — prevents scaling entanglement (Pitfall 7).
3. **FixedRateAdapter + r8brain** — addresses Pitfalls 1, 3, 6 at boundary.
4. **Block-level integration** — addresses Pitfalls 4, 3.
5. **Three-path diagnostics** — proves ProperSRC vs legacy; gates enablement.
6. **Engine crossfade** — addresses Pitfall 5.
7. **Latency/PDC + ADR-003** — addresses Pitfalls 2, 8.
8. **Enablement** — turn 32k Color on only after checklist green.

## Sources

- SendBloom `source/SchroederTank32.h` — accumulator authentic path, instant mode switch, anti-image LPF (`processAuthentic`, lines 67–72, 200–215) [HIGH — codebase]
- SendBloom `source/SchroederTank32DelayTable.h` — `kInternalRate = 32768`, `kAuthenticAntiImageLpHz` [HIGH — codebase]
- SendBloom `.planning/PROJECT.md` — v1 diagnosis (14–15 kHz imaging), v2 Option C plan, r8brain-first, zero-PDC deferral [HIGH — project]
- SendBloom `tests/HighFrequencyRingingDiagnosticsTest.cpp` — acceptance metrics (`14825 Hz`, narrowband dominance, 10k RMS ratio) [HIGH — codebase]
- SendBloom `tests/LatencyTest.cpp` — zero latency contract [HIGH — codebase]
- SendBloom `tests/RealtimeStressTest.cpp` — block size matrix, authentic toggle stress [HIGH — codebase]
- SendBloom `source/PluginProcessor.cpp` — `dryBuffer.setSize` in processBlock (line 178), per-sample chain [HIGH — codebase]
- SendBloom `.planning/milestones/v1.0-phases/05-schroedertank32-reverb/05-RESEARCH.md` — Pattern 3 fractional hold documented as intentional [HIGH — project]
- r8brain-free-src documentation — async pull model, `getLatency()`, `MaxInLen`, leftover output consumption [MEDIUM — vendor docs, not build-verified]
- JUCE forum — `setLatencySamples` in `prepareToPlay`, sample-rate change VST3 quirks, AsyncUpdater for dynamic latency [MEDIUM — community]
- SOF / SSRC algorithm references — polyphase SRC requires anti-imaging + anti-aliasing filters; linear interpolation insufficient [MEDIUM — general DSP]

---
*Pitfalls research for: SendBloom v2.0 Proper 32k SRC integration*
*Researched: 2026-07-08*
