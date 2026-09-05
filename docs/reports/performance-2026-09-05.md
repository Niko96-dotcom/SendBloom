# Processor performance and local verification — 2026-09-05

Baseline: `3149e6b` (`main`, initially clean working tree). Candidate: the local
MIDI cursor change in this working tree. No commit, push, install or release was
performed. Shipping DSP equations, features, control quantum, latency and preset
formats are unchanged.

## Change selected from measurement

Previously each control span scanned the whole MIDI buffer twice, and the
next-event scan constructed an owning `MidiMessage` even for irrelevant SysEx.
With pressure events at every sample that is quadratic work in the number of
events. The processor now walks sorted, non-owning MIDI metadata once, skipping
irrelevant messages. It preserves last-CC1-wins ordering, reset priority at a
shared timestamp, sample offsets, out-of-block exclusion and APVTS purity.

The original hidden CPU matrix reuses output as the next input. Added an
opt-in production-code benchmark with restored deterministic input, parameter
ramps, prebuilt MIDI, reset renders and an exact-output comparison tool. Added
an optional JUCE editor preview application for manual control/preset checks
without a DAW or plug-in installation, plus a tested
[local verification guide](../local-verification.md).

## Measurements

Apple M5 Pro, macOS 26.5.2, native arm64, AppleClang 21.0.0.21000101,
CMake 4.3.3/Ninja, Release with the repository's normal LTO/configuration flags.
Both sustained binaries used byte-identical `BenchmarkProcessor.cpp`.
The baseline was rebuilt in a newly initialized detached worktree at `3149e6b`,
with only the benchmark source/target added; production source stayed unchanged.

Three alternating baseline/candidate runs, seven repeats per scenario per run.
Each repeat: prepare, about one second of warmup, then 20 passes of about one
second of audio. Numbers below are the median of the three within-run medians.
Percentages are wall time divided by audio duration, expressed as one core's
real-time budget; eight instances are processed serially and summed.

| Scenario | Before | After | Relative time reduction |
| --- | ---: | ---: | ---: |
| Audio, 48 kHz / 64 | 0.4091% | 0.3933% | +3.9% |
| Audio, 48 kHz / 512 | 0.3807% | 0.3987% | -4.7% |
| Audio, 96 kHz / 128 | 0.6116% | 0.5350% | +12.5% |
| Audio, 192 kHz / 128 | 0.9013% | 0.8522% | +5.5% |
| Eight instances, 48 kHz / 512 | 3.0709% | 2.8879% | +6.0% |
| Pressure every 128 samples, 48 kHz / 512 | 0.4004% | 0.3366% | +15.9% |
| Pressure every 16 samples, 48 kHz / 512 | 0.4649% | 0.3522% | +24.2% |
| Pressure every sample, 48 kHz / 512 | 8.5949% | 0.5788% | +93.3% |
| Irrelevant CC2 every sample, 48 kHz / 512 | 0.4409% | 0.3805% | +13.7% |

[Raw per-run min/median/max measurements](performance-2026-09-05.csv) are retained
in the repository. All nine reset audio renders were byte-identical in each
of the three comparisons (27 matched pairs).

Limitations: other tasks were compiling/running on this Mac; their work was
not interrupted. Our own builds/tests/pluginval finished before the sustained
measurement batch, though brief manual UI activity overlapped it. Ordinary
audio cases fluctuate in both directions, so no general audio-processing
speedup is claimed. For example, the 48 kHz/512 paired changes ranged from
-18.7% to +14.8%. The stress improvement was 91.7–93.9% in all three pairs;
every-16-sample pressure improved 11.6–34.7%. These MIDI results support keeping
the narrow algorithmic change. This is synthetic offline throughput, not a
real-time deadline, DAW-meter, hardware, or listening result. The exact audio
comparison covers a separate approximately one-second reset render per case,
not every sample of the longer timed run.

Preliminary shorter measurements are retained in ignored local artifacts but
are not used in this table.

## Exercised verification

- Clean configure and full Release build of AU/VST3, Tests, EditorSnapshot,
  SvgSnapshot and RenderTank. Optional benchmark and preview targets built/run.
- Baseline CTest: 280 passed, one skipped. Final CTest: 282 passed, one skipped,
  zero failed (283 discovered), including both strict bundle-signature checks.
- Existing skip: JUCE 8.0.12 lacks the maximum-uncompressed-ZIP API. This task
  did not hide, weaken or claim to pass that test.
- 16 Python reference tests and 19 release-script fixture tests passed.
- Baseline and candidate build-local VST3 both passed pluginval strictness 10,
  including audio processing, state, editor and parameter fuzz tests.
- New collision/boundary regression passes both baseline and candidate: reset
  wins regardless of order, the final CC1 wins otherwise, irrelevant SysEx/CC2
  does not alter audio, negative/end-of-block events are ignored, and oversized
  blocks preserve behavior. Exact buffer bytes and final pressure are compared.
- Existing C++ allocation checks plus a large-SysEx fixture pass. These count
  C++ `new`, not direct `malloc`; this is not comprehensive real-time-safety proof.
- Native preview UI: selected DRY DUB SENDS, changed Size to 0.75, enabled Dark
  and Extended Stereo, adjusted pressure, saved a disposable `.sendbloom` file,
  changed the controls and reloaded the file. The restored values were observed
  through accessibility and the Custom label visually. Closed the window and
  confirmed its process exited. The fixture is retained in the local evidence.
- 17 snapshot states at 1x/2x generated with correct dimensions; the advanced
  view was visually inspected. This complements the live interaction check.
- A fresh detached worktree initialized both pinned submodules, downloaded
  dependencies through CMake, built EditorSnapshot and rendered an advanced
  editor image, without shared caches, source symlinks or manual dependency copies.
  The sustained baseline benchmark was subsequently built there as well.
- Comparator exercised on real results; missing-case and deliberately altered
  audio fixtures were rejected. Benchmark refusal to overwrite an existing
  output directory was exercised.

Still unverified: installed AU validation, real Cubase/Ableton sessions, hardware
audio/listening, Windows/Linux builds, and universal x86_64 execution. Those need
an isolated host configuration and test plug-in location (or a disposable user
account), appropriate platform runners, and an audio/listening setup. The preview
intentionally has no audio device. Its native file picker can store normal OS
picker preferences; it does not initialize persistent audio settings. The
pre-existing Custom preset accessibility value can retain the factory name
even when the painted label correctly reads Custom; future UI checks should
inspect both.

## Exact reproduction

Working commands for build, CTest, Python/release tests, pluginval, the preview,
snapshots and fresh worktrees are in [local-verification.md](../local-verification.md).
The original local evidence is under `artifacts/performance-20260905/`, including
the two sustained binaries, raw CSVs, float renders, snapshots and logs. To rerun
the preserved pair from the repository root (choose a new output folder):

```bash
set -e
evidence="$PWD/artifacts/performance-20260905"
rerun="$(mktemp -d "$PWD/artifacts/sendbloom-rerun.XXXXXX")"
for round in 1 2 3; do
  "$evidence/BenchmarkProcessor-baseline-sustained" "$rerun/before-$round" > "$rerun/before-$round.csv"
  "$evidence/BenchmarkProcessor-candidate-sustained" "$rerun/after-$round" > "$rerun/after-$round.csv"
  python3 tools/compare-processor-benchmarks.py "$rerun/before-$round.csv" "$rerun/after-$round.csv" \
    --before-audio "$rerun/before-$round" --after-audio "$rerun/after-$round"
done
```

To reconstruct the baseline without relying on retained binaries, use the
current harness with production sources from `3149e6b`:

```bash
baseline="$(mktemp -d /tmp/sendbloom-baseline.XXXXXX)"
git worktree add --detach "$baseline" 3149e6b
git -C "$baseline" submodule update --init --recursive
cp tools/BenchmarkProcessor.cpp "$baseline/tools/BenchmarkProcessor.cpp"
cat >> "$baseline/CMakeLists.txt" <<'CMAKE'
add_executable(BenchmarkProcessor EXCLUDE_FROM_ALL tools/BenchmarkProcessor.cpp)
target_compile_features(BenchmarkProcessor PRIVATE cxx_std_20)
target_compile_definitions(BenchmarkProcessor PRIVATE
    $<TARGET_PROPERTY:SendBloom,COMPILE_DEFINITIONS>)
target_link_libraries(BenchmarkProcessor PRIVATE SharedCode)
CMAKE
cmake -S "$baseline" -B "$baseline/Builds/Verify" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$baseline/Builds/Verify" --parallel 4 --target BenchmarkProcessor
"$baseline/Builds/Verify/BenchmarkProcessor" "$baseline/before-audio" > "$baseline/before.csv"
```

Build the candidate with the same compiler/configuration via the guide, then
compare the CSVs/renders. A future changed harness must be copied to **both**
revisions; never compare results from different workloads. The preserved binary
SHA-256 values are:

```text
baseline  4f7f0f61002651b891974665eba69ae0592a2ad98cf3f73c19aacd5e5c6552e6
candidate 03ef6d8b00e2bdaa9a6ce13cf0c5987e8c55d20a5d5a258f81492a01253c0e0f
```
