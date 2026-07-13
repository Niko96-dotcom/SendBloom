---
title: "SendBloom v1.0 — Interaction Truth, Realtime Safety & Release Candidate"
document_type: "GSD master milestone specification"
milestone: "v1.0"
phase_range: "19-27"
status: "ready_for_gsd_import"
created: "2026-07-12"
repository_snapshot: "repomix-output-Niko96-dotcom-SendBloom.md, generated 2026-07-12"
owner: "Niko Audio Labs"
product: "SendBloom"
formats: ["AU", "VST3"]
---

# SendBloom v1.0 — Interaction Truth, Realtime Safety & Release Candidate

> **Use this document as the source packet for `/gsd-new-milestone`.**
>
> This is not a suggestion list. It is the implementation contract for turning the current SendBloom repository into an honest, stable v1.0 release candidate.
>
> The milestone starts at **Phase 19** because the prior ProperSRC program occupied Phases 11–18. Do not renumber the phases unless the repository’s actual `.planning` state proves that another phase number has already been assigned.

---

## 0. Copy-ready `/gsd-new-milestone` brief

Create the next SendBloom milestone as:

**v1.0 — Interaction Truth, Realtime Safety & Release Candidate**

The current repository already has a strong DSP and verification foundation: JUCE 8 AU/VST3 builds, a host-rate Schroeder tank, optional ProperSRC 32,768 Hz mode, wet-only distortion, pre/post gate placement, pressure-send parameters, factory presets, strict Catch2 coverage, pluginval strictness 10, cross-platform CI, and clean-room documentation.

The product is not ready for v1 because several user-facing and realtime contracts are false or incomplete:

1. The on-screen pressure controller changes `send_connected` on press/release. Releasing it therefore restores always-on reverb instead of returning to dry while the existing tail decays.
2. MIDI CC1 mutates the APVTS raw atomic from `processBlock`, ignores event sample positions, and conflates realtime modulation with persistent host parameter state.
3. Host blocks larger than the prepared maximum silently produce no wet signal and may resize `dryBuffer` on the audio thread.
4. Several smoothed controls are advanced per sample but consumed only at the beginning of the block, creating block-size-dependent behavior.
5. The PostHard gate declares a 7 ms release but contains a short-circuit that snaps to zero in one sample.
6. The Input knob display and DSP mapping disagree, and the DSP mapping runs backwards relative to the intended product behavior.
7. Internal bypass is not true bypass: it collapses stereo to mono and still applies output gain.
8. Dark predelay can retain stale samples because the delay line is only clocked when predelay is active.
9. Host-rate modulation depth changes with sample rate.
10. ProperSRC underfill is not explicitly cleared.
11. The wet overdrive defines a pre-clip high-pass but does not implement it, and the asymmetric shaper has no post-clip DC blocker.
12. Shipping-facing assets and fallback drawing still contain direct third-party product naming/trade-dress references despite the repository’s own clean-room and release claims.
13. `VERSION` is still `0.0.1`; the release checklist, CI truth, AU validation, licensing decision, signing, notarization, and DAW smoke are not closed.
14. Sonic fidelity is validated against internally chosen constants rather than a captured hardware reference. v1 may ship as an original inspired effect, but it must not claim faithful hardware emulation without reference evidence.

Implement the milestone in Phases 19–27 exactly as specified below. Every phase must produce the normal GSD packet:

- `CONTEXT.md`
- one or more executable `PLAN.md` files
- implementation commits
- `SUMMARY.md`
- `VERIFICATION.md`

Every requirement ID in this document must be copied into `.planning/REQUIREMENTS.md`, mapped to exactly one owning phase, and referenced in the implementing plan frontmatter and verification report.

Do not call the milestone complete merely because tests pass. Phase 27 requires human DAW smoke, licensing, branding, and release sign-off.

---

# 1. Product truth from primary sources

This milestone is grounded in the official manufacturer product page and manual, not forum folklore.

## 1.1 Reference sources

| ID | Source | URL | Use |
|---|---|---|---|
| `RFX-PRODUCT` | Official Reverb-X product page | https://www.raingerfx.com/shop/p/reverb-x | Publicly described behavior, controls, FAQ |
| `RFX-MANUAL` | Official Reverb-X user manual | https://www.raingerfx.com/s/Reverb-X-Manual-Complete.pdf | Gate placement, pressure-send behavior, true bypass |
| `JUCE-PARAM` | JUCE `AudioProcessorParameter` documentation | https://docs.juce.com/master/classjuce_1_1AudioProcessorParameter.html | Host notification behavior |
| `CMAKE-PROJECT` | CMake `project()` documentation | https://cmake.org/cmake/help/latest/command/project.html | Numeric project-version syntax |
| `APPLE-AUVAL` | Apple Audio Unit validation guidance | https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitProgrammingGuide/AudioUnitDevelopmentFundamentals/AudioUnitDevelopmentFundamentals.html | AU validation requirement |
| `PLUGINVAL` | Tracktion pluginval repository | https://github.com/Tracktion/pluginval | Headless plugin validation |
| `APPLE-NOTARY` | Apple notarization documentation | https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution | External macOS distribution |
| `APPLE-HARDENED` | Apple Hardened Runtime documentation | https://developer.apple.com/documentation/security/hardened-runtime | Notarization prerequisite |

## 1.2 Behavioral contract derived from the official sources

These are the non-negotiable product behaviors that SendBloom is intentionally recreating in original software:

### Dry and level

- The unprocessed input remains present while the effect is engaged.
- `LEVEL` changes the amount of reverb return, not the dry signal level.
- Wet distortion must not distort the dry input.

### Wet distortion

- Distortion is after the reverb.
- `DISTN` blends between clean wet return and distorted wet return.
- The distortion circuit itself is effectively “full on”; the blend chooses how much of it is heard.
- Lowering input reduces how hard the wet distortion is driven.

### Input and overload

- `INPUT` controls the signal entering the effect path.
- It changes clipping/overload behavior.
- It also changes the effective gate trigger relationship.
- The overload indicator is a setup aid, not a destructive failure state.

### Bright/dark modes

- Bright mode is immediate and brighter.
- Dark mode is darker and predelayed.
- Switching modes must not resurrect stale audio or produce a discontinuity.

### Gate

- The gate is always part of the effect.
- In Pre mode it suppresses idle noise before the reverb/distortion path.
- In Post mode it closes after reverb/distortion and creates the hard “edited sample” cut.
- The hard close is intentionally dramatic, but the software must not rely on an accidental one-sample digital discontinuity to create that character.

### Pressure send

- Controller disconnected: ordinary always-on reverb pedal behavior.
- Controller connected and at rest: fully dry input feed, while an already-generated tail may continue.
- Pressing the controller diverts input into the wet path.
- Releasing the controller stops new wet-path input and immediately returns performance input to dry while the existing tail decays.
- The two feel modes represent firmer and softer pressure response.

### Bypass

- The reference behavior is true bypass.
- For the plugin, settled bypass means channel-preserving, unity, unprocessed input:
  - no mono collapse;
  - no input gain;
  - no gate;
  - no wet path;
  - no output gain;
  - no extra latency.

## 1.3 Claim boundary

The official sources describe behavior, not implementation constants. They do **not** establish SendBloom’s current:

- 55 ms predelay;
- Freeverb-derived delay ratios;
- RT60 curve exponent;
- damping frequencies;
- modulation depth/rate;
- distortion transfer function;
- pressure exponents;
- gate timing values;
- 32,768 Hz core assumption.

Those remain original design choices until measured against hardware. Automated tests may prove internal consistency, but must not be described as hardware fidelity evidence.

---

# 2. Current repository audit

## 2.1 Snapshot

Audit target:

- `repomix-output-Niko96-dotcom-SendBloom.md`
- generated `2026-07-12`
- repository contains `source/`, `tests/`, `resources/`, `docs/`, CI, release scripts, and the ProperSRC architecture.

The snapshot already includes a substantial test suite and prior RC documentation. Preserve that work.

## 2.2 Existing strengths that must not regress

| Area | Current strength | Preserve |
|---|---|---|
| Dry/wet topology | Dry tap is taken before wet input gain | Yes |
| Wet-only dirt | `WetOverdriveState` only processes wet | Yes |
| Gate placement | PreSoft and PostHard exist | Yes |
| Reverb abstraction | `IReverbEngine`, host engine, fixed-rate adapter | Yes |
| ProperSRC | r8brain host ↔ 32,768 Hz path | Yes |
| SRC policy | zero reported PDC with measured wet-only delay | Yes unless a separate product decision changes it |
| Engine switching | equal-power 35 ms wet crossfade | Yes |
| Safety default | `authentic_color` off on fresh load and presets | Yes |
| Presets | eight factory presets and state round-trip tests | Yes |
| CI | macOS, Windows, Linux VST3 build/test/pluginval | Yes |
| Release truth tests | static checks and clean-room documentation | Yes, but strengthen |
| Parameter IDs | IDs are marked immutable | Do not rename |
| Manufacturer/plugin codes | `NkMo` / `SbLm` | Do not change |
| Bundle ID | `com.nikoaudiolabs.sendbloom` | Do not change |

## 2.3 Confirmed defects and false contracts

### A. Pressure-pad release is semantically backwards

Current code:

```cpp
void PressureSendPad::mouseDown (...)
{
    setConnected (true);
    setAmountFromY (...);
}

void PressureSendPad::mouseUp (...)
{
    setConnected (false);
}
```

Current snapshot logic:

```cpp
s.sendGain = s.sendConnected
               ? ParameterCurves::sendGain (s.sendAmountNorm, s.sendFirmFeel)
               : 1.0f;
```

Therefore mouse release changes to `sendConnected=false`, which means `sendGain=1.0`. The plugin returns to full always-on wet feed instead of dry-at-rest pressure mode.

### B. `send_amount` defaults to full pressure

`ParameterLayout.cpp` currently gives `send_amount` a default of `1.0f`. This is incompatible with connected-controller resting semantics.

### C. MIDI mutates persistent parameter state from audio processing

Current processor behavior:

```cpp
sendParam->store (norm);
```

Problems:

- bypasses the actual parameter object;
- bypasses host notification and listeners;
- writes persistent APVTS state from realtime processing;
- ignores MIDI event sample positions;
- multiple CC1 events in one block collapse to the last stored value before audio processing begins.

The answer is **not** to call `setValueNotifyingHost()` inside `processBlock`. That would introduce host communication from the realtime callback and violate the existing release test.

### D. Oversized blocks remove the effect

When `numSamples > preparedMaxBlock_`, the processor:

- may call `dryBuffer.setSize()` from `processBlock`;
- advances state;
- sets `wet = 0.0f`;
- outputs dry-only audio;
- returns.

This is survival, not correct processing.

### E. Fake per-sample smoothing

The processor advances smoothers for:

- RT60/size;
- dark mix;
- authentic mode;
- distortion blend;
- pressure send;
- gate threshold;

for every sample, but captures only sample-zero values into `blockStart*` variables and passes constants to `GatedBloomChain::processBlock`.

Result: behavior changes with host block size and fast automation can zipper.

### F. Authentic-mode transition logic is unnecessarily indirect

The APVTS boolean is smoothed for 15 ms and converted back to a boolean at a 0.5 crossing. EngineCrossfade already supplies the audible 35 ms transition. The extra boolean smoother creates difficult edge logic without improving sound.

The feared “two crossings in one block” cannot occur with the current one-snapshot-per-block target, but the implementation should still be simplified and directly tested.

### G. PostHard declared release is dead code

`NoiseGate::prepare()` sets `releaseMs=7.0f`, but:

```cpp
if (floorGain <= 0.0f && ! isOpen)
{
    gain = 0.0f;
    return gain;
}
```

The first closed sample snaps directly to zero.

### H. Input display and DSP disagree

DSP:

```cpp
inputGainDb (0.0) = +9 dB
inputGainDb (0.5) = +3 dB
inputGainDb (1.0) = -3 dB
```

UI formatter:

```text
0.0 = +9 dB
0.5 = 0 dB
1.0 = -9 dB
```

The knob also runs opposite to the intended behavior that more Input should drive the wet path harder.

### I. Level model contains dead dry-gain logic

`levelEqualPower()` returns dry and wet gains, but the processor intentionally keeps dry unity and ignores `levelDryGain`. The dead field/smoother/test implies a dual-sided wet/dry crossfade that the product does not use.

### J. Bypass is not true bypass

Current output mixing:

- uses mono-summed dry when Extended Stereo is off;
- applies output gain even at bypass;
- therefore changes width, polarity relationships, and level.

### K. Dark predelay is conditionally clocked

The predelay line only receives and emits samples when `predelaySamples > 0.5f`. Turning Dark off freezes old delay-line contents. Turning it back on may release stale material.

### L. Modulation depth changes with processing rate

The LFO depth is a fixed `16.0f` samples. At 32,768 Hz that is approximately 0.488 ms. At 96 kHz it is approximately 0.167 ms. Host-rate and fixed-rate engines therefore have different modulation depth in time.

### M. ProperSRC output underfill is not cleared

`FixedRateAdapter` ignores the number returned by `downsample()`. If fewer than `n` host samples are written during priming or an edge case, the remainder is not explicitly zeroed.

### N. Wet dirt has incomplete filtering

`kPreClipHpHz` exists but is unused. The active asymmetric curve has no post-clip DC blocker.

### O. Release/legal truth is inconsistent

The repository claims product-facing metadata is SendBloom-only, but:

- `source/ui/PedalFaceplatePaint.cpp` draws `REVERB X`;
- the embedded asset is named `resources/ui/reverbx-faceplate.png`;
- `design-qa.md` references a `RAINGER FX` local path;
- the legal script only catches the hyphenated spelling and ignores binary filenames;
- the embedded faceplate can contain exact third-party visual material even when source-string scanning passes.

This milestone does not provide legal advice. It enforces the repository’s own declared clean-room and release policy.

### P. Version/release truth is stale

- `VERSION` is `0.0.1`.
- CMake consumes `CURRENT_VERSION` as a numeric `project(VERSION ...)`.
- `1.0.0-rc0` must not be placed directly into that numeric field.
- Release documentation contains stale test counts and incomplete sign-offs.
- AU validation, DAW smoke, licensing, signing, and notarization remain open.

---

# 3. Milestone objective

Ship a v1.0 release candidate where:

1. The pressure-send interaction behaves correctly from UI, host automation, and MIDI.
2. Realtime processing is correct for all host block sizes without audio-thread allocation.
3. Fast controls are smooth and block-size invariant.
4. Post gate sounds intentionally hard without a one-sample digital click.
5. Input, level, overload, gate, and bypass tell the truth.
6. Reverb mode switching cannot emit stale predelay or undefined SRC samples.
7. Shipping code/assets/metadata match the SendBloom clean-room position.
8. Automated, host, human, licensing, and packaging gates are documented and completed.
9. Fidelity claims never exceed the available evidence.

---

# 4. Scope lock

## 4.1 In scope

- pressure-controller state model;
- pressure UI and Advanced connection control;
- MIDI CC1 realtime modulation;
- event-position handling;
- asymmetric pressure smoothing;
- processor chunk/span architecture;
- oversized host blocks;
- removal of audio-thread resizing;
- per-sample send/distortion/threshold control;
- bounded control-rate RT60/dark updates;
- direct authentic-mode request and crossfade verification;
- true bypass;
- Input mapping and display;
- gate behavior and de-click;
- level-curve cleanup;
- predelay state continuity;
- modulation time invariance;
- SRC underfill clearing;
- wet dirt high-pass/DC blocking;
- preset updates;
- release-state migration policy;
- product-facing branding/resource cleanup;
- clean-room scanner strengthening;
- reference-capture tooling and honest claim classification;
- CI, AU validation, pluginval, DAW smoke;
- versioning, licensing, signing, notarization, packaging.

## 4.2 Explicitly out of scope

The agent must not sneak these into v1:

- new reverb topology;
- replacing the Schroeder tank with FDN as production default;
- changing the accepted ProperSRC quality preset;
- making 32k Color default-on;
- implementing `Extended Stereo`;
- implementing `Dirt OS`;
- adding oversampling as a new feature;
- CLAP;
- AAX;
- new preset-browser architecture;
- cloud licensing;
- storefront work;
- telemetry;
- visual redesign beyond required original branding/state truth;
- changing immutable parameter IDs;
- changing plugin codes or bundle ID;
- claiming component-level circuit emulation;
- claiming exact hardware fidelity without Phase 26 evidence.

## 4.3 Deferred work must stay visibly deferred

`extended_stereo` and `dirt_os` may remain APVTS parameters for compatibility, but:

- controls remain disabled;
- presets remain `0`;
- docs call them deferred;
- no agent may partially implement them during this milestone.

---

# 5. Architecture decisions locked for v1

## ADR-V1-01 — Pressure mode state

Retain existing APVTS IDs:

- `send_connected`
- `send_amount`
- `send_feel`

Interpret them as:

| State | Meaning |
|---|---|
| `send_connected = 0` | pressure controller disconnected; wet feed is always on |
| `send_connected = 1`, effective pressure = 0 | controller connected and resting; no new input enters wet path |
| `send_connected = 1`, effective pressure > 0 | controller pressed; pressure amount drives wet input |
| release to 0 | stop new wet input; do not clear tank |

`send_amount` is pressure, not connection state.

Default:

```text
send_connected = 0
send_amount    = 0
send_feel      = Firm
```

## ADR-V1-02 — UI controller behavior

- Advanced drawer gains a persistent `PRESSURE MODE` or `CONTROLLER CONNECTED` toggle attached to `send_connected`.
- Pressing the on-screen pad may automatically enable pressure mode for discoverability.
- `mouseDown`:
  - stop visual fade;
  - if needed, enable pressure mode on the message thread;
  - begin `send_amount` parameter gesture;
  - set pressure from pointer Y.
- `mouseDrag`:
  - update pressure.
- `mouseUp`:
  - set `send_amount` to `0`;
  - end gesture;
  - do **not** disconnect;
  - start visual-only bloom fade.
- Visual pressed state derives from live pressure (`send_amount > epsilon`) or `PressureSendPad::isPressed()`, not from `send_connected`.
- Connection state gets its own visible indicator/toggle.

## ADR-V1-03 — MIDI is modulation, not APVTS mutation

MIDI CC1 must not change APVTS state in `processBlock`.

Maintain a realtime-only MIDI pressure target:

```text
midiPressureTarget ∈ [0, 1]
```

Effective raw pressure while connected:

```text
rawPressure = max(hostOrUiSendAmount, midiPressureTarget)
```

Rationale:

- UI/automation and MIDI may both “press” the pad;
- CC1 value 0 releases MIDI pressure;
- host automation remains persistent and host-visible;
- no host notification occurs from audio processing.

If `send_connected = 0`, effective send gain is `1.0` and CC1 does not affect sound.

## ADR-V1-04 — Asymmetric pressure smoothing

Apply pressure smoothing after source combination and before the Firm/Soft curve:

- attack: `3.0 ms`;
- release: `25.0 ms`.

Implementation may use a small allocation-free `AsymmetricSmoother` class with separate one-pole coefficients or deterministic linear ramps.

Do not use one symmetric 25 ms `SmoothedValue`.

## ADR-V1-05 — Processing spans

Refactor `PluginProcessor::processBlock` into a no-allocation span loop.

Hard constants:

```cpp
static constexpr int kControlQuantum = 128;
```

Each span length is the minimum of:

- samples remaining in the host block;
- `preparedMaxBlock_`;
- `kControlQuantum`;
- distance to the next relevant CC1 event.

Benefits:

- oversized host blocks are processed correctly;
- MIDI event positions become meaningful;
- block-constant reverb controls update at no worse than approximately 2.9 ms at 44.1 kHz;
- results become independent of host block size.

## ADR-V1-06 — Dynamic control arrays

`GatedBloomChain::processBlock` must accept per-sample arrays for controls that sit outside the reverb engine:

```cpp
void processBlock (
    const float* monoIn,
    const float* envelope,
    const float* distnBlend,
    const float* sendGain,
    const float* thresholdDb,
    float* wetOut,
    int numSamples,
    float rt60Seconds,
    float darkMix,
    bool authenticColor,
    bool gatePreSoft) noexcept;
```

Equivalent naming/order is acceptable only if the semantic contract is identical.

Per-sample:

- input gain;
- envelope;
- pressure send;
- distortion blend;
- gate threshold;
- wet level;
- output gain;
- bypass mix.

Per-span:

- RT60;
- dark mix;
- authentic engine selection;
- gate placement;
- disabled feature flags.

## ADR-V1-07 — Authentic mode request

Remove the 15 ms boolean smoother and 0.5 threshold crossing logic for `authentic_color`.

Maintain a processor-side requested state:

```cpp
bool requestedAuthenticColor_;
```

At the beginning of a host block, after snapshot capture:

```text
if snapshot.authenticColor != requestedAuthenticColor_:
    chain.requestEngineCrossfade(snapshot.authenticColor)
    updateReportedLatency(snapshot.authenticColor)
    requestedAuthenticColor_ = snapshot.authenticColor
```

The existing 35 ms `EngineCrossfade` remains the only audible transition.

## ADR-V1-08 — Canonical Input curve

Use one canonical function everywhere:

```cpp
inputGainDb(norm) = -9.0f + 18.0f * smoothstep(norm)
```

Required anchors:

| Normalized | Display/DSP |
|---:|---:|
| 0.0 | -9.00 dB |
| 0.5 | 0.00 dB |
| 1.0 | +9.00 dB |

The UI formatter must call `ParameterCurves::inputGainDb()` rather than duplicate arithmetic.

The gate detector remains after `InputStage`, so increasing Input naturally increases the detector level. `input_threshold` remains an advanced Gate Sens offset/threshold control.

## ADR-V1-09 — Unity dry level

Replace misleading dual-sided level logic with:

```cpp
wetGain = sin (halfPi * levelNorm);
dryGain = 1.0f;
```

Remove dead `dryGain` state and smoothing. `LEVEL=0` means no wet return; it does not mute dry.

## ADR-V1-10 — True bypass location

Compute the engaged output first, including output gain. Then crossfade between:

- original per-channel input copy;
- fully engaged processed output.

Settled bypass must ignore output gain and preserve each input channel.

```text
final[ch] = originalDry[ch] * (1 - engagedMix)
          + processed[ch]   * engagedMix
```

`engagedMix` ramps over 5 ms.

## ADR-V1-11 — PostHard de-click

Replace the one-sample close with a `0.75 ms` deterministic ramp to zero.

- opening ramp remains `0.5 ms`;
- closing must reach zero within `1.0 ms`;
- total post-gate chop must still occur within the existing `15 ms` budget after input silence;
- no exposed user control is added.

The hard-edited character comes from the detector and sub-millisecond envelope, not from a single-sample discontinuity.

### ADR-V1-11a — Gate feel revision (single movable gate + hold)

Amends 11 after the on-audio diagnostic showed the "hard close" was actually
~30 ms from a hot sustained note: the low fixed threshold forced the detector to
decay a large dynamic range before the close ramp could even begin. The chop was
fast; the *decision* to chop was late.

- **Single shared gate.** `preGate`/`postGate` are collapsed into one `NoiseGate`
  whose profile follows the Gate switch (`setProfile`), so state carries across
  toggles instead of two instances freezing stale open/closed state. This models
  the hardware's one physical circuit that *moves* pre↔post.
- **Opening ramp is linear ~`0.2 ms`** (was a one-pole toward unity, which rounded
  the wet transient's front edge away). Closing stays the `0.75 ms` linear chop.
- **Hold stage (`kHoldMs = 5 ms`).** Once the key falls below the close threshold
  the gate stays open for the hold before closing. This lets the detector release
  be fast (`2 ms`) without stuttering low notes: 5 ms clears the ~2.7 ms
  below-threshold window between the peaks of an unclipped low E (82 Hz) with
  margin (verified stutter-free down to INPT 0.35 in `GateCloseTimingDiagnostic`).
- **Detector release `10 ms → 2 ms`** (attack `5 ms → 1 ms`) so the envelope
  reaches the close threshold quickly after a mute.
- **Hysteresis `3 dB → 2 dB` (`kHysteresisDb`)** for boundary sputter without
  full chatter.

CORE-11 is unchanged and still holds: it bounds the ramp "within 1 ms **after the
close command**." The hold precedes the close command (the `isOpen→false`
transition); it does not stretch the ramp. Net result: measured post-gate close
on real audio is ~`15.6 ms` (worst case: instant mute of a hot sustained note),
down from ~`30 ms`. `kHoldMs`, `kHysteresisDb`, and `ParameterCurves::kGateReferenceDb`
are the ears-on tuning knobs.

### ADR-V1-11b — Threshold demoted to a trim

The hardware has no threshold knob; input level alone drives the guitar into a
fixed gate. The `input_threshold` parameter is therefore demoted from an
independent `-52..-18 dB` threshold to a `±6 dB` calibration trim around a fixed
`-45 dB` reference (`ParameterCurves::kGateReferenceDb`), default centred (0 dB).
Input gain stays the dominant sensitivity control because the detector envelope is
measured post-input-gain — turning INPT up effectively lowers the threshold. The
parameter is renamed "Gate Trim" for the DAW; the ID `input_threshold` is retained
for state/preset compatibility.

## ADR-V1-12 — Predelay topology

Use a fixed 55 ms dark delay tap that is always clocked.

Every sample:

```text
predelay.push(input)
delayed = predelay.pop()
tankInput = lerp(input, delayed, darkMix)
```

Do not set delay time to `darkMix * 55 ms`. Do not stop clocking the line in bright mode.

## ADR-V1-13 — Modulation time invariant

Replace the fixed sample-depth contract with a time-depth contract:

```cpp
kTankLfoDepthSeconds = 16.0 / 32768.0; // 0.00048828125 s
depthSamples = kTankLfoDepthSeconds * processingRate;
```

At the fixed internal rate, this still equals 16 samples.

## ADR-V1-14 — SRC underfill

Before ProperSRC downsampling:

```cpp
std::fill(out, out + n, 0.0f);
const int written = converters.downsample(...);
```

Assert/debug-check:

```text
0 <= written <= n
```

Unwritten output remains deterministic zero.

## ADR-V1-15 — Wet dirt filtering

Add:

- pre-clip high-pass at `100 Hz` using the already-declared constant;
- existing pre-clip low-pass at `6.5 kHz`;
- active clipper;
- post-clip low-pass at `7.5 kHz`;
- post-clip DC blocker/high-pass at `20 Hz`.

Do not enable oversampling in v1.

## ADR-V1-16 — Versioning

- `VERSION` file becomes numeric `1.0.0`.
- RC identity lives in the Git tag and release name: `v1.0.0-rc0`.
- Do not feed `1.0.0-rc0` into CMake `project(VERSION ...)`.

## ADR-V1-17 — Fidelity classification

At Phase 26 closeout, assign one status:

| Status | Requirement |
|---|---|
| `original-inspired` | no hardware capture required; all public wording remains generic |
| `hardware-compared` | reference capture and objective comparison complete |
| `fidelity-claim-approved` | hardware comparison plus Niko human listening approval and explicit claim review |

Default status is `original-inspired`.

---

# 6. Requirement catalogue

Every requirement below is mandatory unless marked `human_needed`.

## 6.1 Baseline and traceability (`BASE`)

- **BASE-01** Record current commit, branch, build configuration, test count, CI state, and known manual gaps before modifications.
- **BASE-02** Create `.planning/REQUIREMENTS.md` entries for every requirement in this document.
- **BASE-03** Map each requirement to exactly one phase and at least one verification artifact.
- **BASE-04** Preserve all existing passing ProperSRC, HF, dry-integrity, and release-truth tests unless a requirement explicitly updates their contract.
- **BASE-05** Add a durable v1 verifier script that runs the complete automated gate set.
- **BASE-06** Do not hard-code the expected total number of tests in documentation or scripts.
- **BASE-07** Record baseline audio metrics for representative factory presets before changing DSP.
- **BASE-08** Mark human-only gates as `human_needed`, never silently pass them.

## 6.2 Pressure interaction (`SEND`)

- **SEND-01** `send_connected=false` produces always-on wet input.
- **SEND-02** `send_connected=true` and pressure 0 produces no new wet input.
- **SEND-03** Pressure >0 sends input into the wet path.
- **SEND-04** Releasing pressure stops new wet input but preserves existing tail state.
- **SEND-05** UI mouse release sets `send_amount=0` and leaves `send_connected=true`.
- **SEND-06** UI pressed overlay follows pressure/pressed state, not connection state.
- **SEND-07** Advanced UI exposes persistent pressure-mode connection.
- **SEND-08** Pressing the on-screen pad may auto-connect pressure mode without disconnecting on release.
- **SEND-09** Firm and Soft curves remain audibly and numerically distinct.
- **SEND-10** Pressure attack is 3 ms and release is 25 ms.
- **SEND-11** Factory pressure presets load at rest with `send_amount=0`.
- **SEND-12** Disconnecting pressure mode restores ordinary always-on reverb.
- **SEND-13** `send_amount` APVTS ID remains unchanged.
- **SEND-14** Pressure behavior is invariant across host block sizes.

## 6.3 MIDI (`MIDI`)

- **MIDI-01** MIDI CC1 controls realtime pressure only when pressure mode is connected.
- **MIDI-02** `processBlock` never calls `setValueNotifyingHost`.
- **MIDI-03** `processBlock` never writes `send_amount` raw APVTS state.
- **MIDI-04** CC1 event sample positions are respected.
- **MIDI-05** Multiple CC1 events in one block are applied in order.
- **MIDI-06** CC1 value 0 releases MIDI pressure.
- **MIDI-07** Host/UI pressure and MIDI pressure combine via `max`.
- **MIDI-08** MIDI modulation does not dirty saved plugin state.
- **MIDI-09** Non-CC1 MIDI messages do not change pressure.
- **MIDI-10** MIDI behavior remains finite and deterministic at block sizes 1–2048.

## 6.4 Realtime/block engine (`RT`)

- **RT-01** No `setSize`, `resize`, `assign`, `make_unique`, `push_back`, or equivalent allocation path runs in `PluginProcessor::processBlock`.
- **RT-02** Host blocks larger than `preparedMaxBlock_` retain full wet processing.
- **RT-03** Processing a 2048-sample host block prepared at 512 matches equivalent smaller blocks within tolerance.
- **RT-04** Span processing respects CC1 event boundaries.
- **RT-05** Span processing uses at most 128 samples for control-rate reverb values.
- **RT-06** Dynamic send/distortion/threshold controls are consumed per sample.
- **RT-07** Input gain, level, output gain, and bypass remain per sample.
- **RT-08** Authentic-mode changes request exactly one engine target transition per parameter change.
- **RT-09** Reported latency remains zero under ADR-003 across transitions.
- **RT-10** Engine crossfade begins within the first processed block after the parameter change.
- **RT-11** Engine converges to the final requested target after rapid block-to-block toggles.
- **RT-12** Crossfade completion resets only the now-idle engine.
- **RT-13** Crossfade completion performs zero heap allocations.
- **RT-14** Output remains finite through 10,000-block stress with toggles, MIDI, bypass, and oversized blocks.
- **RT-15** `preparedMaxBlock_ <= 0` is handled safely without allocation or undefined access.

## 6.5 Input, level, gate, bypass (`CORE`)

- **CORE-01** Input mapping is `-9/0/+9 dB` at `0/0.5/1`.
- **CORE-02** Input display calls the canonical DSP curve.
- **CORE-03** Increasing Input increases wet-path drive.
- **CORE-04** Increasing Input increases the detector level at fixed raw input.
- **CORE-05** Dry tap remains before Input gain.
- **CORE-06** Gate Sens remains an advanced parameter using the existing ID.
- **CORE-07** Gate Sens display reports the canonical threshold in dB.
- **CORE-08** Level changes wet return only.
- **CORE-09** Dead dry-gain fields/smoothers/tests are removed.
- **CORE-10** PostHard close uses a 0.75 ms ramp, not a one-sample snap.
- **CORE-11** PostHard reaches zero no later than 1 ms after the close command.
- **CORE-12** Post gate still chops wet within 15 ms after silence onset.
- **CORE-13** PreSoft retains its long unobtrusive close behavior.
- **CORE-14** Settled bypass is channel-preserving.
- **CORE-15** Settled bypass is unity within floating tolerance.
- **CORE-16** Settled bypass ignores Input, Distn, Gate, Level, and Output settings.
- **CORE-17** Bypass transitions remain click-bounded.
- **CORE-18** Engaged mono-first behavior is unchanged unless `extended_stereo` is later implemented.

## 6.6 Reverb and dirt integrity (`DSP`)

- **DSP-01** Predelay line is clocked continuously in bright and dark modes.
- **DSP-02** Dark mode uses a fixed 55 ms delayed tap blended by dark mix.
- **DSP-03** Re-enabling Dark after bright operation emits no stale frozen burst.
- **DSP-04** Bright/dark automation remains finite and click-bounded.
- **DSP-05** LFO modulation depth is invariant in seconds across sample rates.
- **DSP-06** ProperSRC output is pre-cleared.
- **DSP-07** ProperSRC unwritten samples remain zero.
- **DSP-08** Existing ProperSRC imaging/HF gates remain green.
- **DSP-09** Wet dirt implements the 100 Hz pre-clip high-pass.
- **DSP-10** Wet dirt implements a 20 Hz post-clip DC blocker.
- **DSP-11** Wet dirt long-run DC offset is below the defined gate.
- **DSP-12** Dry path remains unaffected by all wet filtering.
- **DSP-13** `dirt_os` stays disabled and unimplemented.
- **DSP-14** `authentic_color` remains off by default.
- **DSP-15** All factory presets keep `authentic_color=0`.

## 6.7 State, presets, UI, and branding (`UX`)

- **UX-01** Parameter IDs remain unchanged.
- **UX-02** Default `send_amount` becomes 0.
- **UX-03** Factory preset XML and `FactoryPresets.cpp` remain identical in recalled state.
- **UX-04** Pressure presets use connected mode and zero resting pressure.
- **UX-05** Always-on presets use disconnected mode.
- **UX-06** UI explains Pressure Mode without third-party controller naming.
- **UX-07** No product-facing source string contains third-party product/brand/controller names.
- **UX-08** No shipping resource filename contains those names.
- **UX-09** Procedural fallback says `SENDBLOOM`, not the referenced product name.
- **UX-10** Exact reference faceplate asset is removed from the shipping binary unless written permission and a separate legal decision are recorded.
- **UX-11** A Niko-approved original SendBloom faceplate is the production asset, or the original procedural faceplate ships.
- **UX-12** Legal metadata scan normalizes punctuation/spacing/case and scans filenames.
- **UX-13** `design-qa.md` contains portable repository-relative paths and current evidence.
- **UX-14** Editor hotspots and state overlays remain hittable and correctly aligned after asset replacement.
- **UX-15** Existing preset sessions are explicitly classified as pre-v1 development state; no hidden migration promise is invented.
- **UX-16** README and clean-room docs describe only verified behavior.

## 6.8 Reference and claim evidence (`REF`)

- **REF-01** Add a reproducible reference-capture protocol.
- **REF-02** Add tooling to measure predelay, decay, spectral centroid, gate envelope, harmonic ratios, and DC offset.
- **REF-03** Store derived metrics with capture metadata and knob positions.
- **REF-04** Never commit third-party firmware, EEPROM, bytecode, schematics, or proprietary dumps.
- **REF-05** Hardware recordings, if made, are user-created audio captures only.
- **REF-06** Compare at least five Size positions in bright and dark modes if hardware is available.
- **REF-07** Compare at least five Input and Distn combinations if hardware is available.
- **REF-08** Compare pre/post gate timing if hardware is available.
- **REF-09** Compare controller press/release behavior if hardware is available.
- **REF-10** Niko performs a blind or level-matched listening review.
- **REF-11** Closeout assigns one ADR-V1-17 fidelity status.
- **REF-12** Public copy matches the assigned status.

## 6.9 Release (`REL`)

- **REL-01** `VERSION` is numeric `1.0.0`.
- **REL-02** RC tag is `v1.0.0-rc0`.
- **REL-03** CMake config/build succeeds from a clean directory.
- **REL-04** Full Catch2 suite passes in Release.
- **REL-05** VST3 pluginval strictness 10 passes locally.
- **REL-06** AU validation uses the actual AU type discovered by `auval -a`, not an assumed `aufx`.
- **REL-07** AU pluginval or equivalent validation passes locally.
- **REL-08** GitHub Actions is green on macOS, Windows, and Linux.
- **REL-09** Logic Pro AU smoke passes.
- **REL-10** Cubase VST3 smoke passes.
- **REL-11** REAPER VST3 smoke passes.
- **REL-12** Minimum 10-minute abuse/soak passes in each host.
- **REL-13** JUCE commercial-vs-GPL decision is documented and approved by Niko.
- **REL-14** Repository/distribution license matches the JUCE decision.
- **REL-15** macOS binaries are Developer ID signed for public distribution.
- **REL-16** macOS distribution package is notarized and stapled.
- **REL-17** Release artifacts have SHA-256 checksums.
- **REL-18** Release checklist contains real tester/date/result evidence.
- **REL-19** Working tree is clean at tag.
- **REL-20** No human gate is represented as automated success.

---

# 7. Roadmap

| Phase | Name | Primary risk removed | Owning requirement groups |
|---:|---|---|---|
| 19 | Baseline, Contracts & Failure Harness | agent starts from false assumptions | `BASE` |
| 20 | Pressure Send State Truth | signature interaction is backwards | `SEND`, state part of `UX` |
| 21 | Realtime Span Engine & True Bypass | oversized blocks, allocations, mono bypass | `RT`, bypass part of `CORE` |
| 22 | MIDI & Per-Sample Control Delivery | audio-thread APVTS mutation and block-rate zipper | `MIDI`, dynamic-control `RT` |
| 23 | Input, Level & Gate Truth | reversed Input, dead level model, click gate | remaining `CORE` |
| 24 | Reverb State & Wet-Dirt Integrity | stale predelay, rate-dependent mod, SRC garbage, DC | `DSP` |
| 25 | Presets, UI, Branding & Release Truth | false clean-room green and misleading state | remaining `UX` |
| 26 | Reference Capture & Sonic Classification | internal consistency mistaken for emulation fidelity | `REF` |
| 27 | RC Verification, Licensing & Distribution | unverified release | `REL` |

Phases are ordered by dependency. Do not begin Phase 27 while any automated requirement from Phases 19–25 is red.

---

# 8. Phase 19 — Baseline, Contracts & Failure Harness

## 8.1 Goal

Freeze the current truth, create failing tests for every confirmed defect, and install durable milestone traceability before production code changes.

## 8.2 Required files

Create:

```text
.planning/phases/19-baseline-contracts/
  19-CONTEXT.md
  19-01-PLAN.md
  19-01-SUMMARY.md
  19-VERIFICATION.md

docs/reports/v1.0-baseline.md
scripts/verify-v1.sh
tests/V1InteractionContractTest.cpp
tests/V1RealtimeContractTest.cpp
tests/V1ReleaseContractTest.cpp
```

Existing test files may also be extended where ownership is clearer.

## 8.3 Baseline report contents

`docs/reports/v1.0-baseline.md` must record:

- Git commit SHA.
- Branch.
- Dirty/clean status.
- Compiler, CMake, JUCE, OS, architecture.
- Release configure command.
- Build command.
- Exact passing/failing test list.
- Test count discovered from CTest; do not hard-code it elsewhere.
- VST3 pluginval result.
- AU validation status.
- CI status for all matrix legs.
- Current `VERSION`.
- Current factory preset parameter matrix.
- Current output for:
  - pressure press/release;
  - oversized 2048 block prepared at 512;
  - bypass with anti-phase stereo input;
  - Input display anchors;
  - PostHard first closed sample;
  - Dark off/on stale-tail probe;
  - long-run distorted-wet DC offset.

## 8.4 Add failing tests before fixes

### Test: pressure release

Use a deterministic test reverb engine whose state exposes:

- total input energy accepted;
- a decaying output tail.

Required sequence:

1. `send_connected=true`.
2. Press/send=1 for a burst.
3. Release/send=0.
4. Continue nonzero dry input.
5. Assert:
   - reverb accepted no new input after release;
   - wet tail remains nonzero;
   - dry output remains nonzero.

### Test: oversized block parity

- Prepare plugin at 48 kHz / 512.
- Render deterministic 2048-sample input in one call.
- Render the same input through a second instance in four 512 calls.
- Use identical initial state.
- Assert max absolute difference `< 2e-5`.
- Baseline is expected to fail because wet becomes zero.

### Test: true bypass

- Stereo input: left sine, right inverted sine or different frequencies.
- Set Output `+12 dB`, Input max, Distn max, Post gate.
- Engage internal bypass and render longer than the 5 ms ramp.
- Assert each output channel matches its original input `< 1e-6`.
- Assert left and right are not collapsed.

### Test: PostHard ramp

- Open gate fully.
- Command closure with low envelope.
- Assert first closed sample gain is `>0` and `<1`.
- Assert per-sample gain delta `<=0.05`.
- Assert gain `<=1e-4` within 1 ms.
- Baseline is expected to fail on first-sample behavior.

### Test: canonical Input

Assert desired anchors `-9, 0, +9`. Baseline expected to fail.

### Test: MIDI state purity

- Send CC1 while connected.
- Assert APVTS `send_amount` remains unchanged.
- Assert effective DSP pressure changes.
- Baseline expected to fail.

### Test: source-policy checks

Baseline tests should identify:

- `dryBuffer.setSize` inside `processBlock`;
- raw `sendParam->store`;
- product-facing `REVERB X`;
- shipping resource filename containing `reverbx`.

Do not “fix” tests by weakening them to match current behavior.

## 8.5 Durable verifier

`scripts/verify-v1.sh` must:

```bash
#!/usr/bin/env bash
set -euo pipefail
```

Run:

1. clean-room/legal metadata script;
2. CMake configure if build directory absent;
3. Release build;
4. full CTest with output on failure;
5. focused v1 test labels;
6. ProperSRC acceptance gates;
7. optional pluginval when path is supplied by environment;
8. documentation existence checks.

Environment options:

```text
BUILD_DIR
PLUGINVAL_BIN
RUN_PLUGINVAL=0|1
```

Do not silently skip requested pluginval.

## 8.6 Phase 19 acceptance

- All preexisting tests still pass.
- New defect tests exist and fail for the intended reasons.
- Failures are documented, not hidden.
- Requirement map is complete.
- `verify-v1.sh` runs and truthfully reports current red gates.

---

# 9. Phase 20 — Pressure Send State Truth

## 9.1 Goal

Make the signature pressure-send behavior correct and understandable before touching MIDI or broad processor architecture.

## 9.2 Preferred implementation files

Modify:

```text
source/ParameterLayout.cpp
source/ParameterSnapshot.h
source/ParameterCurves.h
source/PressureSend.h
source/ui/PressureSendPad.cpp
source/ui/PressureSendPad.h
source/ui/AdvancedDrawer.cpp
source/ui/AdvancedDrawer.h
source/ui/PedalFaceplatePaint.cpp
source/ui/PedalFaceplatePaint.h
source/PluginEditor.cpp
source/PluginEditor.h
resources/presets/*.xml
source/FactoryPresets.cpp
tests/PressureSendTest.cpp
tests/ParameterSnapshotTest.cpp
tests/ParameterLayoutTest.cpp
tests/PluginEditorTest.cpp
tests/PresetTest.cpp
tests/V1InteractionContractTest.cpp
```

## 9.3 Parameter changes

In `ParameterLayout.cpp`:

```cpp
send_amount default: 1.0f -> 0.0f
```

Do not rename IDs.

## 9.4 Snapshot contract

`ParameterSnapshot` should capture:

```cpp
bool sendConnected;
float sendAmountNorm;
bool sendFirmFeel;
```

It may compute a host/UI pressure curve for non-MIDI tests, but final effective pressure is owned by the realtime pressure controller in Phase 22.

Disconnected mode:

```cpp
sendGain = 1.0f;
```

Connected mode at rest:

```cpp
sendGain = 0.0f;
```

## 9.5 Add an allocation-free pressure controller

Preferred new file:

```text
source/PressureController.h
```

Suggested API:

```cpp
class PressureController
{
public:
    void prepare (double sampleRate) noexcept;
    void reset() noexcept;

    void setConnected (bool connected) noexcept;
    void setHostPressureTarget (float normalized) noexcept;
    void setMidiPressureTarget (float normalized) noexcept;
    void setFirmFeel (bool firm) noexcept;

    float processSample() noexcept;
    float getCurrentPressure() const noexcept;
    float getCurrentGain() const noexcept;
};
```

Behavior:

```cpp
if (! connected)
    return 1.0f;

const float raw = std::max (hostPressureTarget, midiPressureTarget);
const float smoothed = asymmetricSmoother.process (raw);
return ParameterCurves::sendGain (smoothed, firm);
```

For Phase 20, MIDI target remains zero. Phase 22 wires it.

## 9.6 UI behavior

### `PressureSendPad`

Add:

```cpp
bool isPressed() const noexcept;
float getDisplayAmount() const noexcept;
```

`mouseDown`:

```text
stop visual fade
isPressed = true
begin send_amount gesture
enable send_connected if false
write amount from pointer Y
repaint
```

`mouseDrag`:

```text
write amount from pointer Y
repaint
```

`mouseUp`:

```text
isPressed = false
write send_amount = 0
end gesture
do not write send_connected
start visual tail fade
repaint
```

The 200 ms `displayAmount` fade is visual only. DSP release timing comes from `PressureController`.

### Advanced drawer

Add a toggle attached to `send_connected`:

```text
PRESSURE MODE
```

Suggested user copy:

- Off: `Always-on reverb`
- On: `Dry until pressure`

Use a tooltip, not a paragraph on the faceplate.

### Overlay

Replace:

```cpp
if (sendConnected)
    drawFootswitchPressedOverlay(...)
```

with pressure/pressed truth:

```cpp
if (sendAmount > epsilon || pressurePad.isPressed())
    drawFootswitchPressedOverlay(...)
```

If paint architecture cannot access `PressureSendPad`, use APVTS `send_amount > 0.001f`.

## 9.7 Preset matrix

Update both XML resources and `FactoryPresets.cpp` to match exactly:

| Preset | `send_connected` | `send_amount` | Behavior on load |
|---|---:|---:|---|
| Sparkle Verb | 0 | 0 | always-on |
| Cut Sample Gate | 0 | 0 | always-on |
| Spacerock Burn | 0 | 0 | always-on |
| Dry Dub Sends | 1 | 0 | dry until pressure |
| Dark Bloom | 0 | 0 | always-on |
| Firm Pressure | 1 | 0 | dry until pressure |
| Gated Room | 0 | 0 | always-on |
| Hot Clip | 1 | 0 | dry until pressure |

Any change to this matrix requires Niko approval and updated documentation.

## 9.8 Tests

Add/modify:

- `send disconnected is unity wet feed`;
- `send connected at zero is zero wet feed`;
- `send press feeds tank`;
- `send release preserves tail and rejects new input`;
- `mouseUp keeps send_connected true`;
- `mouseUp sets send_amount zero`;
- `advanced pressure toggle changes connection state`;
- `overlay state follows amount, not connected`;
- `factory pressure presets rest at zero`;
- `factory program/XML parity`;
- `Firm vs Soft midpoint remains distinct`;
- `pressure attack/release timing`.

## 9.9 Phase 20 acceptance

- UI press/release behavior is correct without MIDI.
- Pressure presets load dry.
- Existing tail survives release.
- No parameter ID changed.
- All preset/state tests green.

---

# 10. Phase 21 — Realtime Span Engine & True Bypass

## 10.1 Goal

Replace the oversized-block fallback and audio-thread resizing with a single correct span architecture. Move bypass to the final channel-preserving output stage.

## 10.2 Preferred implementation files

Modify/create:

```text
source/PluginProcessor.cpp
source/PluginProcessor.h
source/GatedBloomChain.h
source/BypassCrossfade.h
source/SmoothedParameterBank.h
tests/BlockIntegrationTest.cpp
tests/BypassCrossfadeTest.cpp
tests/MonoBusTest.cpp
tests/IntegrationAllocScanTest.cpp
tests/RealtimeStressTest.cpp
tests/V1RealtimeContractTest.cpp
tests/helpers/AllocationCounter.h
tests/helpers/AllocationCounter.cpp
```

## 10.3 Preallocation contract

At `prepareToPlay`:

- require `samplesPerBlock > 0`;
- set `preparedMaxBlock_ = max(1, samplesPerBlock)`;
- allocate all scratch storage once;
- allocate dry scratch for maximum output channels and `preparedMaxBlock_`;
- prepare chain for `preparedMaxBlock_`;
- initialize requested authentic state;
- reset pressure and bypass state.

At `releaseResources`:

- storage may be cleared because this is not realtime processing;
- reset `preparedMaxBlock_` to zero.

Inside `processBlock`:

- no `setSize`;
- no vector growth;
- no object construction that allocates;
- no parameter notification;
- no logging;
- no file I/O.

## 10.4 Span-loop pseudocode

```cpp
void PluginProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midi)
{
    ScopedNoDenormals noDenormals;

    if (preparedMaxBlock_ <= 0)
    {
        // Safe identity/pass-through; clear extra outputs only.
        return;
    }

    clearExtraOutputChannels(...);

    const auto snapshot = ParameterSnapshot::capture (apvts);
    smoothedBank.setTargets (snapshot);
    pressureController.setConnected (snapshot.sendConnected);
    pressureController.setHostPressureTarget (snapshot.sendAmountNorm);
    pressureController.setFirmFeel (snapshot.sendFirmFeel);

    handleAuthenticRequestIfChanged (snapshot.authenticColor);

    int offset = 0;
    MidiCursor cursor (midi);

    while (offset < buffer.getNumSamples())
    {
        applyAllCc1EventsAt (offset, cursor);

        const int nextMidi = nextRelevantCc1SampleOrEnd (cursor);
        const int span = min (
            buffer.getNumSamples() - offset,
            preparedMaxBlock_,
            kControlQuantum,
            nextMidi - offset);

        if (span == 0)
        {
            applyAllCc1EventsAt (offset, cursor);
            continue;
        }

        processSpan (buffer, offset, span, snapshot);
        offset += span;
    }

    clipHoldFlag.store (...);
}
```

Do not allocate a list of MIDI events. Iterate `MidiBuffer` directly.

## 10.5 `processSpan` contract

For each span:

1. Copy original per-channel samples at `buffer[offset:offset+span]` into preallocated dry scratch at `[0:span]`.
2. Mono-sum only for the engaged mono wet path.
3. Fill per-sample control scratch arrays.
4. Process `GatedBloomChain`.
5. Build engaged output:
   - mono-first dual-mono unless Extended Stereo is eventually implemented;
   - unity dry plus wet level;
   - engaged Output gain.
6. Apply final bypass crossfade:
   - dry side = original per-channel scratch;
   - processed side = engaged output;
   - bypass side does not receive Output gain.
7. Write back to `buffer` at the original offset.

## 10.6 Bypass implementation

Rename ambiguous `wetMix` to `engagedMix` where practical.

Suggested helper:

```cpp
struct BypassCrossfade
{
    static float mixSample (
        float originalDry,
        float processed,
        float engagedMix) noexcept
    {
        return originalDry * (1.0f - engagedMix)
             + processed * engagedMix;
    }
};
```

The smoother remains 5 ms.

## 10.7 Runtime allocation test

Add a test-only allocation counter compiled only into the test executable.

Requirements:

- override relevant global `new/new[]` forms in one test translation unit;
- counter disabled by default;
- prewarm plugin before counting;
- enable counter only around `processBlock`;
- test:
  - normal block;
  - oversized block;
  - authentic crossfade start;
  - exact block where crossfade completes and idle engine resets;
  - bypass toggle;
  - MIDI events.

Expected allocations: `0`.

If a fully portable allocation counter cannot be made reliable, this phase may not be marked complete until an alternative test proves the same property. A static grep alone is insufficient for reset completion.

## 10.8 Static allocation scan

Extend token checks to include at least:

```text
.setSize(
.resize(
.assign(
make_unique
push_back
emplace_back
new 
malloc(
calloc(
realloc(
```

Scope scan carefully to function bodies so preparation-time allocations remain allowed.

## 10.9 Tests

- 2048 prepared-at-512 parity.
- 4096 prepared-at-128 parity.
- odd block sizes: 1, 17, 63, 127, 129, 511, 513.
- anti-phase stereo true bypass.
- bypass ignores +12 dB Output.
- bypass ignores max Input/Distn.
- bypass click metric.
- no process allocation.
- wet remains nonzero in oversized blocks.
- tail remains continuous across span boundaries.
- control quantum independent of host block size.
- default engaged dual-mono tests remain green.

## 10.10 Phase 21 acceptance

- The dry-only oversized fallback is deleted.
- `dryBuffer.setSize` is absent from `processBlock`.
- True bypass passes channel-identity tests.
- Oversized parity passes.
- Allocation gates pass.

---

# 11. Phase 22 — MIDI & Per-Sample Control Delivery

## 11.1 Goal

Make CC1 a sample-position-aware realtime pressure source and ensure fast controls are actually consumed at sample resolution.

## 11.2 Preferred implementation files

```text
source/PluginProcessor.cpp
source/PluginProcessor.h
source/PressureController.h
source/SmoothedParameterBank.h
source/GatedBloomChain.h
source/WetOverdrive.h
tests/MidiSendAmountTest.cpp
tests/SmoothedParameterBankTest.cpp
tests/GatedBloomChainTest.cpp
tests/BlockIntegrationTest.cpp
tests/ReleaseTruthTest.cpp
tests/V1InteractionContractTest.cpp
tests/V1RealtimeContractTest.cpp
```

## 11.3 MIDI cursor behavior

Only CC1 is relevant.

For each span boundary:

- apply every CC1 event whose sample position equals the current offset;
- clamp `value / 127.0f`;
- write realtime MIDI target only;
- do not touch APVTS;
- do not notify host.

Events at sample 0 affect sample 0.
Events at sample N affect sample N and later, not samples before N.

If multiple CC1 events share a sample, the last one at that sample wins.

## 11.4 Effective pressure

Per sample:

```cpp
const auto pressureGain = pressureController.processSample();
```

PressureController internally combines:

```cpp
max (hostPressureTarget, midiPressureTarget)
```

then applies attack/release smoothing and Firm/Soft curve.

## 11.5 Dynamic arrays

Add preallocated scratch arrays:

```text
distnScratch_
sendGainScratch_
thresholdDbScratch_
```

Fill every sample.

Update chain:

### Pre-reverb loop

```cpp
wet = monoIn[i];

if (gatePreSoft)
    wet *= preGate.process (envelope[i], thresholdDb[i]);

wetSendScratch[i] = wet * sendGain[i];
```

### Post-reverb loop

```cpp
wet = overdrive.process (reverbScratch[i], distnBlend[i]);

if (! gatePreSoft)
    wet *= postGate.process (envelope[i], thresholdDb[i]);

wetOut[i] = wet;
```

For the host-rate fast path, it is acceptable to keep sample processing, but it must consume the arrays.

## 11.6 RT60 and dark control rate

At each span, use the first sample’s smoothed size/dark values as span constants.

Because `kControlQuantum=128`, the maximum control hold is bounded.

Do not redesign reverb interfaces for per-sample RT60 during v1.

## 11.7 Authentic-mode simplification

Delete:

- `lastAuthenticColorSmoothed_`;
- `crossfadeEdgeHandled`;
- `authenticColorTarget` smoother;
- 0.5-crossing logic.

Add:

```cpp
bool requestedAuthenticColor_ { false };
```

Update on snapshot target edge as specified in ADR-V1-07.

## 11.8 Update MIDI tests

Replace tests that expect APVTS mutation.

Required tests:

1. CC1 changes audible pressure but not `send_amount`.
2. CC1 ignored when disconnected.
3. CC1 event at sample 256 produces no pressure feed before 256.
4. CC1 127 at sample 128 and CC1 0 at sample 384 produce a bounded send window.
5. Multiple events in one block are ordered.
6. Non-CC1 ignored.
7. MIDI pressure release follows 25 ms smoothing.
8. CC1 behavior matches across 64, 512, and 2048 host blocks.
9. `processBlock` source contains neither `setValueNotifyingHost` nor raw APVTS writes.

## 11.9 Distortion zipper test

Render a steady wet signal while changing Distn.

Compare:

- 64-sample host block;
- 512-sample host block;
- 2048-sample host block prepared at 512.

Requirements:

- trajectories align within tolerance after time alignment;
- no one-block step;
- max adjacent-sample discontinuity stays below a defined baseline threshold;
- end state reaches target.

## 11.10 Phase 22 acceptance

- APVTS state remains pure under MIDI.
- MIDI event offsets are audible and tested.
- send/distortion/threshold use sample arrays.
- block-size-invariance tests pass.
- authentic transition code is simpler and directly tested.

---

# 12. Phase 23 — Input, Level & Gate Truth

## 12.1 Goal

Make core controls agree with their labels and intended behavior, and retain the brutal post-gate character without an accidental digital tick.

## 12.2 Preferred implementation files

```text
source/ParameterCurves.h
source/ParameterSnapshot.h
source/SmoothedParameterBank.h
source/PluginEditor.cpp
source/ui/AdvancedDrawer.cpp
source/NoiseGate.h
source/InputStage.h
tests/ParameterCurvesTest.cpp
tests/ParameterSnapshotTest.cpp
tests/PluginBasics.cpp
tests/InputStageTest.cpp
tests/NoiseGateTest.cpp
tests/PostGateTimingTest.cpp
tests/RequirementsTraceabilityTest.cpp
```

## 12.3 Input mapping

Implement ADR-V1-08.

Delete `formatSignedDbFromNorm()` or make it call the canonical curve.

Correct negative-zero formatting:

```text
abs(db) < 0.005 -> "0.00"
```

Do not display `-0.00`.

## 12.4 Gate coupling truth

The detector already receives `monoScratch_` after `InputStage`, so Input affects effective gate sensitivity.

Make this explicit in code comments and tests.

`input_threshold` remains:

- ID: unchanged;
- UI label: `GATE SENS`;
- displayed value: actual threshold dB via `ParameterCurves::inputThresholdDb`;
- role: advanced offset/threshold, not replacement for Input interaction.

## 12.5 Level cleanup

Replace:

```cpp
levelEqualPower(norm, dry, wet)
```

with:

```cpp
float levelWetGain(float norm)
```

Suggested curve:

```cpp
return std::sin (halfPi * juce::jlimit (0.0f, 1.0f, norm));
```

Remove:

- `ParameterSnapshot::dryGain`;
- `SmoothedParameterBank::levelDryGain`;
- getter;
- tests describing dual-sided equal-power mixing.

Preserve:

```cpp
ParallelWetMixer::mix (dryTap, wet, wetGain)
```

## 12.6 PostHard gate implementation

Preferred implementation:

- PreSoft: existing exponential attack/release and -80 dB floor.
- PostHard:
  - 0.5 ms linear opening ramp;
  - 0.75 ms linear closing ramp;
  - exact zero at completion.

Possible state:

```cpp
float gain;
float targetGain;
float step;
int samplesRemaining;
int attackSamples;
int releaseSamples;
```

On state change, configure ramp.
Do not restart the ramp every sample while state remains unchanged.

## 12.7 Gate tests

- PreSoft closes toward floor over long release.
- PostHard does not snap.
- PostHard reaches zero by 1 ms.
- PostHard first-sample delta <=0.05 at unity.
- Hysteresis remains 3 dB.
- Post gate wet output drops below 2% of prior peak within 15 ms after silence onset.
- Long riff keeps gate open.
- Dry path is never gated.
- sample rates: 44.1, 48, 96 kHz.

## 12.8 Input tests

- exact curve anchors;
- display/DSP same function;
- low Input produces less wet clipping than high Input;
- same raw input can fail/open gate depending on Input;
- dry RMS unchanged across Input positions when wet level is zero;
- overload LED hold behavior remains 50 ms.

## 12.9 Phase 23 acceptance

- Input knob increases clockwise and reads correctly.
- no `-0.00`;
- Level has no dead dry branch;
- PostHard is intentionally hard and de-clicked;
- all dry-integrity tests pass.

---

# 13. Phase 24 — Reverb State & Wet-Dirt Integrity

## 13.1 Goal

Close latent DSP correctness holes without retuning the entire effect.

## 13.2 Preferred implementation files

```text
source/SchroederTankCore.h
source/SchroederTank32DelayTable.h
source/FixedRateAdapter.h
source/RateConverterPair.h
source/WetOverdrive.h
tests/SchroederTankCoreTest.cpp
tests/SchroederTank32Test.cpp
tests/FixedRateAdapterTest.cpp
tests/WetOverdriveTest.cpp
tests/WetOverdriveDiagnosticsTest.cpp
tests/HighFrequencyRingingDiagnosticsTest.cpp
tests/ReleaseTruthTest.cpp
```

## 13.3 Predelay implementation

In `prepare()`:

```cpp
const auto darkDelaySamples =
    kDarkPredelaySeconds * processingRate_;

predelayLine.setDelay (darkDelaySamples);
```

In every `processTank()` call:

```cpp
predelayLine.pushSample (0, input);
const auto delayed = predelayLine.popSample (0);
const auto x = input + darkMix * (delayed - input);
```

Do not conditionally clock.

## 13.4 Predelay tests

- Bright impulse has immediate tank excitation.
- Dark impulse begins near 55 ms later within tolerance.
- Toggle Dark off, process more than 55 ms of silence, toggle on: no stale burst.
- Toggle Dark during continuous tone: finite output and bounded adjacent delta.
- Host-rate and fixed-rate dark predelay agree in wall-clock time.

## 13.5 Modulation depth

Add helper:

```cpp
static float tankLfoDepthSamplesForRate (double rate) noexcept;
```

Test:

```text
depthSamples/rate == kTankLfoDepthSeconds
```

at 32,768; 44,100; 48,000; 88,200; 96,000 Hz.

Do not change `kTankLfoHz`.

## 13.6 ProperSRC underfill

`FixedRateAdapter::processBlock`:

```cpp
std::fill (out, out + n, 0.0f);
const int written = converters.downsample (...);
jassert (written >= 0 && written <= n);
```

Add a sentinel test:

1. Fill output with a nonzero sentinel.
2. Process initial/priming ProperSRC block.
3. Assert no sentinel remains.
4. Assert unwritten region is zero.
5. Assert later blocks become nonzero and finite.

## 13.7 Wet overdrive filters

Implement simple allocation-free one-pole high-pass/DC blocker.

Suggested class:

```cpp
class OnePoleHighpass
{
public:
    void prepare (double sampleRate, float cutoffHz) noexcept;
    void reset() noexcept;
    float process (float x) noexcept;

private:
    float alpha {};
    float prevInput {};
    float prevOutput {};
};
```

Chain:

```cpp
auto x = preClipHp.process (wet);
x = preClipLp.process (x);
x = clipSample (x, activeCurve);
x = postClipLp.process (x);
x = postClipDcBlock.process (x);
```

Blend against original wet as before.

## 13.8 Dirt tests

- 20 Hz/low-frequency attenuation matches intended high-pass behavior.
- 1 kHz musical band remains within reasonable gain tolerance.
- DC input decays toward zero.
- symmetric sine long-run mean absolute DC `<1e-4`.
- asymmetric harmonic character remains measurably different from symmetric reference.
- Distn=0 returns original wet within tolerance.
- dry path unchanged.
- existing harshness/ringing diagnostics remain green.

## 13.9 Phase 24 acceptance

- no stale predelay;
- modulation time invariant;
- no ProperSRC sentinel/undefined remainder;
- wet dirt DC controlled;
- no ProperSRC/HF regression;
- no broad reverb retune.

---

# 14. Phase 25 — Presets, UI, Branding & Release Truth

## 14.1 Goal

Make the shipping surface match the product’s actual state and the repository’s own clean-room claims.

## 14.2 Preferred implementation files

```text
resources/ui/
source/ui/PedalFaceplatePaint.cpp
source/ui/PedalFaceplatePaint.h
source/ui/AdvancedDrawer.cpp
source/ui/AdvancedDrawer.h
source/PluginEditor.cpp
CMakeLists.txt
scripts/check-legal-metadata.sh
docs/CLEAN_ROOM.md
docs/RELEASE_CHECKLIST.md
docs/DAW_SMOKE_RC0.md
README.md
design-qa.md
tests/ReleaseTruthTest.cpp
tests/PluginEditorTest.cpp
tests/PresetTest.cpp
tests/V1ReleaseContractTest.cpp
```

## 14.3 Shipping asset policy

Before public v1, choose one:

### Path A — Niko-approved original image asset

- filename: `resources/ui/sendbloom-faceplate.png`;
- visible product name: `SendBloom`;
- original Niko Audio Labs graphic design;
- no copied logo, product name, exact faceplate artwork, or third-party controller name;
- update CMake BinaryData symbol references;
- update screenshot QA.

### Path B — Procedural original faceplate

- remove external reference asset from BinaryData;
- use procedural drawing as production;
- change title to `SENDBLOOM`;
- remove reference-derived exact copy elements as needed;
- preserve control usability.

Path A is preferred when an approved asset exists. Path B is the safe fallback.

Do not ship the current `reverbx-faceplate.png` merely because string scanning cannot inspect its pixels.

## 14.4 Legal scanner strengthening

Rewrite scanner around normalized matching.

For textual content:

```text
lowercase
remove spaces, hyphens, underscores, and punctuation
search normalized banned tokens
```

For filenames:

```text
find source resources tests README CMake docs/release surfaces
normalize full relative path
search banned tokens
```

Keep internal research/milestone docs out of the product-facing scan, or explicitly maintain a separate allowlist for cited research.

Required product-facing banned concepts include the referenced:

- company name;
- product name;
- proprietary controller name.

Do not place the literal terms in source comments merely to test the scanner; keep test fixtures isolated.

## 14.5 UI truth

Advanced drawer must show:

- Gate Sens;
- Send Feel;
- Pressure Mode;
- 32k Color;
- disabled Extended Stereo;
- disabled Dirt OS.

Tooltips:

### Pressure Mode

```text
Off: reverb input is always active.
On: the signal stays dry until the pressure pad, automation, or MIDI CC1 sends audio into the reverb. Existing tails continue after release.
```

### 32k Color

Update stale warning copy. ProperSRC has been validated against current automated gates; it remains optional/off by default, not “may exhibit HF artifacts” unless current evidence still supports that warning.

Do not claim firmware authenticity.

## 14.6 Preset review

For every preset:

- load in plugin;
- verify connection mode;
- verify resting pressure;
- verify Input mapping after curve correction;
- compensate preset Input normalized values only if listening shows an unintended level shift;
- record before/after screenshots and short rendered metrics;
- keep `authentic_color=0`.

Because Input mapping direction changes, all preset Input values require human listening review. Automated agents may not blindly mirror normalized values and declare sonic equivalence.

## 14.7 State compatibility policy

This is pre-v1 software. The v1 contract begins at v1.0.

Document:

- parameter IDs are preserved;
- old development sessions load;
- old `send_amount` values may sound different because resting semantics are corrected;
- no hidden state-version migration is promised;
- factory presets are explicitly migrated.

Do not add a complex state wrapper unless a real released-session requirement is discovered.

## 14.8 Design QA

Update `design-qa.md`:

- no absolute local machine paths;
- repository-relative asset path;
- generated screenshot paths;
- default state;
- Pressure Mode off/on;
- pressure pressed/released;
- gate pre/post;
- dark on/off;
- bypass state;
- advanced drawer;
- clip LED;
- original-branding sign-off;
- test/build evidence.

## 14.9 Phase 25 acceptance

- no banned product-facing strings or filenames;
- original faceplate path selected;
- legal script catches spacing/hyphen variants and filenames;
- UI state is truthful;
- all presets reviewed;
- docs match current implementation;
- Niko approves the shipping faceplate (`human_needed`).

---

# 15. Phase 26 — Reference Capture & Sonic Classification

## 15.1 Goal

Separate “well-built original inspired effect” from “hardware-compared emulation” using reproducible evidence.

This phase does not use proprietary firmware, dumps, schematics, or reverse engineering.

## 15.2 Files

Create:

```text
docs/reference/REFERENCE_CAPTURE_PROTOCOL.md
docs/reference/REFERENCE_ANALYSIS.md
docs/reference/CLAIM_STATUS.md
tools/reference/analyze_reference.py
tools/reference/render_plugin_probe.cpp   # or equivalent host/test renderer
reference/README.md
reference/metrics/.gitkeep
```

Raw recordings may be gitignored if large. Commit derived metrics and small permitted test assets.

## 15.3 Capture chain metadata

Every capture set records:

- hardware serial/revision if known;
- date;
- interface;
- sample rate/bit depth;
- reamp device;
- input/output calibration;
- power supply;
- cable routing;
- knob positions;
- switch states;
- controller orientation/pressure method;
- dry baseline capture;
- active Level=0 baseline;
- latency alignment method;
- file hashes.

## 15.4 Test signals

Generate and retain deterministic source files:

1. single-sample/short bandlimited impulse;
2. exponential sine sweep;
3. pink-noise bursts;
4. stepped sine amplitudes;
5. guitar-like pluck;
6. palm-mute burst;
7. sustained chord;
8. riff followed by exact silence;
9. repeated controller send windows.

## 15.5 Measurement grid

### Reverb

Size positions:

```text
0%, 25%, 50%, 75%, 100%
```

Modes:

```text
Bright
Dark
```

Measure:

- predelay;
- EDT;
- RT20/RT30 estimate;
- spectral centroid over time;
- high-frequency decay;
- modulation peak/rate;
- tail density;
- maximum output.

### Distortion

Input:

```text
20%, 50%, 80%, 100%
```

Distn:

```text
0%, 25%, 50%, 75%, 100%
```

Measure:

- harmonic ratios;
- transfer curve;
- small-signal gain;
- DC offset;
- spectral tilt;
- clipping onset.

### Gate

- Pre and Post;
- low, medium, high Input;
- deterministic burst then silence.

Measure:

- opening threshold relative to source;
- close decision time;
- close envelope duration;
- chatter/hysteresis behavior.

### Pressure

- disconnected;
- connected/rest;
- 25%, 50%, 75%, 100% pressure if repeatable;
- release with tail.

Measure:

- feed-gain curve;
- attack;
- release;
- tail preservation;
- Firm/Soft difference.

## 15.6 Wet isolation procedure

Because dry remains present:

1. Capture true-bypass baseline.
2. Capture active path at Level=0.
3. Capture active effect setting.
4. Align active Level=0 baseline to active effect capture.
5. Match dry level/phase.
6. Subtract active dry baseline to estimate wet-only response.
7. Record residual/error and reject measurements where subtraction is unstable.

Do not assume true-bypass capture is phase-identical to active dry path.

## 15.7 Analysis outputs

`analyze_reference.py` should emit JSON/CSV:

```json
{
  "capture_id": "...",
  "sample_rate": 48000,
  "settings": {},
  "predelay_ms": 0.0,
  "rt60_s": 0.0,
  "spectral_centroid_hz": [],
  "harmonics_db": {},
  "dc_offset": 0.0,
  "gate_close_ms": 0.0
}
```

The script must be deterministic and include unit tests for synthetic known signals.

## 15.8 Calibration rules

- Do not tune more than one subsystem at a time.
- Preserve a before render and metrics.
- Every constant change requires:
  - hardware metric improvement;
  - no regression in safety gates;
  - Niko listening approval.
- Do not overfit one setting.
- Keep 32k Color off by default unless broad evidence supports changing product policy; changing that policy is outside this milestone.

## 15.9 No-hardware outcome

If hardware is unavailable:

- complete protocol/tooling;
- mark capture tasks `human_needed`;
- set `CLAIM_STATUS.md` to `original-inspired`;
- public wording remains generic;
- do not block a correctly positioned v1 release solely because hardware was unavailable.

## 15.10 Phase 26 acceptance

One of:

### Outcome A

- hardware grid captured;
- metrics generated;
- Niko listening complete;
- claim status assigned.

### Outcome B

- tooling/protocol complete;
- hardware unavailable documented;
- status explicitly `original-inspired`;
- no fidelity claim.

No fake “passed” status.

---

# 16. Phase 27 — RC Verification, Licensing & Distribution

## 16.1 Goal

Produce a real, reproducible v1.0.0-rc0 candidate and close every release gate with evidence.

## 16.2 Versioning

Set:

```text
VERSION = 1.0.0
```

Tag later:

```bash
git tag -a v1.0.0-rc0 -m "SendBloom v1.0.0-rc0"
```

Do not tag before all required RC gates pass.

## 16.3 Clean build

From a clean directory:

```bash
rm -rf Builds-v1
cmake -S . -B Builds-v1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds-v1 --config Release
ctest --test-dir Builds-v1 -C Release --output-on-failure
bash scripts/verify-v1.sh
```

Record output in:

```text
docs/reports/v1.0-rc0-automated-verification.md
```

## 16.4 VST3 pluginval

Use strictness 10:

```bash
"$PLUGINVAL_BIN" \
  --strictness-level 10 \
  --verbose \
  --validate "Builds-v1/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
```

Store logs.

## 16.5 AU discovery and validation

Because the plugin accepts MIDI, do not assume its AU type.

First:

```bash
auval -a | grep -E 'SbLm|NkMo|SendBloom'
```

Use the actual listed type:

```bash
auval -v <actual-type> SbLm NkMo
```

The type may be `aumf` rather than `aufx`. The discovered type is source of truth.

Install/copy the component as required for local discovery, then validate.

Also run pluginval against the AU where supported.

## 16.6 CI

Require green latest commit on:

- macOS;
- Windows;
- Linux.

Evidence must include workflow URL/run ID in release report.

No “not locally verified” checkbox may remain without CI evidence.

## 16.7 DAW smoke

Update `docs/DAW_SMOKE_RC0.md` to include the fixed contracts.

### Logic Pro / AU

Verify:

- instantiation;
- editor;
- all presets;
- always-on mode;
- Pressure Mode dry-at-rest;
- pad press;
- pad release with tail;
- MIDI CC1;
- Pre/Post gate;
- Dark toggle;
- 32k Color transition;
- stereo true bypass;
- automation of Input, Level, Distn, Output;
- 10-minute abuse.

### Cubase / VST3

Same, plus:

- host automation read/write;
- MIDI routing;
- unusual block-size/offline render parity if configurable.

### REAPER / VST3

Same, plus:

- variable block-size behavior;
- offline render;
- mono and stereo tracks.

Sign-off table:

| Host | Version | OS | Format | Tester | Date | Result | Notes |
|---|---|---|---|---|---|---|---|
| Logic Pro | | | AU | Niko | | | |
| Cubase | | | VST3 | Niko | | | |
| REAPER | | | VST3 | Niko | | | |

All three are `human_needed`.

## 16.8 Licensing decision

Create:

```text
docs/LICENSING_DECISION.md
```

Niko must select and document one path:

### Commercial JUCE path

- valid commercial JUCE license covers distribution;
- own source may retain chosen license;
- third-party notices remain complete.

### GPL path

- distribution and repository licensing satisfy JUCE GPL obligations;
- current standalone MIT presentation is corrected as necessary.

The coding agent must not choose on Niko’s behalf.

## 16.9 Signing/notarization

For public macOS binaries:

- Developer ID sign plugin bundles;
- use hardened runtime as required;
- package for notarization;
- submit with `xcrun notarytool`;
- wait for Accepted;
- staple ticket to distributable container where applicable;
- validate signatures and staple;
- test on a clean/non-development Mac user account when possible.

Credentials come from environment/keychain. Never commit them.

Create scripts with placeholders/environment variables, not embedded secrets:

```text
scripts/package-macos.sh
scripts/sign-macos.sh
scripts/notarize-macos.sh
```

## 16.10 Release artifacts

Produce:

```text
SendBloom-1.0.0-rc0-macOS-universal.*
SendBloom-1.0.0-rc0-Windows-x64.*
SendBloom-1.0.0-rc0-Linux-x64.*
SHA256SUMS
THIRD_PARTY_LICENSES.txt
RELEASE_NOTES.md
```

Only produce platforms actually supported and verified.

## 16.11 Final release report

Create:

```text
docs/reports/v1.0.0-rc0-release-evidence.md
```

Include:

- commit and tag;
- artifact hashes;
- configure/build/test commands;
- test results;
- CI runs;
- pluginval logs;
- auval output;
- DAW smoke;
- branding approval;
- claim status;
- licensing decision;
- signing/notarization IDs/status;
- remaining known limitations;
- rollback instructions.

## 16.12 Phase 27 acceptance

RC0 may be tagged only when:

- all automated requirements green;
- all required human gates signed;
- licensing resolved;
- product-facing branding approved;
- macOS distribution signed/notarized if distributed publicly;
- repository clean;
- release report complete.

---

# 17. Exact test plan

## 17.1 Required new/updated test cases

Use names close to these so failures are searchable.

### Pressure

```text
Pressure mode disconnected feeds wet at unity
Pressure mode connected rests fully dry
Pressure press feeds wet path
Pressure release blocks new wet feed and preserves tail
Pressure pad mouseUp keeps controller connected
Pressure pad mouseUp returns amount to zero
Pressure attack is approximately 3 ms
Pressure release is approximately 25 ms
Pressure behavior invariant across block sizes
```

### MIDI

```text
MIDI CC1 modulates pressure without changing APVTS state
MIDI CC1 ignored when pressure mode disconnected
MIDI CC1 honors event sample position
Multiple MIDI CC1 events in one block are ordered
MIDI CC1 zero releases pressure
Non-CC1 messages do not alter pressure
```

### Blocks/realtime

```text
Oversized host block matches equivalent prepared-size blocks
Odd host blocks remain finite and wet
processBlock performs zero heap allocations
Engine crossfade completion performs zero heap allocations
Control spans never exceed 128 samples
```

### Input/level

```text
Input gain canonical anchors are -9 0 +9 dB
Input UI formatter equals DSP mapping
Increasing Input increases wet drive
Input changes gate trigger relationship
Level zero preserves unity dry and removes wet
Level control has no dry-gain state
```

### Gate

```text
PostHard close does not snap in one sample
PostHard reaches zero within one millisecond
PostHard step delta is bounded
Post gate chops within fifteen milliseconds
PreSoft release remains long
```

### Bypass

```text
Settled bypass preserves left and right independently
Settled bypass ignores output gain
Settled bypass ignores wet controls
Bypass transition remains click bounded
```

### Reverb/dirt

```text
Dark delay line is continuously clocked
Dark re-enable emits no stale burst
LFO depth is time invariant across sample rates
ProperSRC underfill clears sentinel output
Wet overdrive pre highpass removes DC and sub bass
Wet overdrive post DC blocker converges to zero
Wet overdrive Distn zero is identity
```

### Branding/release

```text
Product-facing normalized legal scan catches spaced and hyphenated variants
Product-facing legal scan checks filenames
Shipping BinaryData contains original SendBloom faceplate only
VERSION is numeric semantic version
CMake project version accepts VERSION file
Release docs contain no stale fixed test count
```

## 17.2 Numerical gates

| Gate | Threshold |
|---|---:|
| Oversized block max difference | `< 2e-5` |
| Settled bypass per-channel max difference | `< 1e-6` |
| PostHard per-sample gain delta | `<= 0.05` |
| PostHard close completion | `<= 1.0 ms` |
| Post wet chop | `< 2%` of pre-silence peak within `15 ms` |
| Wet dirt DC mean | `< 1e-4` after settling |
| ProperSRC sentinel remainder | exactly zero |
| Realtime allocations | `0` |
| Non-finite samples | `0` |
| Control span | `<=128` |
| Reported latency | `0 samples` |
| Pressure attack target | `3 ms`, tolerance ±25% |
| Pressure release target | `25 ms`, tolerance ±20% |

Adjust a numerical threshold only with documented evidence that the test method, not the implementation, is wrong.

---

# 18. File ownership map

| File | Phase owner | Required change |
|---|---:|---|
| `source/PluginProcessor.cpp` | 21/22 | span loop, no resizing, MIDI cursor, true bypass, direct authentic request |
| `source/PluginProcessor.h` | 21/22 | scratch arrays, pressure state, span helper |
| `source/GatedBloomChain.h` | 22 | per-sample dynamic arrays |
| `source/PressureController.h` | 20/22 | new pressure-source/smoothing owner |
| `source/PressureSend.h` | 20 | keep simple multiplication or retire behind controller |
| `source/ui/PressureSendPad.*` | 20 | release-to-zero, persistent connection, gestures |
| `source/ui/AdvancedDrawer.*` | 20/25 | Pressure Mode and truthful copy |
| `source/ParameterLayout.cpp` | 20 | send default zero |
| `source/ParameterSnapshot.h` | 20/23 | state truth, remove dry gain |
| `source/SmoothedParameterBank.h` | 21–23 | remove dead/authentic smoothing, dynamic controls |
| `source/ParameterCurves.h` | 20/23 | Input, level, pressure curves |
| `source/NoiseGate.h` | 23 | hard ramp |
| `source/SchroederTankCore.h` | 24 | continuous fixed predelay and LFO time depth |
| `source/SchroederTank32DelayTable.h` | 24 | seconds-based mod constant |
| `source/FixedRateAdapter.h` | 24 | output clear/underfill |
| `source/WetOverdrive.h` | 24 | HP/DC blocker |
| `source/ui/PedalFaceplatePaint.*` | 20/25 | pressure overlay and original branding |
| `resources/presets/*.xml` | 20/25 | send resting state and review |
| `source/FactoryPresets.cpp` | 20/25 | exact XML parity |
| `CMakeLists.txt` | 25/27 | original asset, numeric version path |
| `scripts/check-legal-metadata.sh` | 25 | normalized text/path scan |
| `scripts/verify-v1.sh` | 19/27 | durable gate |
| `docs/*` | 25–27 | truthful product/release docs |
| `VERSION` | 27 | `1.0.0` |

Avoid two phases editing the same file concurrently. Execute sequentially or use explicit branch ownership.

---

# 19. Commit plan

Recommended commits, one coherent behavior per commit:

1. `test(v1): lock failing interaction and realtime contracts`
2. `fix(send): make pressure mode dry at rest with trails`
3. `fix(processor): replace oversized fallback with realtime spans`
4. `fix(bypass): preserve channel identity and unity`
5. `fix(midi): add sample-position-aware realtime pressure`
6. `fix(automation): consume send distortion and threshold per sample`
7. `fix(input): unify clockwise -9 to +9 dB mapping`
8. `fix(gate): de-click PostHard with sub-ms ramp`
9. `refactor(level): remove dead dry-gain path`
10. `fix(reverb): continuously clock fixed dark predelay`
11. `fix(reverb): make modulation depth time invariant`
12. `fix(src): clear ProperSRC underfill`
13. `fix(dirt): add pre HP and post DC blocker`
14. `chore(presets): migrate pressure resting states`
15. `chore(ui): ship original SendBloom branding`
16. `test(release): strengthen legal and runtime gates`
17. `docs(reference): add capture and claim protocol`
18. `chore(release): prepare 1.0.0-rc0 evidence`

Do not create one giant commit.

---

# 20. Cheap-agent trap list

A plan or code review must reject any implementation that does any of the following:

1. Calls `setValueNotifyingHost()` from `processBlock`.
2. Keeps raw `sendParam->store()` and merely changes the test.
3. Sets `send_connected=false` on pad release.
4. Clears the reverb tank on pressure release.
5. Makes controller-connected resting pressure default to 1.
6. Solves oversized blocks by allocating a larger buffer in `processBlock`.
7. Solves oversized blocks by truncating or returning dry.
8. Ignores MIDI sample positions.
9. Collects MIDI events into a growing vector on the audio thread.
10. Continues advancing smoothers per sample but using only sample zero.
11. Changes immutable parameter IDs.
12. Applies Output gain to settled bypass.
13. Preserves mono collapse during settled bypass.
14. Keeps the PostHard one-sample snap and changes `releaseMs` comments only.
15. Changes Input UI without changing DSP, or vice versa.
16. Uses a variable predelay length `darkMix * 55ms` instead of fixed-tap blending.
17. Stops clocking predelay in bright mode.
18. Changes the ProperSRC quality preset without evidence.
19. Enables 32k Color by default.
20. Implements Extended Stereo or Dirt OS “while already here.”
21. Removes failing tests to regain green.
22. Hard-codes “135/135” or any fixed test count into release truth.
23. Puts `1.0.0-rc0` into CMake numeric project version.
24. Assumes AU type `aufx` without checking `auval -a`.
25. Ships the reference faceplate because source strings were renamed.
26. Claims exact emulation based only on internal unit tests.
27. Treats missing human listening as pass.
28. Commits signing/notarization credentials.
29. Performs broad DSP retuning before Phase 26 measurement.
30. Rewrites the architecture instead of fixing the specified contracts.

---

# 21. Review checklist per phase

Every phase code review must answer:

## Correctness

- Does behavior match the owning requirement IDs?
- Is there a test that would fail if the old bug returned?
- Are edge cases tested at 44.1/48/96 kHz?
- Are odd and oversized blocks tested?

## Realtime

- Any allocation?
- Any lock?
- Any host notification?
- Any I/O?
- Any unbounded loop based on external input?
- Any vector growth?
- Any GUI access?

## State

- Parameter IDs unchanged?
- Factory XML/program parity?
- Defaults truthful?
- Old development-state behavior documented?

## Audio

- Dry path unchanged?
- Tail preserved?
- No NaN/Inf?
- No one-sample discontinuity?
- No stale delay?
- No unexpected DC?
- No block-size sonic change?

## Claims/release

- Does documentation match actual implementation?
- Is human evidence distinguished from automated evidence?
- Is third-party naming absent from shipping surfaces?
- Is the current fidelity status explicit?

---

# 22. Milestone-wide verification command

At final automated closeout:

```bash
set -euo pipefail

rm -rf Builds-v1
cmake -S . -B Builds-v1 -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds-v1 --config Release
ctest --test-dir Builds-v1 -C Release --output-on-failure
bash scripts/check-legal-metadata.sh
BUILD_DIR="$PWD/Builds-v1" bash scripts/enab-acceptance-gates.sh
BUILD_DIR="$PWD/Builds-v1" bash scripts/verify-v1.sh
```

Then plugin validation and human gates.

---

# 23. Definition of done

The milestone is done only when all are true:

## Interaction

- Pressure Mode off is always-on reverb.
- Pressure Mode on is dry at rest.
- Press sends.
- Release returns to dry and preserves trails.
- UI, automation, and CC1 agree.

## Realtime

- No process allocation.
- Oversized blocks retain wet effect.
- MIDI positions respected.
- block-size parity passes.
- crossfades are finite and converge.

## Core sound

- Input is correctly mapped and displayed.
- Level scales wet only.
- Post gate is brutal but de-clicked.
- dry path stays clean.
- bypass is true channel-preserving unity.

## DSP integrity

- predelay cannot freeze stale material;
- modulation depth is time invariant;
- ProperSRC remainder deterministic;
- dirt DC controlled;
- ProperSRC/HF suite green.

## Product/release truth

- original SendBloom branding ships;
- clean-room scan is meaningful;
- presets migrated/reviewed;
- docs current;
- fidelity status honest;
- numeric version correct;
- CI and validation green;
- DAW smoke signed;
- JUCE licensing resolved;
- public macOS package signed/notarized;
- hashes and release report complete.

---

# 24. Final GSD closeout instruction

After Phase 27:

1. Run `/gsd-audit-milestone`.
2. Resolve every orphaned requirement.
3. Confirm every phase has Summary and Verification.
4. Confirm human-needed gates contain real evidence or remain explicitly open.
5. Run `/gsd-complete-milestone` only when the RC tag exists and the release report points to the exact tag.
6. If any human gate remains open, stop at `RC blocked — human_needed`; do not archive as complete.

---

# Appendix A — Current-to-target signal flow

## Current conceptual flow

```text
stereo input
  -> save dry copy
  -> mono sum
  -> wet InputStage
  -> gate/send/reverb/dirt/gate
  -> dry + wet
  -> mono/dual-mono output
  -> output gain
  -> pseudo-bypass inside mono mix
```

## Target v1 flow

```text
original stereo input
  |
  +----------------------------------------------+
  |                                              |
  |                                      true-bypass dry side
  |
  -> per-span dry copy
  -> mono sum for engaged mono-first path
  -> wet InputStage
  -> detector
  -> optional PreSoft gate (per-sample threshold)
  -> PressureController gain (per sample)
  -> reverb engine (span RT60/dark, requested authentic mode)
  -> wet dirt (per-sample blend)
  -> optional PostHard gate (per-sample threshold)
  -> unity mono dry + wet level
  -> engaged output gain
  -> dual mono engaged output
  |
  +-> final per-channel 5 ms bypass crossfade against original stereo input
```

---

# Appendix B — Factory preset target table

| Preset | Input | Gate Sens | Size | Level | Distn | Out | Dark | Gate | Pressure Mode | Rest Pressure | Feel | 32k |
|---|---:|---:|---:|---:|---:|---:|---:|---|---|---:|---|---:|
| Sparkle Verb | review | current | current | current | current | current | off | Post | off | 0 | Firm | off |
| Cut Sample Gate | review | current | current | current | current | current | off | Post | off | 0 | Firm | off |
| Spacerock Burn | review | current | current | current | current | current | on | Post | off | 0 | Firm | off |
| Dry Dub Sends | review | current | current | current | current | current | off | Pre | on | 0 | Firm | off |
| Dark Bloom | review | current | current | current | current | current | on | Pre | off | 0 | Soft | off |
| Firm Pressure | review | current | current | current | current | current | off | Pre | on | 0 | Firm | off |
| Gated Room | review | current | current | current | current | current | off | Post | off | 0 | Soft | off |
| Hot Clip | review | current | current | current | current | current | off | Post | on | 0 | Firm | off |

“Review” means the normalized value must be auditioned after the Input curve direction correction. Do not automatically invert it without listening.

---

# Appendix C — Evidence classification

| Evidence | Can prove |
|---|---|
| Unit test | local code contract |
| Integration render | cross-module behavior |
| pluginval/auval | host/API compatibility |
| CI | platform build/test repeatability |
| DAW smoke | practical host behavior |
| Objective hardware capture | measured similarity at tested settings |
| Human level-matched listening | subjective product approval |
| Clean-room audit | absence of prohibited implementation inputs in audited scope |
| None of the above alone | legal freedom to operate or universal hardware fidelity |

---

# Appendix D — Known product decisions that remain unchanged

- Product name: `SendBloom`.
- Publisher: `Niko Audio Labs`.
- AU/VST3 formats.
- Manufacturer code: `NkMo`.
- Plugin code: `SbLm`.
- Bundle ID: `com.nikoaudiolabs.sendbloom`.
- Host-rate reverb production default.
- 32k Color optional and off by default.
- Zero reported latency under ADR-003 Path B.
- Wet-only distortion.
- Mono-first engaged processing.
- Deferred Extended Stereo and Dirt OS.
- Eight factory presets.
- Main release target: v1.0.0-rc0, then v1.0.0 after RC validation.

---

# Appendix E — Source snapshot references

The repository findings in this specification were derived from the uploaded 2026-07-12 Repomix snapshot, especially:

```text
source/PluginProcessor.cpp
source/PluginProcessor.h
source/GatedBloomChain.h
source/NoiseGate.h
source/ParameterCurves.h
source/ParameterLayout.cpp
source/ParameterSnapshot.h
source/SmoothedParameterBank.h
source/ui/PressureSendPad.cpp
source/ui/AdvancedDrawer.cpp
source/ui/PedalFaceplatePaint.cpp
source/SchroederTankCore.h
source/FixedRateAdapter.h
source/RateConverterPair.h
source/SchroederTank32.h
source/EngineCrossfade.h
source/WetOverdrive.h
tests/MidiSendAmountTest.cpp
tests/NoiseGateTest.cpp
tests/PostGateTimingTest.cpp
tests/LatencyTest.cpp
tests/IntegrationAllocScanTest.cpp
tests/ReleaseTruthTest.cpp
tests/PluginBasics.cpp
CMakeLists.txt
README.md
docs/CLEAN_ROOM.md
docs/RELEASE_CHECKLIST.md
docs/DAW_SMOKE_RC0.md
design-qa.md
VERSION
```

If the live repository differs from the snapshot, the agent must record the delta in Phase 19 and preserve the intent of this milestone rather than blindly applying stale line edits.
