## What this changes

<!-- One or two sentences. What behaviour differs after this merges? -->

## Why

<!-- The problem, not the patch. -->

## Verification

<!-- What you actually ran, and what it said. Not what you expect to pass. -->

- [ ] `ctest --test-dir Builds -C Release --output-on-failure`
- [ ] Tested in a host (say which):

## Release impact

Tick anything that applies — these decide whether a release gate has to change.

- [ ] Changes the shipping parameter contract or preset format
- [ ] Changes anything under `scripts/release/`, `cmake-local/`, or `VERSION`
- [ ] Changes an installed path, bundle id, or signing requirement
- [ ] Changes product-facing copy that names a version
- [ ] Needs a `CHANGELOG.md` entry under `## [Unreleased]`

If any of the first four are ticked:

- [ ] `bash tests/release/run-release-tests.sh` passes
- [ ] `scripts/release/check-version-consistency.sh` passes
- [ ] `scripts/release/check-tree-hygiene.sh` passes

Do **not** bump `VERSION` in a feature PR. Version bumps happen once, in the
release PR — see [docs/release.md](../docs/release.md).

## Claims

- [ ] This PR makes no fidelity or hardware-comparison claim beyond
      `original-inspired` as recorded in `CLAIM_STATUS.md`
