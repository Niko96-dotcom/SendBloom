# Reproduce the comparison

Run from `/Users/niko/Documents/RAINGER FX`. No installation or real projects are
involved. Python needs NumPy and SciPy (the current `/opt/homebrew/bin/python3`
has both). Every generated directory below must be new; render outputs refuse
replacement. The 60-cell render repeats every invocation and verifies byte equality.

## Replay the preserved baseline and candidate

```bash
# Choose a new disposable output directory each time.
RUN="$(mktemp -d /tmp/sendbloom-fidelity.XXXXXX)"
python3 tools/reference/fidelity_compare.py prepare "$RUN" \
  --di-manifest /Users/niko/Datasets/Active/guitar/Phase135/real-di-manifest.json
python3 tools/reference/fidelity_compare.py render "$RUN" \
  --binary "$PWD/artifacts/fidelity-20260905/baseline/RenderProcessor" --label baseline
python3 tools/reference/fidelity_compare.py render "$RUN" \
  --binary "$PWD/artifacts/fidelity-20260905/candidate/RenderProcessor" --label candidate
python3 tools/reference/fidelity_compare.py compare "$RUN"
open "$RUN/listening/index.html"
```

Omit `--di-manifest` to run the five entirely generated inputs, if that licensed
local corpus is unavailable. That is a smaller 30-cell comparison, not the full
60-cell verification. Source corpus is read-only. Original gains are preserved;
250ms lead-in and 8s tail silence are added. All 13 parameter values and CC1
sample positions are in each generated settings JSON. Default rate/block: 48k/128.

The original run is `artifacts/fidelity-20260905/`. Baseline source is HEAD
`95a3012b69ab7c0da5225b5ed422131debac02a2`; its source tar, CMake cache, renderer
source and CMake tooling patch are in `baseline/`. Candidate source patch and
new source files are in `candidate/`. JSON identity receipts contain compiler,
submodules, source hashes and executable hashes. Nothing was committed or pushed.

## Build and verify the current candidate

```bash
cmake -S . -B Builds/Performance -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build Builds/Performance --parallel 4 \
  --target RenderProcessor RenderTank BenchmarkProcessor Tests SendBloom_AU SendBloom_VST3
Builds/Performance/Tests '[pressure-post-gate]'
ctest --test-dir Builds/Performance -C Release --output-on-failure --no-tests=error
python3 -m unittest discover -s tests/reference
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness-level 10 \
  --validate "$PWD/Builds/Performance/SendBloom_artefacts/Release/VST3/SendBloom.vst3"
```

`COPY_PLUGIN_AFTER_BUILD FALSE` is retained. Do not use registered `auval` to
attribute an installed component's result to these uninstalled builds.

For a fresh build, choose a new build directory instead of `Builds/Performance`;
JUCE and cmake submodules must be initialized. The baseline can be rebuilt by
unpacking `baseline/source-HEAD.tar` in a disposable checkout with the recorded
submodules, applying `baseline/tooling.patch` and adding the archived
`RenderProcessor.cpp` under tools. This adds the same renderer without the DSP
change. Do not apply a baseline patch over the working project.

## Reproduce a tank measurement

```bash
RUN="$(mktemp -d /tmp/sendbloom-ir.XXXXXX)"
artifacts/fidelity-20260905/baseline/RenderTank ir "$RUN/bright.f32" 6 0 10 ring
python3 tools/reference/measure_ir.py "$RUN/bright.f32" \
  --sample-rate 32768 --json "$RUN/bright.json"
```

Repeat with Dark `1`, targets `1.2`, `3`, `6`, and the candidate RenderTank.
`ir-comparison.json` preserves all 12 commands and metrics. The candidate and
baseline IRs are byte-identical; these measure the software only.

## Reproduce CPU observations

Run while builds, validators and tests are idle. Alternate baseline/candidate
three times; neither benchmark writes a real session or opens an audio device.

```bash
RUN="$(mktemp -d /tmp/sendbloom-cpu.XXXXXX)"
for n in 0 1 2; do
  artifacts/fidelity-20260905/baseline/BenchmarkProcessor > "$RUN/baseline-$n.csv"
  artifacts/fidelity-20260905/candidate/BenchmarkProcessor > "$RUN/candidate-$n.csv"
done
```

Each of nine scenarios already performs seven fresh-prepare repeats, about one
second warmup and twenty seconds of audio per repeat. Copying and deterministic
automation are timed; I/O and prepare are excluded. Eight-instance row is total
core load. The existing benchmark uses Pre, so it measures added detector work
and processor cost, not a separate Post-specific deadline distribution. Post is
covered by the new functional regression and realtime automation tests.
