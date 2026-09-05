# Further fidelity work: wet distortion reconstruction

This pass follows the user's challenge that the first pressure/gate correction
was not a complete investigation. It adds an implemented numerical improvement,
not another speculative reverb/drive voicing. Both changes are now in the working
source. The first pass's preserved binaries and results remain historical.

## Evidence and scope

The target remains the five-knob **Rainger FX Reverb-X with Igor**. No PCB or
firmware revision has been established. The [manufacturer's product page](https://www.raingerfx.com/shop/p/reverb-x)
describes a fixed overdrive circuit after reverb, with a blend control. Its
[Pull Focus description](https://www.raingerfx.com/shop/p/pull-focus) explicitly
calls that related pedal's distortion analog and identifies its initial overdrive
circuit as derived from Reverb-X/El Distorto, followed by a separate Tonebender
section. This corroborates an analog overdrive class; it does **not** establish
Reverb-X component values, gain, clipping law, or the additional Tonebender stage.
Both pages were retrieved again during this follow-up on 2026-09-05.

The existing discrete waveshaper folds high harmonics into nonharmonic audible
frequencies. An analog circuit has no host-rate Nyquist boundary at which to fold
those harmonics. Removing this numerical artifact is justified without claiming
that the retained transfer curve or tone filters match the hardware. Oversampling
also reduces the host-rate dependence of the existing one-pole filters. There is
no hardware accuracy percentage, spectral fitting to video, or new demo audition.

Technical literature investigated: Parker/Zavalishin/Le Bivic,
[Reducing the Aliasing of Nonlinear Waveshaping Using Continuous-Time Convolution](https://www.dafx.de/paper-archive/2016/dafxpapers/20-DAFx-16_paper_41-PN.pdf),
and Bilbao et al., *Antiderivative Antialiasing for Memoryless Nonlinearities*.
Direct PDF fetches were unreliable; the implementation relies on JUCE's locally
available, inspected oversampling implementation. First-order ADAA was considered
but not implemented: its uncompensated frequency response and fractional timing
would add a different tradeoff. No results are claimed for an ADAA experiment.

## What changed

`WetOverdriveState` processes the existing dirty filters and nonlinear function
at **4× host rate**, using JUCE's two-stage, high-quality polyphase IIR oversampler.
The clean wet branch traverses the same interpolation/reconstruction filters so
DISTN blends two equally reconstructed branches. Both branches run continuously,
including at blend endpoints. Buffers are allocated during prepare; the production
chain uses the block API, including its variable MIDI spans. Post gating remains
after reconstruction, so it can still close to exact zero.

The static reciprocal curve, drive 3, positive asymmetry 1.1, makeup 1.2, filter
cutoff settings, input gain, tank, size/dark maps, gates and pressure curves were
not retuned. Incorrect comments claiming the asymmetric curve adds no harmonics
below threshold were corrected: unequal slopes at zero create even harmonics.

**Compatibility / preset sound:** parameter IDs, values, automation, program
selection and state formats are unchanged. All presets with a wet return acquire
the reconstruction phase response; dirty presets also have reduced aliases and
slightly darker upper treble. These are shared DSP changes, not preset edits.
Direct and both bypass routes gain **six samples** of delay, included in host PDC.
At 48 kHz the measured report changes **186 → 192 samples**, an extra **0.125 ms**.
An earlier progress message incorrectly said five; six is the verified value.
IIR phase is frequency dependent, so the wet path is not a pure six-sample shift.
The first-pass claim that non-pressure outputs were bit-identical does not apply
to this second change.

## Quantitative result

Acceptance criteria were frozen in `artifacts/fidelity-followup-20260905/EXPERIMENT.md`
before implementation. Both baseline and candidate use production DSP. The dirt
probe spans 44.1/48/96/192 kHz, 997/4001/7001/10003 Hz, and peaks .03/.3/.8/1.5
(64 cells/build). Each integer-frequency sine runs 1.25 s, with .25 s settling;
the coherent one-second FFT separates representable harmonics from other bins.
The residual includes numerical noise; at deep rejection it is not pure aliasing.

At 48 kHz, peak input .8 to the wet dirt stage:

| Probe | Before nonharmonic energy | After | Reduction | Fundamental change |
|---|---:|---:|---:|---:|
| 997 Hz | -70.53 dBc | -97.77 dBc | 27.24 dB | +0.017 dB |
| 4001 Hz | -42.73 dBc | -58.10 dBc | 15.37 dB | -0.101 dB |
| 7001 Hz | -36.84 dBc | -71.77 dBc | **34.93 dB** | -0.395 dB |
| 10003 Hz | -28.82 dBc | -66.24 dBc | **37.41 dB** | -0.925 dB |

The frozen >=15 dB rejection improvement at 7/10 kHz and low/mid fundamental
limits pass. These are stress probes, not estimates of the audible improvement
on guitar. At .03 peak / 10003 Hz, fundamental gain spread across the four host
rates falls **1.285 → 0.073 dB**. At 48 kHz that quiet high-frequency fundamental
falls 1.076 dB. The exact pedal's treble response is still unmeasured.

The full processor was rendered twice per cell over the existing **60-cell DI
and synthetic corpus** for each build. Repeated renders are byte-identical within
each build. Settings, input hashes, binary hashes and receipts are retained.
After individual PDC alignment, clean-wet RMS changes by less than 0.000001 dB;
Dark/long/fully-dirty wet RMS changes from -0.108 to +0.051 dB across the ten inputs.
This demonstrates gain preservation in these fixtures, not perceptual identity.
Output peaks can exceed 0 dBFS in both versions (maximum +2.64 / +2.63 dBFS in
this corpus). Absolute float WAVs preserve those existing gains without clipping;
matched PCM24 examples have at least 1 dB headroom.

## Verification and review

- Release arm64 builds: processor renderer, tank/dirt probe, benchmark, AU, VST3.
- **285 CTests passed; one existing ZIP capability test skipped; zero failed.**
  All eight initial failures were old zero-delay or SRC-only latency assertions.
  Replacement tests check clean passband magnitude over four rates, reconstructed
  blend identity under modulation, direct/bypass PDC, block partition/reset
  invariance, and quantitative alias rejection. Their original red log remains.
- C++ allocation guards, dense MIDI, oversized callbacks, finite-output stress,
  pressure release, preset/state and existing host-contract tests pass. Allocation
  guards cover C++ allocation calls, not every possible direct malloc in libraries;
  source inspection confirms oversampling buffers are prepared outside callbacks.
- 16 Python reference tests passed. All 40 generated preview WAVs are hashed;
  matched pair RMS differences are below 0.000001 dB. Full raw renders are retained.
- pluginval strictness 10 passed on the local VST3; strict code-signature checks
  passed on both local AU/VST3 bundles. These are local tests, not installed AU,
  DAW-session or human listening verification. No music project was opened.
- Three alternating benchmark pairs: at 48 kHz/512 samples, median core-equivalent
  load rises **0.305% → 0.650%** (+113%). At 192 kHz/128 it rises 0.792% → 2.167%;
  eight 48 kHz instances rise 2.448% → 5.215% total. Dense one-CC-per-sample stress
  rises 0.513% → 0.809%. The cost is material relatively, but modest absolutely on
  this Mac. Maximum observed single-instance aggregate run load was 2.232%; this
  is not a worst-callback deadline or a guarantee for other machines. Raw CSVs,
  measurement protocol and all nine scenarios are in `cpu-comparison.json`.

[Open the listening comparison](../../artifacts/fidelity-followup-20260905/listening/index.html).
Five examples provide full output and isolated wet, matched and original gain.
They cover fast strums, dense chords, arpeggio, quiet decay and clean reconstruction.
The before version already includes the first pressure fix. The original
[first-pass comparison](../../artifacts/fidelity-20260905/listening/index.html)
is retained separately. Real DI is licensed Guitar-TECHS / FreePats material;
provenance and signal chains are recorded in the corpus ledger. It is excitation,
not a paired hardware reference.

## Reproduce

Run from the repository root; every output directory below is disposable/new:

```bash
RUN="$(mktemp -d /tmp/sendbloom-antialias.XXXXXX)"
python3 tools/reference/fidelity_compare.py prepare "$RUN" \
  --di-manifest /Users/niko/Datasets/Active/guitar/Phase135/real-di-manifest.json
python3 tools/reference/fidelity_compare.py render "$RUN" \
  --binary "$PWD/artifacts/fidelity-followup-20260905/baseline/RenderProcessor" --label baseline
python3 tools/reference/fidelity_compare.py render "$RUN" \
  --binary "$PWD/artifacts/fidelity-followup-20260905/oversampled/RenderProcessor" --label candidate
python3 tools/reference/compare_wet_antialias.py "$RUN"
open "$RUN/listening/index.html"
```

Omit the DI manifest for the generated-only 30-cell subset. The comparison drops
each build's own reported PDC and retains the common length; it does not perform
an arbitrary phase fit. The first-pass comparison script deliberately retains its
stricter equal-PDC contract.

```bash
RUN="$(mktemp -d /tmp/sendbloom-dirt-probe.XXXXXX)"
python3 tools/reference/probe_wet_dirt.py \
  artifacts/fidelity-followup-20260905/baseline/RenderTank "$RUN/before"
python3 tools/reference/probe_wet_dirt.py \
  artifacts/fidelity-followup-20260905/oversampled/RenderTank "$RUN/after"
cmake --build Builds/Performance --parallel 4 \
  --target Tests RenderProcessor RenderTank BenchmarkProcessor SendBloom_AU SendBloom_VST3
ctest --test-dir Builds/Performance --output-on-failure --no-tests=error
python3 -m unittest discover -s tests/reference
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
  --validate "$PWD/Builds/Performance/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
```

For CPU reproduction, alternate the preserved first-pass candidate
`artifacts/fidelity-20260905/candidate/BenchmarkProcessor` and the follow-up
`artifacts/fidelity-followup-20260905/oversampled/BenchmarkProcessor` three times,
redirecting each stdout to a new CSV while other builds/validators are idle.

Build metadata, final source patch/untracked source copies, compiler/submodule
identities and source/binary SHA-256s live under the follow-up artifact root.
The original baseline source archive plus first-pass patch reproduce this pass's
baseline in a separate checkout. No installation, commit, push or publication.

## Remaining uncertainties and next smallest measurements

This is not an exhaustive claim of hardware fidelity. Important open questions:

| Unknown | Why no further sound change was kept | Smallest useful direct measurement |
|---|---|---|
| Analog transfer / real input level / DISTN law | No calibrated voltages or paired wet recordings; retained 3× curve is still an approximation | Short coherent tone at three calibrated levels, Distn 0 and maximum, record exact same reamp twice |
| Reverb topology, density, frequency decay and modulation | Vendor DSP examples do not identify installed firmware; earlier density retune was rejected in listening | Two low-level impulses with Size maximum, Distn 0, Bright then Dark, 10 s capture each |
| Dark predelay and Size taper | Relative behavior and maximum are documented, numerical map is not | Same impulse at Size minimum/noon/maximum in each mode |
| Gate/pressure threshold and release | Full-release endpoint is supported; sent-input detector node and timing remain inferences | Steady source, Post, pressure hold/half/release/repress; matching DI |
| Input overload, dry gain and noise | FAQ mentions extreme dry overload but no threshold or circuit; guessing would damage normal dry behavior | Input-level ramp with Level 0, followed by terminated-input silence |

The best next small recording for overall fidelity is **two 10-second,
low-level direct impulse responses**, Bright/Dark, Size maximum, Distn minimum,
with the input and processed channels captured together. Use Pre, and keep a
very quiet pilot tone above the gate threshold if needed so the impulse is not
chopped; include the pilot separately for subtraction and record its level.
No amp/cab/normalization/limiter, include a calibration tone, knob photo, reamp
and interface gains, and serial/revision if visible. This would constrain the
largest remaining sound component: the reverb itself. It cannot by itself settle
the nonlinear transfer; the short three-level tone pair above is the next test.
