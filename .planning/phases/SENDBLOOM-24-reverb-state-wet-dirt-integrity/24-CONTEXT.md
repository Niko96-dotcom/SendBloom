# Phase 24: Reverb State & Wet-Dirt Integrity - Context

**Gathered:** 2026-07-12
**Status:** Ready for planning
**Mode:** Auto-accepted smart-discuss recommendations (full autonomous run)

<domain>
## Phase Boundary

Predelay, modulation, ProperSRC underfill, and wet-dirt filtering are correct without retuning the reverb character. Covers DSP-01…15. ADRs: ADR-V1-12, ADR-V1-13, ADR-V1-14, ADR-V1-15.

Does NOT change ProperSRC quality preset, tank voicing/RT60 character, `dirt_os` enablement, or shipping-policy/branding (Phase 25). Does NOT regress ProperSRC/HF greens or Phase 20–23 contract greens: `[pressure-release]`, `[oversized-block]`, `[true-bypass]`, `[midi-apvts]`, `[input-anchors]`, `[posthard]`, `[per-sample]`, `[realtime]`, `[authentic]`.

</domain>

<decisions>
## Implementation Decisions (LOCKED — auto-accepted)

### D-01 — Continuous fixed 55 ms dark tap (ADR-V1-12 / DSP-01…04)
- Always clock the predelay line every sample in bright and dark modes
- Fixed delay length = `kDarkPredelaySeconds` (0.055 s) × processing rate — set once in prepare / rate change, not `darkMix * 55 ms`
- Every sample: `push(input); delayed = pop(); tankInput = lerp(input, delayed, darkMix)`
- Do not stop clocking when bright (`darkMix ≈ 0`); bright must still advance the line so re-enabling Dark emits no stale frozen burst
- Bright/dark automation stays finite and click-bounded (no unbounded adjacent deltas)
- Apply to production tank path (`SchroederTankCore`); update `Fdn8Reverb` / legacy only if they share the defective variable-delay pattern and are still in live audio — prefer production path first
- Host-rate and fixed-rate dark predelay agree in wall-clock time

### D-02 — Modulation time invariant (ADR-V1-13 / DSP-05)
- Replace fixed sample-depth `kTankLfoDepthSamples = 16` usage with time-depth contract:
  - `kTankLfoDepthSeconds = 16.0 / 32768.0` (0.00048828125 s)
  - `depthSamples = kTankLfoDepthSeconds * processingRate`
- At internal 32,768 Hz this still equals 16 samples (character preserved)
- Add `tankLfoDepthSamplesForRate(double rate)` helper; assert `depthSamples/rate == kTankLfoDepthSeconds` at 32768, 44100, 48000, 88200, 96000
- Do **not** change `kTankLfoHz`

### D-03 — ProperSRC underfill pre-clear (ADR-V1-14 / DSP-06…08)
- Before ProperSRC downsampling: `std::fill(out, out + n, 0.0f);` then `written = converters.downsample(...)`
- Debug-check `0 <= written <= n`; unwritten samples remain deterministic zero
- Do **not** retune ProperSRC quality / imaging preset
- Existing ProperSRC imaging/HF gates must stay green (BASE-04 / DSP-08)

### D-04 — Wet dirt HP + DC blocker (ADR-V1-15 / DSP-09…15)
- Chain on dirty branch only: pre-clip HP 100 Hz (`kPreClipHpHz`) → pre-clip LP 6.5 kHz → clipper → post-clip LP 7.5 kHz → post-clip DC blocker/HP 20 Hz
- Allocation-free one-pole high-pass / DC blocker (`OnePoleHighpass`); wire into `WetOverdriveState` (constants already declared for 100 Hz)
- Distn blend against original wet as today; Distn=0 returns original wet within tolerance
- Dry path unaffected by all wet filtering
- `dirt_os` stays disabled and unimplemented (UI remains Coming soon / disabled)
- `authentic_color` remains off by default and in all factory presets (DSP-14/15) — no preset XML edits unless a preset is wrongly on
- Do not enable oversampling in v1

### D-05 — Contract / regression posture
- Add or flip Phase 24 DSP fidelity contracts / unit proofs for predelay continuous clock, stale-burst absence, mod time-invariance, SRC sentinel clear, dirt HP/DC
- Preserve green: ProperSRC/HF diagnostic gates; Phase 20–23 `[v1][contract]` tags listed in domain
- Leave red: `[shipping-policy]` (Phase 25)
- Do not retune reverb character (comb delays, damping maps, tank gain, LFO rate)

### Claude's Discretion
- Exact one-pole HP / DC-blocker coefficient form as long as 100 Hz / 20 Hz contracts and long-run DC `<1e-4` hold
- Whether V1 contracts live as new `V1Contract*Test.cpp` files vs extending tank/wet/adapter suites — prefer clear `[v1][contract]` tags matching Phase 19 harness style
- Scope of `Fdn8Reverb` / `LegacyAccumulatorPath` predelay fixes if unused on production path — fix production first; legacy only if still reachable in diagnostics A/B

</decisions>

<code_context>
## Existing Code Insights

### Exact defect loci
- `SchroederTankCore::updateCoeffs` — `predelaySamples = mix * kDarkPredelaySeconds * rate` (variable delay); `processTank` only clocks when `predelaySamples > 0.5f` → stale burst / non-continuous clock
- `SchroederTank32DelayTable::kTankLfoDepthSamples = 16.0f` used directly → depth in seconds changes with host/processing rate
- `FixedRateAdapter` ProperSRC path calls `converters.downsample` without pre-clearing `out` (Off path clears; ProperSRC does not)
- `WetOverdrive::kPreClipHpHz = 100` declared but unused; `WetOverdriveState` only prepares/runs pre/post LPs — missing 100 Hz HP and 20 Hz DC blocker
- Same variable-delay pattern in `Fdn8Reverb.h` and `LegacyAccumulatorPath.h`

### Reusable assets
- Constants: `kDarkPredelaySeconds`, `kPreClipHpHz`, `kPreClipLpHz`, `kPostClipLpHz`, `OnePoleLowpass`
- Suites to extend/preserve: `SchroederTank32Test`, `SchroederTankCoreTest`, `FixedRateAdapterTest`, `WetOverdrive*`, `HighFrequencyRingingDiagnosticsTest`, `AuthenticPathDiagnosticsTest`, ReleaseTruth safe defaults / presets
- Milestone §13.3–13.8 implementation sketches and acceptance gates

### Integration Points
- `source/SchroederTankCore.h`, `SchroederTank32DelayTable.h`
- `source/FixedRateAdapter.h` / `RateConverterPair`
- `source/WetOverdrive.h` (`WetOverdriveState`)
- Possibly `Fdn8Reverb.h`, `LegacyAccumulatorPath.h` if still exercised
- New/extended tests under `tests/`

</code_context>

<specifics>
## Specific Ideas

Autonomous run — accept recommended defaults; pause only on blockers.
Preserve ProperSRC/HF greens and Phase 20–23 contract greens.
Leave `[shipping-policy]` red for Phase 25.
No reverb character retune — time-invariant mod and continuous 55 ms tap only.

</specifics>

<deferred>
## Deferred Ideas

- Shipping brand strings / faceplate / UX copy / `[shipping-policy]` → Phase 25
- Reference capture / fidelity status (ADR-V1-17) → Phase 26
- ProperSRC quality preset changes (evidence-only later)
- `dirt_os` oversampling enablement (explicitly out of v1)

</deferred>
