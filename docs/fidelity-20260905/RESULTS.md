> Historical first-pass results. The later [distortion follow-up](FOLLOWUP.md)
> supersedes the current-build identity, latency and sound claims below; the
> preserved first-pass audio and its bit-identical controls remain valid.

# SendBloom fidelity result — 2026-09-05

Implemented one supported sound/interaction correction: **Post-gated pressure
release now cuts the reverb while dry playing continues**. Pre still allows
trails. This follows the manufacturer's [Igor/Gate description](https://www.raingerfx.com/shop/p/reverb-x).
It does not establish circuit identity or a hardware-matched timbre.

[Open the four-pair listening page](../../artifacts/fidelity-20260905/listening/index.html)
• [Reproduction commands](REPRODUCE.md)
• [Research/evidence map](RESEARCH.md)
• [Source ledger](SOURCES.jsonl)

## Consequential findings and accepted changes

1. **A documented signature interaction was missing.** The old gate detector
   listened to unsent input, so playing dry after releasing the pad held the wet
   gate open. The new detector follows the smoothed send in Post and retains its
   original key in Pre. Existing placement smoothing prevents a separate abrupt
   detector switch. No gate threshold, hold, release or pressure curve changed.
   The exact physical detector node and partial-pressure behavior are still
   uncertain; this is the smallest behavioral model consistent with the sourced
   endpoints, not a schematic-derived circuit claim.
2. **Earlier research overstated topology evidence.** Chip memory capacity does
   not force an allpass ring; Spin allows different clocks and allpass counts.
   Corrected architecture documentation/source comments and stale Unreleased
   changelog entries. The previously rejected density/drive experiments were
   not restored. Current reverb, filters and distortion retain accepted voicing.
3. **Calibration limits remain substantial.** Minimum Size, decay taper,
   predelay amount, filters, modulation and analog transfer are unmeasured.
   Retuning these from different performances or amp/cab-processed videos would
   confound the source chain with the effect. No matched public stems, product
   schematic, program, or numbered revision specification were found.

## Presets and compatibility

No parameter IDs, ranges, defaults, state schema or preset files changed.
Factory/state/MIDI tests pass. All Pre and disconnected-send corpus cases are
byte-identical. Post with full send retains the previous steady-state key.

**Hot Clip** uses connected pressure plus Post: releasing now cuts the existing
wet tail; partial pressure may close the gate at a higher incoming instrument
level. Existing saved sessions with the same combination change likewise.
Other presets retain their normal behavior; manually enabling pressure with
Post activates the corrected interaction. Pre/pressure presets, including Dry
Dub Sends and Firm Pressure, retain trails. No per-preset gain/EQ compensation.

## Reproducible audio evidence

Baseline is clean HEAD `95a3012b69ab7c0da5225b5ed422131debac02a2` plus the
non-shipping renderer harness. Source, CMake cache and executables are retained
under `artifacts/fidelity-20260905/baseline/`. Candidate remains uncommitted.
Build: Release arm64, Apple clang 21.0.0, macOS 26.5.2, JUCE 8.0.12 at
`29396c22c93392d6738e021b83196283d6e4d850`, pinned r8brain
`e71c31bf320f84210bb4bdcb57e296c39ce940f9`. Copy-after-build stays disabled.

Renderer SHA-256:

- Baseline: `b9aab9933db01d36da3e0fb841c522ff950149110144591ee5eba2d7a6fc7a6f`
- Candidate: `ca5d4d1c9e6895e75909de45b4911d9a66f66256273f618454d3f96acf53acac`

The baseline was built and all baseline audio rendered **before** changing DSP.
Ten inputs × six settings = **60 cells per build**, each rendered twice with
byte-identical results. Five real DI clips cover soft notes, open/dense chords,
arpeggios and fast strums; five generated stimuli cover quiet decay, sustained
notes/chords, impulses and strong transients. Each carries 250ms lead-in and
8s tail silence. Full production processing includes input, pressure smoothing,
gate, SRC, tank, wet dirt, level/output mix, and actual dry PDC.

**44 of 60 outputs are byte-identical.** All 40 cases outside Post-pressure are
identical; four quiet/impulse Post-pressure cases also produce no difference.
Every changed cell belongs to the targeted Post-pressure interaction.

| Post release, continued input | Old wet RMS | New wet RMS |
|---|---:|---:|
| 220Hz sustained tone | -20.38 dBFS | exactly zero |
| Sustained generated chord | -26.87 dBFS | exactly zero |
| Real DI fast strums | -29.14 dBFS | exactly zero |
| Real DI slow arpeggio | -27.91 dBFS | exactly zero |

Wet is isolated by subtracting the known PDC-aligned dry input from the full
output; window is 200–1200ms after release. JSON uses a -300dB numerical floor
for zero, not a claim of 300dB physical rejection. For the sustained tone at
48k/128, the last residual above 1e-7 occurs **44.19ms after release**. That is
measured software behavior, **not a hardware release-time estimate**.

The matched examples are seven-second excerpts: press at 0.25s, release at
1.75s; the Soft/repress example presses again at 3s. Match full-clip RMS to
-20dBFS with common peak headroom attenuation if needed. Exported packed-24-bit
pairs differ by less than **0.000000014dB RMS**; highest matched peak is
**-2.13dBFS**. Pre is an unchanged listening control. Human sonic preference,
blind listening and DAW audition were **not performed**.

Absolute-gain float WAVs are included separately, with exactly unity export gain.
RMS matching a shorter tail raises its surviving notes relative to an unmatched
comparison, so both versions matter. Across the full unnormalized corpus, the
largest peak is **+2.64dBFS in both builds**: existing gain summation can exceed
full scale. Float renders retain those peaks; no limiter was added and no claim
is made about corresponding hardware volts or clipping thresholds.

## Reverb measurements and preserved uncertainty

Six raw tank IRs per build (1.2/3/6s target × Bright/Dark) are byte-identical.
Existing scalar Schroeder T30 analysis was cross-checked with an independent
NumPy cumulative-energy/linear-fit calculation (agreement <1e-8s).

| Size target | Bright measured T30 | Dark measured T30 |
|---|---:|---:|
| 1.2s | 1.475s | 1.339s |
| 3s | 3.002s | 2.666s |
| 6s | 5.659s | 5.146s |

Fits use -5 to -35dB integrated energy and extrapolate to 60dB; R² ranges
0.9912–0.9995. The maximum remains compatible with the manual's stated 5–6s
range. The 1.2s minimum is a control target, not an exact measured tail duration.
Bright/Dark tank onset at threshold 1e-4 remains 6.409/61.462ms. These show
software response and preservation; no distance to a measured pedal is claimed.

## Verification and CPU

- New regression failed on baseline (post-release wet RMS 0.18116 at 44.1k/64),
  then passed all 24 rate/block/feel/control combinations, checking both Pre and
  Post through the shipping processor. 1,655,376 assertions in one case.
- Final CTest: **284 discovered, 283 passed, 1 existing capability skip, 0 failed**.
  Skip: stock JUCE lacks the ZIP maximum-uncompressed-size API. Includes state,
  presets, dry integrity/PDC, sample-positioned MIDI, oversized/random blocks,
  hostile-input stability, editor tests and C++ allocation interception.
- Reference Python suite: **16 passed**. New renderer additionally exercised by
  240 successful production renders and exact repeats; comparison verifies hashes.
- Release AU/VST3 build and strict bundle signatures pass. Local **VST3
  pluginval strictness 10: SUCCESS**, including parameter automation, editor,
  state and bus-layout tests. Final comment-only rebuild retained identical
  renderer, Tests, AU and VST3 executable hashes. No installed AU `auval` result
  is attributed to this build. No real DAW session was opened.
- One initial aggregate failed the repository's metadata rule because a new
  source comment/test name used hardware branding. Changed those labels to
  generic wording and reran the full suite; original failed log retained.
- DSP change adds bounded scalar arithmetic only; no storage, allocation,
  locks, I/O or logging added. C++ allocation guards pass. They do not intercept
  all direct malloc calls, locks or system calls; no universal realtime proof.

Three alternating benchmark pairs, seven repeats each; median of per-run medians
shown as percent of one core's audio-time budget:

| Scenario | Before | After | Relative change |
|---|---:|---:|---:|
| 48k / 64 | 0.341% | 0.341% | +0.02% |
| 48k / 512 | 0.319% | 0.317% | -0.67% |
| 96k / 128 | 0.500% | 0.494% | -1.33% |
| 192k / 128 | 0.809% | 0.818% | +1.13% |
| Eight instances, total | 2.576% | 2.542% | -1.33% |
| Sparse pressure MIDI | 0.321% | 0.418% | +30.08% |
| Dense pressure MIDI | 0.324% | 0.330% | +1.97% |
| One pressure event/sample | 0.521% | 0.573% | +9.88% |
| Irrelevant MIDI | 0.321% | 0.324% | +0.86% |

Median relative change across scenarios: +0.86%. Sparse/stress MIDI increases
are retained rather than hidden; timings show large transient variation, with
maximum observed candidate aggregate 2.34% for one instance and 6.95% for eight.
The machine/background load was uncontrolled. These numbers do not establish a
worst-case callback deadline or rule out a small overhead. Benchmark uses Pre;
all nine untimed benchmark audio renders are identical. Actual Post behavior and
automation are covered separately by functional/realtime tests. No performance
optimization or topology retune was justified by these measurements.

## What is closer, and what would resolve the remaining question

The **release endpoint** now agrees with the documented Post/pressure behavior.
Reverb timbre and distortion are preserved, not proven closer. There is no
accuracy percentage, perceptual preference claim, hardware null comparison or
DAW listening claim.

The highest-value additional capture is one 10-second direct DI/output pair:
steady reamped tone or chord, Post/Bright, Size/Level noon, Distn minimum, Input
below overload; press, release while source continues, hold partial pressure,
then repress. Include knob photo, revision/serial and calibrated interface/reamp
levels. Release measures timing; partial pressure distinguishes a sent-input
key from a separate pressure mute; repress reveals tank-state behavior. This
can be supplied by a remote owner. See [research protocol](RESEARCH.md#smallest-resolving-capture).

No installation, real music-project edits, commit, push, deployment or publication.
