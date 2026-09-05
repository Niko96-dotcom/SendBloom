# Local development and verification

SendBloom is a JUCE plug-in, not a standalone audio application. Work locally
without installing over an existing AU/VST3 or opening a real DAW project.
Inspect `git status --short` and `git submodule status` first; preserve unrelated
changes, including changes inside submodules.

## Build and tests

On macOS: Xcode command-line tools, CMake >= 3.25, Ninja, Git, and Python 3.
The first configure needs network access for pinned r8brain and Catch2 sources.
Initialize missing submodules in a new checkout; do not run submodule updates
over someone else's submodule work. A logged-in desktop is needed for JUCE GUI
tests and previews. Linux GUI tests need a display (CI uses Xvfb).

Run from the repository root:

```bash
git submodule update --init --recursive
cmake -S . -B Builds/Performance -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds/Performance --config Release --parallel 4
ctest --test-dir Builds/Performance -C Release --output-on-failure --no-tests=error
python3 -m unittest discover -s tests/reference
bash tests/release/run-release-tests.sh
```

The reference Python tests use the standard library. The release tests use disposable fixtures and fake
signing/publishing tools. They do not publish anything. The full CTest suite
includes DSP, presets/state, editor rendering, MIDI, dry-path/PDC, real-time C++
allocation checks, legal metadata and macOS bundle signatures. Hidden diagnostic
benchmarks are not part of the default suite. The existing ZIP maximum-size test
is explicitly skipped on stock JUCE 8.0.12 because its required API is absent.

Logs: redirect build/test stdout and stderr to a chosen file; CTest also keeps
`Builds/Performance/Testing/Temporary/LastTest.log` and
`LastTestsFailed.log` when applicable. Read actual failures and skips rather than
treating a discovered test count as proof. C++ allocation tests do not intercept
direct `malloc` calls, locks, or system calls.

Validate the **local build** of VST3, if pluginval is installed:

```bash
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
  --validate "$PWD/Builds/Performance/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
```

This does not validate the installed AU or a real DAW session. Do not run
`auval` against the registered component and then attribute that result to an
uninstalled build. The existing `scripts/verify-v1.sh` is the broader fail-closed
release gate runner; this local workflow makes no signing, shipping, or hardware
equivalence claims.

## Interactive editor and snapshots

The optional `EditorPreview` uses a normal JUCE application lifecycle and a
macOS app bundle so accessibility tools can discover its window. It hosts the
real processor/editor in memory, without an audio device or persistent audio
settings. Audio processing is exercised separately by tests/benchmark/pluginval.

```bash
cmake --build Builds/Performance --config Release --parallel 4 --target EditorPreview
open -n "Builds/Performance/EditorPreview_artefacts/Release/SendBloom Editor Preview.app"
```

Its bundle identifier is `com.nikoaudiolabs.sendbloom.editor-preview`. Select a
factory preset, change Size/Dark/Gate, open Advanced, toggle Pressure Mode and
Extended Stereo, and exercise the pressure pad. Save a disposable `.sendbloom`
preset, change values, and load it back. On macOS use Cmd-Shift-G in the native
file picker to choose `/tmp`; putting a full path in the Save As name field can
turn slashes into filename colons. Check restored values and the visible Custom
label; the JUCE preset accessibility value can still name the previous factory
selection after custom edits. Close the preview window to quit.

The existing deterministic renderer also runs without installing anything:

```bash
Builds/Performance/EditorSnapshot /tmp/sendbloom-editor.png --advanced
bash scripts/capture-ui-state-matrix.sh "$PWD/Builds/Performance/EditorSnapshot" \
  "$PWD/artifacts/local-ui-matrix"
```

The matrix captures 17 states at 1x and 2x and checks dimensions. Inspect the
images as well; image creation alone does not prove controls work.

## Deterministic processor benchmark

```bash
cmake --build Builds/Performance --config Release --parallel 4 --target BenchmarkProcessor
mkdir -p artifacts/local-benchmark
Builds/Performance/BenchmarkProcessor "$PWD/artifacts/local-benchmark/before-audio" \
  > artifacts/local-benchmark/before.csv
```

The output directory must be **new**; the tool refuses to overwrite an existing
one. Omit the directory argument to measure without saving renders. Check its
exit code. It fails on non-finite/silent output or file-write failure. The binary
uses shipping code without `SENDBLOOM_ENABLE_DIAGNOSTICS`.

The nine scenarios cover 48/96/192 kHz, 64/128/512 sample blocks, eight serial
instances, and sparse/dense/stress pressure MIDI or irrelevant CC2. Input is a
precomputed harmonic pluck/silence sequence, restored before **every** callback.
Size and Dark change deterministically every 32 blocks. MIDI creation, prepare,
output validation, and file I/O are untimed. Copies and parameter changes are
timed. Each of seven repeats has a fresh prepare and about one second of warmup,
then 20 passes through about one second of input. The CSV reports min/median/max
wall-clock time as a percentage of one core's real-time budget; multiple
instances are summed, not averaged.

Each `.f32` is a separate untimed render from reset, in native float32 order:
block, instance, channel, sample. It covers about one second per instance.
It is a regression artifact, not a WAV or a listening reference. Every sample
is checked for finiteness. The comparison tool requires exact bytes when both
render directories are supplied, and rejects missing cases or changed rate,
block size, instance count or audio.

Before a code experiment, preserve the executable so before/after can alternate:

```bash
cp Builds/Performance/BenchmarkProcessor artifacts/local-benchmark/BenchmarkProcessor-before
# Make the source change, rebuild the same target with the same configuration.
cmake --build Builds/Performance --config Release --parallel 4 --target BenchmarkProcessor
Builds/Performance/BenchmarkProcessor "$PWD/artifacts/local-benchmark/after-audio" \
  > artifacts/local-benchmark/after.csv
python3 tools/compare-processor-benchmarks.py \
  artifacts/local-benchmark/before.csv artifacts/local-benchmark/after.csv \
  --before-audio artifacts/local-benchmark/before-audio \
  --after-audio artifacts/local-benchmark/after-audio
```

Use at least three alternating before/after runs, each with a new output path.
Do not benchmark while your build/tests/pluginval are running. Record compiler,
architecture, OS, background load, source revision and build options. Treat
small changes inside the timing spread as inconclusive. Stress MIDI at one
event per sample is deliberately extreme; it does not describe a normal pedal
performance or establish a worst-case host deadline guarantee. Do not use the
older hidden `[.performance-budget]` matrix for controlled A/B comparisons: its
output is reused as input, unlike this harness.

## Fresh worktree

Each worktree needs its own submodules and build directory. No symlinks to the
original checkout or copied dependency caches are required:

```bash
git worktree add --detach /tmp/sendbloom-check HEAD
git -C /tmp/sendbloom-check submodule update --init --recursive
cmake -S /tmp/sendbloom-check -B /tmp/sendbloom-check/Builds/Verify \
  -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/sendbloom-check/Builds/Verify --parallel 4 --target EditorSnapshot
/tmp/sendbloom-check/Builds/Verify/EditorSnapshot /tmp/sendbloom-worktree-editor.png --advanced
```

`HEAD` contains committed files only. To verify a local patch, apply exactly that
patch and its new files to the disposable checkout, or test the original working
tree explicitly. Never reuse another worktree's CMake cache. The commands above
were exercised on 2026-09-05; see the [measurement report](reports/performance-2026-09-05.md).
