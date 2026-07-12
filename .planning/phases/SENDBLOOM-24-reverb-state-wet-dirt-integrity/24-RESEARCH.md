# Phase 24: Reverb State & Wet-Dirt Integrity - Research

**Researched:** 2026-07-12
**Domain:** Schroeder tank predelay/modulation, ProperSRC underfill, wet-path overdrive filtering (DSP-01…15; ADR-V1-12…15)
**Confidence:** HIGH

## Summary

Phase 24 closes four latent DSP correctness holes without retuning reverb character. Production audio flows **host-rate** through `HostRateReverbEngine` → `SchroederTankCore`, and **authentic_color** through `FixedRateAdapter` → same core at 32,768 Hz with ProperSRC round-trip. All four defects are localized: variable predelay length + conditional clocking in `SchroederTankCore` (mirrored in `LegacyAccumulatorPath` / `Fdn8Reverb`), fixed-sample LFO depth in `SchroederTankCore`, missing `std::fill` before ProperSRC downsample in `FixedRateAdapter`, and an incomplete wet-dirt chain in `WetOverdriveState` (100 Hz HP and 20 Hz DC blocker declared but unwired).

Locked CONTEXT decisions prescribe **fixed 55 ms tap always clocked with mix-only lerp**, **time-invariant mod depth** (`kTankLfoDepthSeconds = 16/32768`), **sentinel pre-clear before downsample**, and **HP100 → LP6.5k → clip → LP7.5k → DC/HP20** on the dirty branch only. No ProperSRC quality retune, no `dirt_os` enablement, no `authentic_color` default change.

**Primary recommendation:** Three bounded plans — (1) predelay topology in `SchroederTankCore` + legacy parity, (2) LFO time depth + SRC underfill, (3) wet-dirt filters + safety-default contracts — each with new `[v1][contract]` Catch2 proofs and regression runs on existing `[verb][FixedRateAdapter][SRC-06]` / HF diagnostics.

## Architectural Responsibility Map

| Capability | Primary Tier | Secondary Tier | Rationale |
|------------|-------------|----------------|-----------|
| Dark predelay clock + blend | API / Backend (DSP core) | — | Delay-line state lives in `SchroederTankCore`; host and ProperSRC paths share it |
| LFO modulation depth | API / Backend (DSP core) | — | Tank AP delay modulation is internal to core; must scale with `processingRate_` |
| ProperSRC underfill zero-fill | API / Backend (`FixedRateAdapter`) | — | Output buffer is host block; adapter owns downsample write contract |
| Wet dirt HP / DC / LPs | API / Backend (`WetOverdriveState`) | — | Applied after reverb, before gate; dry tap is upstream |
| `authentic_color` / `dirt_os` defaults | API / Backend (parameter layout) | Database / Storage (preset XML) | APVTS defaults + factory preset recall; no runtime DSP for `dirt_os` today |

<user_constraints>
## User Constraints (from CONTEXT.md)

**CRITICAL:** Locked decisions are NON-NEGOTIABLE for planning/execution.

### Locked Decisions

#### D-01 — Continuous fixed 55 ms dark tap (ADR-V1-12 / DSP-01…04)
- Always clock the predelay line every sample in bright and dark modes
- Fixed delay length = `kDarkPredelaySeconds` (0.055 s) × processing rate — set once in prepare / rate change, not `darkMix * 55 ms`
- Every sample: `push(input); delayed = pop(); tankInput = lerp(input, delayed, darkMix)`
- Do not stop clocking when bright (`darkMix ≈ 0`); bright must still advance the line so re-enabling Dark emits no stale frozen burst
- Bright/dark automation stays finite and click-bounded (no unbounded adjacent deltas)
- Apply to production tank path (`SchroederTankCore`); update `Fdn8Reverb` / legacy only if they share the defective variable-delay pattern and are still in live audio — prefer production path first
- Host-rate and fixed-rate dark predelay agree in wall-clock time

#### D-02 — Modulation time invariant (ADR-V1-13 / DSP-05)
- Replace fixed sample-depth `kTankLfoDepthSamples = 16` usage with time-depth contract:
  - `kTankLfoDepthSeconds = 16.0 / 32768.0` (0.00048828125 s)
  - `depthSamples = kTankLfoDepthSeconds * processingRate`
- At internal 32,768 Hz this still equals 16 samples (character preserved)
- Add `tankLfoDepthSamplesForRate(double rate)` helper; assert `depthSamples/rate == kTankLfoDepthSeconds` at 32768, 44100, 48000, 88200, 96000
- Do **not** change `kTankLfoHz`

#### D-03 — ProperSRC underfill pre-clear (ADR-V1-14 / DSP-06…08)
- Before ProperSRC downsampling: `std::fill(out, out + n, 0.0f);` then `written = converters.downsample(...)`
- Debug-check `0 <= written <= n`; unwritten samples remain deterministic zero
- Do **not** retune ProperSRC quality / imaging preset
- Existing ProperSRC imaging/HF gates must stay green (BASE-04 / DSP-08)

#### D-04 — Wet dirt HP + DC blocker (ADR-V1-15 / DSP-09…15)
- Chain on dirty branch only: pre-clip HP 100 Hz (`kPreClipHpHz`) → pre-clip LP 6.5 kHz → clipper → post-clip LP 7.5 kHz → post-clip DC blocker/HP 20 Hz
- Allocation-free one-pole high-pass / DC blocker (`OnePoleHighpass`); wire into `WetOverdriveState` (constants already declared for 100 Hz)
- Distn blend against original wet as today; Distn=0 returns original wet within tolerance
- Dry path unaffected by all wet filtering
- `dirt_os` stays disabled and unimplemented (UI remains Coming soon / disabled)
- `authentic_color` remains off by default and in all factory presets (DSP-14/15) — no preset XML edits unless a preset is wrongly on
- Do not enable oversampling in v1

#### D-05 — Contract / regression posture
- Add or flip Phase 24 DSP fidelity contracts / unit proofs for predelay continuous clock, stale-burst absence, mod time-invariance, SRC sentinel clear, dirt HP/DC
- Preserve green: ProperSRC/HF diagnostic gates; Phase 20–23 `[v1][contract]` tags listed in domain
- Leave red: `[shipping-policy]` (Phase 25)
- Do not retune reverb character (comb delays, damping maps, tank gain, LFO rate)

### Claude's Discretion
- Exact one-pole HP / DC-blocker coefficient form as long as 100 Hz / 20 Hz contracts and long-run DC `<1e-4` hold
- Whether V1 contracts live as new `V1Contract*Test.cpp` files vs extending tank/wet/adapter suites — prefer clear `[v1][contract]` tags matching Phase 19 harness style
- Scope of `Fdn8Reverb` / `LegacyAccumulatorPath` predelay fixes if unused on production path — fix production first; legacy only if still reachable in diagnostics A/B

### Deferred Ideas (OUT OF SCOPE)
- Shipping brand strings / faceplate / UX copy / `[shipping-policy]` → Phase 25
- Reference capture / fidelity status (ADR-V1-17) → Phase 26
- ProperSRC quality preset changes (evidence-only later)
- `dirt_os` oversampling enablement (explicitly out of v1)
</user_constraints>

<phase_requirements>
## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| DSP-01 | Predelay line clocked continuously in bright and dark | Remove `predelaySamples > 0.5f` gate; always `push`/`pop` in `processTank` [CITED: milestone §13.3] |
| DSP-02 | Dark mode uses fixed 55 ms tap blended by dark mix | `setDelay(kDarkPredelaySeconds * rate)` in `prepare`; lerp not variable delay [CITED: ADR-V1-12] |
| DSP-03 | Re-enabling Dark emits no stale frozen burst | Continuous clock + fixed tap; §13.4 toggle test after bright silence |
| DSP-04 | Bright/dark automation finite and click-bounded | Ramp `darkMix` during tone; assert bounded adjacent delta (§13.4) |
| DSP-05 | LFO depth invariant in seconds across rates | `tankLfoDepthSamplesForRate`; keep 16 samples @ 32768 [CITED: ADR-V1-13, §13.5] |
| DSP-06 | ProperSRC output pre-cleared | `std::fill(out, out+n, 0)` before `downsample` in ProperSRC branch [CITED: ADR-V1-14, §13.6] |
| DSP-07 | ProperSRC unwritten samples remain zero | `RateConverterPair::downsample` may return `written < n`; fill guarantees zeros [VERIFIED: source/RateConverterPair.h] |
| DSP-08 | Existing ProperSRC imaging/HF gates stay green | Preserve `FixedRateAdapterTest` SRC-06, `HighFrequencyRingingDiagnosticsTest`, `ReleaseTruthTest` safe HF |
| DSP-09 | Wet dirt 100 Hz pre-clip HP | Wire `kPreClipHpHz` via `OnePoleHighpass` before existing preClipLp [CITED: ADR-V1-15, §13.7] |
| DSP-10 | Wet dirt 20 Hz post-clip DC blocker | Second `OnePoleHighpass` @ 20 Hz after postClipLp [CITED: §13.7] |
| DSP-11 | Long-run DC offset below gate | Symmetric sine + DC input; mean abs output `< 1e-4` (§13.8) |
| DSP-12 | Dry path unaffected by wet filtering | `WetOverdriveState` only in wet chain; extend `DryPathIntegrityTest` / distn=0 identity |
| DSP-13 | `dirt_os` disabled and unimplemented | UI `setEnabled(false)`; snapshot field unused in `GatedBloomChain` [VERIFIED: codebase grep] |
| DSP-14 | `authentic_color` off by default | `ParameterLayout` default + `[release][safe]` fresh-load test |
| DSP-15 | All factory presets keep `authentic_color=0` | All preset XML `value="0"` + `[release][safe]` preset loop |
</phase_requirements>

## Current State (Exact Bug Loci)

### 1. Variable predelay + conditional clock — `SchroederTankCore.h`

**CURRENT (wrong):**
```cpp
// updateCoeffs — delay shrinks with bright mix
predelaySamples = mix * kDarkPredelaySeconds * static_cast<float>(processingRate_);
predelayLine.setDelay(predelaySamples);

// processTank — line frozen when bright
if (predelaySamples > 0.5f) {
    predelayLine.pushSample(0, input);
    x = predelayLine.popSample(0);
}
```

**REQUIRED (ADR-V1-12 / §13.3):**
```cpp
// prepare() — once per rate
const auto darkDelaySamples = kDarkPredelaySeconds * processingRate_;
predelayLine.setDelay(static_cast<float>(darkDelaySamples));

// processTank() — every sample
predelayLine.pushSample(0, input);
const auto delayed = predelayLine.popSample(0);
const auto x = input + darkMix * (delayed - input);  // equivalent lerp
```

Production path: `SchroederTank32` → `HostRateReverbEngine` / `FixedRateAdapter::core` both use `SchroederTankCore`. [VERIFIED: source tree]

### 2. Same predelay defect — `LegacyAccumulatorPath.h`

Mirrors variable delay (`mix * kDarkPredelaySeconds`) and conditional clock (`predelaySamples > 0.5f`). Still exercised by `FixedRateAdapterTest` LegacyAccumulator parity (`[SRC-05]`). Fix after core so A/B stays aligned. [VERIFIED: tests/FixedRateAdapterTest.cpp]

### 3. Same predelay defect — `Fdn8Reverb.h` (discretionary)

`predelaySec = mix * kDarkPredelaySeconds` + conditional clock. Not on production `SchroederTank32` path; only `Fdn8ReverbTest`. Planner may defer unless diagnostics still reach it.

### 4. Fixed-sample LFO depth — `SchroederTankCore.h` + `SchroederTank32DelayTable.h`

**CURRENT:**
```cpp
static constexpr float kTankLfoDepthSamples = 16.0f;
// ...
const auto mod = std::sin(lfoPhase) * SchroederTank32DelayTable::kTankLfoDepthSamples;
```

At 48 kHz host rate, 16 samples = 0.333 ms — not 16/32768 s (0.488 ms). Character drifts with rate. [VERIFIED: SchroederTankCore.h]

**REQUIRED:**
```cpp
static constexpr double kTankLfoDepthSeconds = 16.0 / 32768.0;
static float tankLfoDepthSamplesForRate(double rate) noexcept {
    return static_cast<float>(kTankLfoDepthSeconds * rate);
}
const auto mod = std::sin(lfoPhase) * tankLfoDepthSamplesForRate(processingRate_);
```

Keep `kTankLfoHz = 0.55f` unchanged. Legacy path at fixed 32768 may keep using helper with `kInternalRate`.

### 5. ProperSRC underfill — `FixedRateAdapter.h`

**CURRENT:** ProperSRC branch calls `converters.downsample(...)` with no output pre-clear. Off branch clears (`std::fill`). `RateConverterPair::downsample` returns early with `hostWritten < nHostWanted` without zeroing tail slots. [VERIFIED: FixedRateAdapter.h, RateConverterPair.h:83-119]

**REQUIRED:**
```cpp
std::fill(out, out + n, 0.0f);
const int written = converters.downsample(internalProcessBuf.data(), nInternal, out, n);
jassert(written >= 0 && written <= n);
```

Do not change `kProperSrcQuality` in `RateConverterPair.h`.

### 6. Incomplete wet-dirt chain — `WetOverdrive.h`

**CURRENT `WetOverdriveState::processFilteredBranch`:**
```cpp
const auto bandLimited = preClipLp.process(wet);  // 6.5 kHz only
const auto clipped = WetOverdrive::clipSample(bandLimited, kActiveCurve);
return postClipLp.process(clipped);               // 7.5 kHz only
```

`kPreClipHpHz = 100` declared, never used. No DC blocker. Asymmetric clip can leave DC on dirty branch. [VERIFIED: WetOverdrive.h]

**REQUIRED chain (dirty branch / `processFilteredBranch`):**
```cpp
auto x = preClipHp.process(wet);       // 100 Hz
x = preClipLp.process(x);              // 6.5 kHz
x = clipSample(x, kActiveCurve);
x = postClipLp.process(x);             // 7.5 kHz
x = postClipDcBlock.process(x);        // 20 Hz DC blocker
return x;
```

`process(wet, distnBlend)` blend unchanged: `wet + distnBlend * (driven - wet)`.

### 7. Safety defaults — already green (preserve)

- `ReleaseTruthTest`: `[release][safe]` fresh load + all presets `authentic_color=0` [VERIFIED: tests/ReleaseTruthTest.cpp]
- `dirt_os`: parameter default `false`, UI disabled, not read in `GatedBloomChain` [VERIFIED: source grep]

## Recommended Implementation Shape

### Plan 01 — Predelay continuous fixed tap (DSP-01…04)

1. **`SchroederTankCore.h`**
   - In `prepare()`: compute and `setDelay` fixed `kDarkPredelaySeconds * processingRate_`.
   - Remove per-block `predelaySamples` delay mutation from `updateCoeffs` (keep damping/RT60 there).
   - Store `darkMix` for `processTank` (member set in `updateCoeffs` or pass through).
   - `processTank`: unconditional push/pop + `input + mix * (delayed - input)`.
   - Drop `predelaySamples` member if unused.

2. **`LegacyAccumulatorPath.h`** (diagnostics parity)
   - Same fixed-delay + continuous-clock pattern at `kInternalRate`.
   - Re-run `[verb][FixedRateAdapter][LegacyAccumulator][SRC-05]` tests.

3. **`Fdn8Reverb.h`** — optional same fix if planner keeps FDN tests honest.

4. **Tests** — new `tests/V1ContractPredelayTest.cpp` (or extend `SchroederTankCoreTest`):
   - `[v1][contract][predelay][DSP-01]` continuous clock: bright mode still advances line (impulse timing).
   - `[v1][contract][predelay][DSP-02]` dark IR onset ≈ 55 ms ± tolerance vs bright immediate.
   - `[v1][contract][predelay][DSP-03]` bright → silence > 55 ms → dark: no pre-55 ms stale burst.
   - `[v1][contract][predelay][DSP-04]` darkMix automation on tone: finite, bounded `|Δsample|`.
   - Host @ 48 kHz vs ProperSRC @ 48 kHz dark onset agree in wall-clock (§13.4).

5. **Regression:** `SchroederTank32Test` dark predelay case, `LatencyTest` tail +0.055 s, RT60 tests unchanged.

### Plan 02 — Mod time-invariant + ProperSRC underfill (DSP-05…08)

1. **`SchroederTank32DelayTable.h`**
   - Add `kTankLfoDepthSeconds`, `tankLfoDepthSamplesForRate(double)`.
   - Deprecate direct `kTankLfoDepthSamples` use (may keep alias `= tankLfoDepthSamplesForRate(kInternalRate)` for docs).

2. **`SchroederTankCore.h`** (+ `LegacyAccumulatorPath.h` tank AP mod line)
   - Replace `* kTankLfoDepthSamples` with helper at `processingRate_` / `kInternalRate`.

3. **`FixedRateAdapter.h`**
   - ProperSRC branch: `std::fill(out, out+n, 0)` before downsample; capture `written`; `jassert` bounds.

4. **Tests**
   - `[v1][contract][mod-invariant][DSP-05]` depth/rate ratio at 32768, 44100, 48000, 88200, 96000.
   - `[v1][contract][src-underfill][DSP-06][DSP-07]` sentinel fill `0xBD` (or `1.0f`), first ProperSRC blocks: no sentinel; tail zeros where `written < n`.
   - **Regression (must stay green):** `[verb][FixedRateAdapter][SRC-06]`, `[verb][FixedRateAdapter][ProperSRC][multiRate]`, `HighFrequencyRingingDiagnosticsTest`, `ReleaseTruthTest` `[release][safe]` HF imaging.

### Plan 03 — Wet dirt HP/DC + safety defaults (DSP-09…15)

1. **`WetOverdrive.h`**
   - Add `OnePoleHighpass` (mirror `OnePoleLowpass`: `alpha = 1 - exp(-ω)`, `y = alpha*(x - x1) + alpha*y1` or milestone §13.7 form).
   - `WetOverdriveState`: `preClipHp` @ `kPreClipHpHz`, `postClipDcBlock` @ 20 Hz; wire chain in `processFilteredBranch`.
   - `prepare`/`reset` both filters.

2. **Tests** — new `tests/V1ContractWetDirtTest.cpp`:
   - `[v1][contract][wet-dirt][DSP-09]` 30 Hz strongly attenuated vs 1 kHz on filtered branch.
   - `[v1][contract][wet-dirt][DSP-10][DSP-11]` DC input + long symmetric sine: mean abs output `< 1e-4`.
   - `[v1][contract][wet-dirt][DSP-12]` distn=0 identity; `DryPathIntegrityTest` dry extract unchanged at distn max.
   - `[v1][contract][wet-dirt][DSP-09]` asymmetry: `|clip(+x)| > |clip_sym(+x)|` preserved.
   - `[v1][contract][wet-dirt][DSP-13]` source scan: `dirtOs` not referenced in chain processor; UI disabled.
   - **DSP-14/15:** keep `[release][safe]` tests green; grep presets if any `authentic_color=1`.

3. **Regression:** `WetOverdriveTest`, `WetOverdriveDiagnosticsTest` (220 Hz swell `< 1.15`, finite output).

## Validation Reality

| Tag / suite | Expect after phase |
|-------------|-------------------|
| `[v1][contract][predelay]` | GREEN (new) |
| `[v1][contract][mod-invariant]` | GREEN (new) |
| `[v1][contract][src-underfill]` | GREEN (new) |
| `[v1][contract][wet-dirt]` | GREEN (new) |
| `[verb][FixedRateAdapter][SRC-06]` | stay GREEN |
| `[release][safe]` (HF + authentic defaults) | stay GREEN |
| `[pressure-release]` `[oversized-block]` `[true-bypass]` `[midi-apvts]` `[input-anchors]` `[posthard]` `[per-sample]` `[realtime]` `[authentic]` | stay GREEN |
| `[v1][contract][shipping-policy]` | stay RED (Phase 25) |
| RT60 / tail character tests | stay GREEN (no comb/LFO Hz retune) |

## Validation Architecture

### Test Framework

| Property | Value |
|----------|-------|
| Framework | Catch2 3.8.1 (CPM, `cmake/Tests.cmake`) |
| Config file | Catch discovered via `catch_discover_tests(Tests)` |
| Quick run command | `ctest --test-dir Builds -C Release -R 'predelay|mod-invariant|src-underfill|wet-dirt' --output-on-failure` |
| Full suite command | `ctest --test-dir Builds -C Release --output-on-failure` or `scripts/verify-v1.sh` |

Direct binary filter (faster per-task):
```bash
./Builds/Tests "[v1][contract][predelay]"
./Builds/Tests "[v1][contract][mod-invariant]"
./Builds/Tests "[v1][contract][src-underfill]"
./Builds/Tests "[v1][contract][wet-dirt]"
```

### Phase Requirements → Test Map

| Req ID | Behavior | Test Type | Automated Command | File Exists? |
|--------|----------|-----------|-------------------|--------------|
| DSP-01 | Predelay clocks in bright | unit/contract | `./Builds/Tests "[v1][contract][predelay][DSP-01]"` | ❌ Wave 0 |
| DSP-02 | Fixed 55 ms tap + mix blend | unit/contract | `./Builds/Tests "[v1][contract][predelay][DSP-02]"` | ❌ Wave 0 |
| DSP-03 | No stale burst on dark re-enable | unit/contract | `./Builds/Tests "[v1][contract][predelay][DSP-03]"` | ❌ Wave 0 |
| DSP-04 | Bounded darkMix automation | unit/contract | `./Builds/Tests "[v1][contract][predelay][DSP-04]"` | ❌ Wave 0 |
| DSP-05 | LFO depth seconds invariant | unit/contract | `./Builds/Tests "[v1][contract][mod-invariant][DSP-05]"` | ❌ Wave 0 |
| DSP-06 | ProperSRC pre-clear | unit/contract | `./Builds/Tests "[v1][contract][src-underfill][DSP-06]"` | ❌ Wave 0 |
| DSP-07 | Unwritten output zero | unit/contract | `./Builds/Tests "[v1][contract][src-underfill][DSP-07]"` | ❌ Wave 0 |
| DSP-08 | HF/SRC imaging regression | integration | `./Builds/Tests "[verb][FixedRateAdapter][SRC-06]"` | ✅ |
| DSP-09 | 100 Hz pre-clip HP | unit/contract | `./Builds/Tests "[v1][contract][wet-dirt][DSP-09]"` | ❌ Wave 0 |
| DSP-10 | 20 Hz DC blocker | unit/contract | `./Builds/Tests "[v1][contract][wet-dirt][DSP-10]"` | ❌ Wave 0 |
| DSP-11 | Long-run DC `< 1e-4` | unit/contract | `./Builds/Tests "[v1][contract][wet-dirt][DSP-11]"` | ❌ Wave 0 |
| DSP-12 | Dry path unaffected | integration | `./Builds/Tests "[v1][contract][wet-dirt][DSP-12]"` + `DryPathIntegrityTest` | partial ✅ |
| DSP-13 | dirt_os disabled | contract/source | `./Builds/Tests "[v1][contract][wet-dirt][DSP-13]"` | ❌ Wave 0 |
| DSP-14 | authentic_color default off | release safe | `./Builds/Tests "[release][safe]" -c "fresh plugin load defaults authentic_color off"` | ✅ |
| DSP-15 | Presets authentic_color=0 | release safe | `./Builds/Tests "[release][safe]" -c "all factory presets recall authentic_color off"` | ✅ |

### Nyquist Tag → Command Map

| Nyquist tag | Catch2 filter | Command |
|-------------|---------------|---------|
| `[predelay]` | `[v1][contract][predelay]` | `ctest --test-dir Builds -C Release -R predelay` |
| `[mod-invariant]` | `[v1][contract][mod-invariant]` | `ctest --test-dir Builds -C Release -R mod-invariant` |
| `[src-underfill]` | `[v1][contract][src-underfill]` | `ctest --test-dir Builds -C Release -R src-underfill` |
| `[wet-dirt]` | `[v1][contract][wet-dirt]` | `ctest --test-dir Builds -C Release -R wet-dirt` |
| `[proper-src-hf]` (regression) | `[verb][FixedRateAdapter][SRC-06]` | `ctest --test-dir Builds -C Release -R SRC-06` |
| `[release-safe-dsp]` | `[release][safe]` | `ctest --test-dir Builds -C Release -R 'release.*safe'` |

### Sampling Rate

- **Per task commit:** plan-scoped tag filter (table above)
- **Per plan merge:** plan tag + SRC-06 + affected `SchroederTank*` / `WetOverdrive*` suites
- **Phase gate:** full `ctest` / `verify-v1.sh`; HF diagnostics + Phase 20–23 contract tags green

### Wave 0 Gaps

- [ ] `tests/V1ContractPredelayTest.cpp` — DSP-01…04
- [ ] `tests/V1ContractModInvariantTest.cpp` — DSP-05
- [ ] `tests/V1ContractSrcUnderfillTest.cpp` — DSP-06…07
- [ ] `tests/V1ContractWetDirtTest.cpp` — DSP-09…13
- [ ] `OnePoleHighpass` in `WetOverdrive.h` — implementation prerequisite for dirt contracts

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| One-pole HP / DC block | Biquad designer / RBJ cookbook | `OnePoleHighpass` same pattern as `OnePoleLowpass` | Allocation-free RT; milestone specifies one-pole |
| SRC underfill policy | Custom partial-buffer writer | `std::fill` + existing `RateConverterPair::downsample` | Downsampler already documents partial writes |
| Predelay blend | Variable delay time per mix | Fixed delay + linear blend | ADR-V1-12; avoids frozen-line stale bursts |

## Common Pitfalls

### Pitfall 1: Retuning delay line length with darkMix
**What goes wrong:** Restoring old `mix * 55ms` behavior while only fixing the clock gate.
**How to avoid:** Set delay only in `prepare`; mix affects lerp only.

### Pitfall 2: Breaking 32768 Hz character when fixing LFO
**What goes wrong:** Changing `kTankLfoHz` or depth seconds constant.
**How to avoid:** `16/32768` exactly; verify 16 samples at internal rate post-change.

### Pitfall 3: SRC pre-clear without regression run
**What goes wrong:** Missing imaging regression after adapter touch.
**How to avoid:** Mandatory `[SRC-06]` + HF matrix after Plan 02.

### Pitfall 4: Filtering dry path
**What goes wrong:** HP/DC applied before distn blend or on dry tap.
**How to avoid:** Only inside `processFilteredBranch`; distn=0 skips driven branch.

## Code Examples

### Fixed predelay + blend (milestone §13.3) [CITED: .planning/MILESTONE-SPEC-v1.0-interaction-truth.md §13.3]
```cpp
predelayLine.pushSample(0, input);
const auto delayed = predelayLine.popSample(0);
const auto x = input + darkMix * (delayed - input);
```

### SRC underfill (ADR-V1-14) [CITED: milestone ADR-V1-14]
```cpp
std::fill(out, out + n, 0.0f);
const int written = converters.downsample(internalProcessBuf.data(), nInternal, out, n);
jassert(written >= 0 && written <= n);
```

### Wet dirt chain (§13.7) [CITED: milestone §13.7]
```cpp
auto x = preClipHp.process(wet);
x = preClipLp.process(x);
x = WetOverdrive::clipSample(x, WetOverdrive::kActiveCurve);
x = postClipLp.process(x);
x = postClipDcBlock.process(x);
```

## Environment Availability

Step 2.6: SKIPPED — no new external dependencies; CMake + Catch2 already required by repo.

## Security Domain

DSP-only phase; no new attack surface. ASVS input validation unchanged. No secrets or network I/O.

## Package Legitimacy Audit

No new external packages in this phase.

## Assumptions Log

| # | Claim | Section | Risk if Wrong |
|---|-------|---------|---------------|
| — | (none) | — | All loci verified in source + milestone spec this session |

## Sources

### Primary (HIGH confidence)
- `.planning/MILESTONE-SPEC-v1.0-interaction-truth.md` — ADR-V1-12…15, §13.3–13.8, §13.9 acceptance
- `source/SchroederTankCore.h`, `FixedRateAdapter.h`, `WetOverdrive.h`, `RateConverterPair.h` — defect loci [VERIFIED: codebase read]

### Secondary
- `.planning/phases/SENDBLOOM-24-reverb-state-wet-dirt-integrity/24-CONTEXT.md` — locked decisions
- `tests/FixedRateAdapterTest.cpp`, `ReleaseTruthTest.cpp`, `WetOverdriveTest.cpp` — regression anchors

## Confidence

HIGH — ADR text and §13 implementation sketches are explicit; bugs map to single-file edits; existing test harness supports new `[v1][contract]` tags without framework changes.

## RESEARCH COMPLETE

**Phase:** 24 — Reverb State & Wet-Dirt Integrity
**Confidence:** HIGH

### Key Findings
- All four defects are localized with exact line-level mismatches vs ADR-V1-12…15.
- Production audio uses `SchroederTankCore` on both host and ProperSRC paths; legacy accumulator shares predelay/LFO bugs for SRC-05 parity.
- `RateConverterPair::downsample` partial writes make `std::fill` mandatory; Off mode already clears.
- Wet dirt needs two one-pole HPF stages; `dirt_os` is already inert in DSP; `authentic_color` safety tests exist.

### File Created
`.planning/phases/SENDBLOOM-24-reverb-state-wet-dirt-integrity/24-RESEARCH.md`

### Plan Shape Summary
1. **Plan 01 — Predelay:** fixed 55 ms tap, continuous clock, lerp by `darkMix` in `SchroederTankCore` + legacy parity; `V1ContractPredelayTest`.
2. **Plan 02 — Mod + SRC:** `tankLfoDepthSamplesForRate`, ProperSRC `std::fill` before downsample; `V1ContractModInvariantTest` + `V1ContractSrcUnderfillTest`; keep SRC-06/HF green.
3. **Plan 03 — Wet dirt:** `OnePoleHighpass` HP100 + DC20 in `WetOverdriveState`; `V1ContractWetDirtTest`; preserve `[release][safe]` and diagnostics.

### Ready for Planning
Research complete. Planner can now create PLAN.md files.
