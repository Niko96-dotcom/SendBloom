# Repository housekeeping and local UI verification — 2026-09-30

This change commits the pending registered-scene editor update from a working
tree based on `9474c5d`. It also corrects the snapshot dimensions and UI docs,
adds the new snapshot states, lists runtime artwork explicitly in CMake, and
keeps unused design studies ignored locally. No dependency revisions, release
tag, installed plug-in or public release are changed by this housekeeping.

## Local evidence

The Release build was configured with Ninja in `Builds/Performance`, using the
existing pinned JUCE and cmake submodules. The machine is arm64 macOS 26.5.2,
with CMake 4.3.3 and Apple Clang 21.0.0. Build output stays ignored.

| Check | Observed result |
|---|---|
| Release AU/VST3, Tests and snapshot build | Passed |
| Focused editor tests | 18 cases, 270 assertions passed |
| Full CTest | 290 discovered: 289 passed, 1 skipped, 0 failed |
| Python reference tools | 16 tests passed |
| Release-script fixtures | 19 suites passed, 0 failed/skipped |
| Local VST3 pluginval | Strictness 10 passed |
| AU/VST3 strict bundle signatures | Passed with local ad-hoc signatures |
| Version consistency including built bundles | Passed for both bundles |
| Legal metadata and reference-claim checks | Passed; original-inspired classification retained |
| Dependency/licence inventory | SBOM generation passed |
| Snapshot matrix | 23 states at 840×700 and 1680×1400; dimensions passed |
| Visual inspection | Default, Advanced and longest-preset menu inspected at 1x |
| Changed Markdown local links, shell syntax, diff whitespace | Passed |

The skipped ZIP maximum-size test requires an API absent from stock JUCE
8.0.12; it is not reported as passing. The full CTest also includes registered
control motion, pressure-release, accessibility and palette regressions.
Screenshots are runtime editor renders, not DAW interaction evidence.

Local logs and 46 snapshot images are in
`artifacts/housekeeping-20260930/` and remain ignored. Commands are documented in
[local verification](../local-verification.md). The built-bundle version check
used `RELEASE_BUILD_DIR="$PWD/Builds/Performance"` so it checked these bundles
rather than a missing release build directory.

## Git and GitHub housekeeping

After fetching/pruning, local `main` and `origin/main` had no divergence. GitHub
had no open pull requests and only the `main` branch; historical PRs were already
merged or closed, so no PR closure or branch deletion was needed. Nine stale
worktree registrations whose checkout paths no longer existed were pruned.
Earlier v2/v4 and clear-instruments artwork and the unused layout prototype were
preserved locally; they are not runtime build inputs.

The source update is published by the commit containing this report. GitHub CI
results must be read separately for that commit; earlier green runs are not
validation of this tree. This report makes no installed DAW, AU registration,
hardware comparison, notarization, universal-build or new-release claim.
