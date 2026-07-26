# Releasing SendBloom

The maintainer process. One command does the whole thing; everything below
explains what that command does and what it needs.

```bash
RELEASE_MODE=public scripts/release/release-all.sh
```

That runs every gate and stops at the first failure without publishing.
Add `--publish` only when you have read the dry-run plan and mean it.

---

## Architecture in one page

```
VERSION  ─── the single canonical release version, e.g. 1.0.0 or 1.0.0-rc1
   │
   ├─ cmake-local/SendBloomVersion.cmake   numeric core -> CMake, JUCE, plists
   └─ scripts/release/lib.sh               everything else: artifact names,
                                           installer metadata, checksum file,
                                           release title, tag, build id

scripts/release/
  lib.sh                        shared helpers, version parse, naming, guards
  version.sh                    print any canonical version field
  check-version-consistency.sh  version truth + stale-version gate
  check-tree-hygiene.sh         public-tree hygiene + secret scan
  build-macos.sh                clean universal Release build
  sign-macos.sh                 Developer ID signing (or labelled ad-hoc)
  notarize-macos.sh             submit, wait, staple, validate
  package-macos.sh              the ordered pipeline that makes the artifact
  sbom.sh                       CycloneDX dependency + licence inventory
  checksums.sh                  SHA256SUMS.txt, basenames, post-staple only
  manifest.sh                   release manifest + provenance
  validate-artifacts.sh         validate what a user receives
  publish-github.sh             tag + GitHub Release (dry run by default)
  verify-hosted-release.sh      download the hosted assets and revalidate
  verify-installed-macos.sh     inspect what is actually installed
  install-smoke-macos.sh        destructive install/upgrade/uninstall test
  release-all.sh                the orchestrator

tests/release/                  regression tests with faked platform tools
.release/version-allowlist.txt  documented exemptions from the version gate
```

Every gate is fail-closed. There is no step that logs a warning and carries on
as if it had succeeded. Checks that are deliberately not run are reported as
**SKIPPED** and appear in the release report's residual-risk section; skipped
is never rendered as passed.

---

## Product and channels

| | |
|---|---|
| Product | Audio plug-in: Audio Unit + VST3 |
| Released platform | macOS 11+, universal (arm64 + x86_64) |
| Channel | GitHub Releases, `Niko96-dotcom/SendBloom` |
| Install paths | `/Library/Audio/Plug-Ins/VST3/SendBloom.vst3`, `/Library/Audio/Plug-Ins/Components/SendBloom.component` |

Windows and Linux VST3 builds run in CI but are **not** part of the signed
public release. Nothing in this pipeline produces or claims them.

## The public artifact contract

Choose one with `SENDBLOOM_ARTIFACT_CONTRACT`:

**`pkg-in-dmg`** (default) — a signed, notarized, stapled `.pkg` installer
inside a signed, notarized, stapled `.dmg`. The installer puts both plug-ins in
the system locations and leaves receipts, so upgrade and uninstall are
verifiable. Requires a **Developer ID Installer** certificate in addition to
the Application one.

**`bundles-in-dmg`** — the signed, notarized, stapled `.vst3` and `.component`
inside a signed, notarized, stapled `.dmg`, with an `INSTALL.txt`. The user
copies them into place. Requires only the **Developer ID Application**
certificate. Equally trusted by Gatekeeper; worse install UX and no receipts,
so `verify-installed-macos.sh` cannot check receipts under this contract.

If you set `pkg-in-dmg` without an Installer identity the pipeline stops
immediately and tells you both remedies. It does not silently downgrade.

Released assets, whichever contract:

```
SendBloom-<version>-macOS.dmg
SHA256SUMS.txt
release-manifest.json
provenance.json
SendBloom-<version>-sbom.json
```

---

## Prerequisites

### Signing and notarization credentials

None of these are stored in the repository. The scripts read them from the
environment and the keychain and never print their values.

| Variable | What it is | How to check |
|---|---|---|
| `DEVELOPER_ID_APPLICATION` | Exact name of the Developer ID Application identity | `security find-identity -v -p codesigning` |
| `DEVELOPER_ID_INSTALLER` | Exact name of the Developer ID Installer identity (only for `pkg-in-dmg`) | `security find-identity -v` |
| `NOTARY_PROFILE` | Name of a stored `notarytool` keychain profile | `xcrun notarytool history --keychain-profile "$NOTARY_PROFILE"` |

Create the notary profile once, interactively. Run this yourself — it prompts
for an app-specific password, which must never be pasted into a script, a
CI log, or this repository:

```bash
xcrun notarytool store-credentials "sendbloom-notary" --apple-id "<apple-id>" --team-id "<team-id>"
```

Then, for a release shell:

```bash
export DEVELOPER_ID_APPLICATION="Developer ID Application: <Name> (<TEAMID>)"
export DEVELOPER_ID_INSTALLER="Developer ID Installer: <Name> (<TEAMID>)"
export NOTARY_PROFILE="sendbloom-notary"
```

If you have an Application certificate but no Installer certificate, generate
the Installer one from the same Apple Developer account (Certificates →
Developer ID Installer), or release under `SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg`.

### Tools

`cmake` 3.25+, Xcode command line tools, `python3`, `gh` (authenticated), and
`pluginval`. Ninja is used when present. The pipeline checks for each and
fails with the missing name rather than half-running.

### Which checks need credentials

| Credential-free | Needs maintainer credentials or hardware |
|---|---|
| version consistency, stale-version search | Developer ID signing |
| tree hygiene and secret scan | notarization, stapling, Gatekeeper assessment |
| legal metadata and reference-claim audit | Developer ID Installer packaging |
| release-script regression tests | publishing to GitHub Releases |
| clean build, unit tests, pluginval, auval | hosted-artifact revalidation |
| artifact layout and installer payload inspection | install / upgrade / uninstall smoke (admin) |

---

## Bumping the version

Edit **one file**:

```bash
echo "1.1.0" > VERSION       # or 1.1.0-rc1 for a release candidate
```

Then add the matching `## [1.1.0]` section at the top of `CHANGELOG.md` and
rewrite `RELEASE_NOTES.md` for that version. Everything else — artifact names,
tag, installer metadata, bundle version, release title, checksum file, docs
examples — is derived. `check-version-consistency.sh` fails the release if any
product-facing number disagrees, and blocks it outright if the previous
released version still appears anywhere outside `.release/version-allowlist.txt`.

To exempt a legitimate historical literal, add a line to that allowlist with a
reason. An entry without a reason is itself a failure.

---

## Running a release

### 1. Rehearse locally (no credentials needed)

```bash
scripts/release/release-all.sh
```

Defaults to `RELEASE_MODE=local-unsigned`: ad-hoc signatures, no notarization,
every artifact renamed `…-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE`, and publishing
hard-blocked. It exercises the whole pipeline so a real release does not
discover a scripting problem halfway through signing.

### 2. Dry run the real thing

```bash
RELEASE_MODE=public scripts/release/release-all.sh
```

Builds, signs, notarizes, staples, packages, validates, and prints exactly
what publishing would do — without publishing. Read the plan.

### 3. Publish

```bash
RELEASE_MODE=public scripts/release/release-all.sh --publish
```

This creates the tag, pushes it, creates the GitHub Release with the five
assets, then downloads those assets into a fresh temporary directory and
re-runs the full validation against them.

### 4. Prove the installed product is this build

On a machine you are willing to modify:

```bash
SENDBLOOM_INSTALL_SMOKE=1 scripts/release/install-smoke-macos.sh --i-understand
scripts/release/verify-installed-macos.sh
```

Without this, install smoke is reported as SKIPPED in the release report, and
the release is at best READY WITH CAVEATS.

---

## What the pipeline refuses to do

- Sign, notarize or publish when a required credential is absent.
- Build a public release from a dirty working tree.
- Generate checksums before signing, notarization and stapling are finished.
- Build a disk image around an artifact that has not already been signed,
  notarized, stapled and validated.
- Publish a rehearsal (`local-unsigned`) artifact.
- Publish when an artifact's hash no longer matches the manifest.
- Retag a version that already points somewhere else, or overwrite an existing
  GitHub Release. Both are `docs/rollback.md` situations.
- Treat a skipped check as a passed one.

## See also

- [docs/release-validation.md](release-validation.md) — the validation
  checklist and the exact commands.
- [docs/install.md](install.md) — what users do.
- [docs/rollback.md](rollback.md) — when a release has to be pulled.
- [CHANGELOG.md](../CHANGELOG.md)
