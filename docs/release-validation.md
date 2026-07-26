# Release validation checklist

Every check, the exact command, and what a failure means. Run in this order —
each step assumes the previous one passed.

`scripts/release/release-all.sh` runs all of it and writes
`dist/<version>/release-report.md` with the result of each gate. This document
is the manual reference and the reviewer's checklist.

Throughout, `<version>` is whatever `VERSION` says:

```bash
scripts/release/version.sh version
```

---

## 1. Repository state

| # | Check | Command | Blocks release? |
|---|---|---|---|
| 1.1 | Working tree clean | `git status --porcelain` (empty) | yes |
| 1.2 | Public-tree hygiene and secret scan | `scripts/release/check-tree-hygiene.sh` | yes |
| 1.3 | Version bumped once, at `VERSION` only | `scripts/release/version.sh` | yes |
| 1.4 | Version consistency and stale-version search | `scripts/release/check-version-consistency.sh` | yes |
| 1.5 | Changelog top entry matches the version | covered by 1.4 | yes |
| 1.6 | Release notes title matches the version | covered by 1.4 | yes |
| 1.7 | Legal metadata audit | `scripts/check-legal-metadata.sh` | yes |
| 1.8 | Fidelity-claim truth | `scripts/verify-reference-claims.sh` | yes |

A stale-version failure names the file and line. Either fix the text or add a
reasoned exemption to `.release/version-allowlist.txt`. Adding an exemption
without a reason fails 1.4 on its own.

## 2. The release scripts themselves

| # | Check | Command | Blocks release? |
|---|---|---|---|
| 2.1 | Release-script regression tests | `bash tests/release/run-release-tests.sh` | yes |

These run with no credentials and no network: platform tools are faked so the
suite can assert on command ordering, missing-credential behaviour, rejected
notarization, checksum basenames, and the several valid shapes of `codesign`
output. Run one case with `bash tests/release/run-release-tests.sh T09`.

## 3. Build and product tests

| # | Check | Command | Blocks release? |
|---|---|---|---|
| 3.1 | Clean universal Release build | `scripts/release/build-macos.sh` | yes |
| 3.2 | Bundle metadata matches the version | re-run of 1.4 after the build | yes |
| 3.3 | Full Catch2 suite | `ctest --test-dir build-release -C Release --output-on-failure` | yes |
| 3.4 | ProperSRC / HF acceptance gates | `BUILD_DIR=build-release scripts/enab-acceptance-gates.sh` | yes |
| 3.5 | pluginval strictness 10 on the VST3 | `pluginval --strictness-level 10 --validate build-release/SendBloom_artefacts/Release/VST3/SendBloom.vst3` | yes for public |
| 3.6 | auval on the Audio Unit | `auval -v aufx SbLm NkMo` | skipped-with-note if unavailable |

3.1 removes the build tree first. Reusing a tree with `KEEP_BUILD_DIR=1` is a
debugging aid and prints a warning; it is not a release build.

## 4. Sign, notarize, staple, package

| # | Check | Command | Blocks release? |
|---|---|---|---|
| 4.1 | Developer ID signing with hardened runtime and secure timestamp | `scripts/release/package-macos.sh` (calls `sign-macos.sh`) | yes |
| 4.2 | Bundles notarized, then stapled, then validated | same | yes |
| 4.3 | Installer signed with Developer ID Installer, notarized, stapled (`pkg-in-dmg`) | same | yes for that contract |
| 4.4 | Installer assessed as an install source | `spctl --assess --type install` | yes for that contract |
| 4.5 | Disk image built only from finalised inner artifacts | same | yes |
| 4.6 | Disk image signed, notarized, stapled, Gatekeeper-assessed | same | yes |

Signature verification accepts hardened runtime reported either as a
`CodeDirectory … flags=0x…(runtime)` parenthetical or as a separate
`Runtime Version=` line. Both are valid `codesign` output; rejecting either
would block a correct release. Test T12 pins this.

## 5. Inventory, checksums, provenance

| # | Check | Command | Blocks release? |
|---|---|---|---|
| 5.1 | SBOM and licence citations | `scripts/release/sbom.sh` | yes |
| 5.2 | Checksums, generated only after stapling | `scripts/release/checksums.sh` | yes |
| 5.3 | Checksum entries are basenames | enforced inside 5.2 | yes |
| 5.4 | Checksums verify from a directory that is not the build directory | enforced inside 5.2 | yes |
| 5.5 | Manifest and provenance | `scripts/release/manifest.sh` | yes |

5.2 refuses to run at all if a `.dmg` or `.pkg` has no valid stapled ticket.
Stapling rewrites the file, so a checksum taken before it is simply wrong.

## 6. The artifact a user receives

```bash
scripts/release/validate-artifacts.sh
```

| # | Check | Blocks release? |
|---|---|---|
| 6.1 | Expected assets present, no leftovers from another release | yes |
| 6.2 | `SHA256SUMS.txt` verifies in place, entries are basenames | yes |
| 6.3 | Every artifact hash equals the manifest hash | yes |
| 6.4 | Disk image signature, stapled ticket, Gatekeeper verdict | yes |
| 6.5 | Disk image mounts; visible layout matches the contract | yes |
| 6.6 | `INSTALL.txt`, `LICENSE.txt`, `THIRD_PARTY_LICENSES.txt`, `RELEASE_NOTES.md` present | yes |
| 6.7 | Installer payload contains both plug-ins at the right versions, targeting the right install locations | yes for `pkg-in-dmg` |
| 6.8 | Plug-in bundles inside the image are Developer ID signed, hardened and stapled | yes for `bundles-in-dmg` |

This step never looks at the build directory. It mounts and unpacks the
shipped container, because that is the only thing a user ever sees.

## 7. Publication

```bash
scripts/release/publish-github.sh              # dry run — prints the plan
scripts/release/publish-github.sh --publish    # tags, pushes, uploads
```

| # | Check | Blocks release? |
|---|---|---|
| 7.1 | Mode is `public`, tree is clean | yes |
| 7.2 | All five assets present | yes |
| 7.3 | Artifacts stapled | yes |
| 7.4 | Manifest version and commit equal the current version and HEAD | yes |
| 7.5 | Artifact hashes still match the manifest | yes |
| 7.6 | Tag does not already point elsewhere; no existing release for it | yes |

## 8. Hosted artifact truth

```bash
scripts/release/verify-hosted-release.sh v<version>
```

| # | Check | Blocks release? |
|---|---|---|
| 8.1 | The release exists, is not a draft, prerelease flag matches the version | yes |
| 8.2 | The tag resolves to the commit the artifacts were built from | yes |
| 8.3 | Exactly the expected assets are attached, none obsolete | yes |
| 8.4 | Assets download into a **fresh** temporary directory | yes |
| 8.5 | Downloaded bytes are identical to what was built and tested | yes |
| 8.6 | The full section 6 validation passes against the downloaded files | yes |
| 8.7 | All CI check runs on the released commit succeeded | yes |

"It was fine locally" is not evidence. This section is the only thing that
proves what the world can actually download.

## 9. Installed truth

```bash
SENDBLOOM_INSTALL_SMOKE=1 scripts/release/install-smoke-macos.sh --i-understand
scripts/release/verify-installed-macos.sh
```

| # | Check | Blocks release? |
|---|---|---|
| 9.1 | Installed bundle `Info.plist` reports this version | yes, when run |
| 9.2 | Installed binary architectures and hash recorded | yes, when run |
| 9.3 | Installed copy is Developer ID signed, hardened, stapled | yes, when run |
| 9.4 | Installer receipts report this version (`pkg-in-dmg`) | yes, when run |
| 9.5 | `auval` accepts the installed Audio Unit | yes, when run |
| 9.6 | Upgrade over an existing install leaves no old version | yes, when run with `--upgrade` |
| 9.7 | Uninstall removes bundles and receipts completely | yes, when run with `--uninstall` |

Version facts come from the bundle plist, the binary, and package receipts —
never from Spotlight, `mdls`, or the AudioComponent registry cache. Those
caches will happily report a version that is no longer on disk.

`install-smoke-macos.sh` does nothing unless both `SENDBLOOM_INSTALL_SMOKE=1`
and `--i-understand` are given. When it does nothing, section 9 is SKIPPED and
must be named as such in the release report.

---

## Known caveats

Standing limitations of SendBloom release validation. Restate these in every
release report rather than letting a reader assume coverage.

- **Windows and Linux are not released.** CI builds a VST3 on both, but
  neither is signed, packaged, validated or published here.
- **Multi-host DAW smoke is manual.** pluginval and auval are automated; Logic,
  Cubase and REAPER are not. Where a host was not exercised for a release, the
  report says so.
- **Install, upgrade and uninstall smoke is destructive and opt-in.** It is
  skipped by default and is therefore usually a named caveat rather than a
  passed check.
- **Hardware fidelity comparison is outstanding.** `CLAIM_STATUS.md` holds the
  classification at `original-inspired`, with hardware grids and a dated blind
  listening verdict both `human_needed`. No release may imply otherwise.
- **JUCE commercial entitlement.** `docs/LICENSING_DECISION.md` records the
  chosen path; a selection is not proof of current coverage.
- **Notarization depends on Apple.** A submission can be slow or unavailable
  independently of this repository. That is a blocked release, not a passed
  one.

---

## Worked example

A complete dry run at the time of writing, canonical version `1.0.0`:

```bash
scripts/release/version.sh
bash tests/release/run-release-tests.sh
scripts/release/check-tree-hygiene.sh
scripts/release/check-version-consistency.sh
RELEASE_MODE=public scripts/release/release-all.sh
```

The last command writes `dist/1.0.0/release-report.md` containing every gate,
its status, the commands run, the artifact hashes, the skipped checks, and one
of READY / READY WITH CAVEATS / NOT READY.
