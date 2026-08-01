# SendBloom release-readiness report — 1.0.0

- Project: SendBloom
- Date: 2026-08-01 (Europe/Berlin)
- Scope: published macOS AU/VST3 release, `pkg-in-dmg` contract
- Proof level: `PUBLISHED`
- Verdict: `READY WITH CAVEATS`
- Release verified: `NO` (the verdict has named optional/environment-specific caveats)
- Version: `1.0.0`
- Source commit: `e61576572a0b99950a891261df9d9df61955089f`
- Tag/release: [`v1.0.0`](https://github.com/Niko96-dotcom/SendBloom/releases/tag/v1.0.0)

## Decision

The public release is live and the exact hosted DMG and PKG were fetched into a
fresh directory, matched to the local manifest byte-for-byte, and passed the
full artifact validator. The final public artifacts are Developer ID signed,
notarized, stapled, Gatekeeper-accepted, and installable as universal
arm64+x86_64 AU/VST3 components.

The exact public artifact was installed locally with administrator approval.
The installed AU and VST3 binary hashes match the final build, both receipts
are version `1.0.0`, and `auval -v aumf SbLm NkMo` passes. Linux, macOS, and
Windows CI are green on the immutable release commit after rerunning a
transient macOS UI-test failure.

This is `READY WITH CAVEATS`, not `RELEASE VERIFIED`: the signed GitHub Actions
duplicate was an optional job run from the pre-hardening tag workflow and
failed because repository signing secrets were absent; the local maintainer
pipeline supplied the signed/notarized artifact. DAW-host behavior, uninstall /
rollback, store review, and hardware-equivalence listening remain separate
checks and are not silently promoted to PASS.

## Release contract

| Item | Expected |
| --- | --- |
| Product/components | SendBloom AU component and VST3 bundle |
| Platforms/architectures | macOS 11+; universal `arm64` + `x86_64` |
| Distribution channel | Public GitHub Release |
| User artifact | `SendBloom-1.0.0-macOS.dmg` containing `SendBloom-1.0.0.pkg` |
| Hosted metadata | checksum, manifest, provenance, and SBOM alongside the DMG/PKG |
| Install targets | `/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/Components` |
| Release tag | Immutable `v1.0.0` at the source commit above |

## Gate evidence

| Gate | Status | Evidence |
| --- | --- | --- |
| Contract/version/source identity | PASS | Canonical `VERSION=1.0.0`; tag, manifest, and release commit agree |
| Local release-script regressions | PASS | `bash tests/release/run-release-tests.sh`: 18/18 |
| Local release build and product gates | PASS | Clean universal build; CTest 284 discovered, 283 passed, 1 intentional capability skip (#281); ENAB PASS; pluginval strictness 10 PASS |
| Security/hygiene/legal/reference claims | PASS | Tree hygiene, legal metadata, SBOM, and reference-claim checks passed; claim status remains `original-inspired` with human evidence separate |
| Developer ID signing | PASS | Application and Installer identities for team `4H5447ZWS3`; hardened runtime verified |
| Notarization/stapling/Gatekeeper | PASS | Accepted submissions for bundles `32b2c479-cf71-4367-b962-64b83a76c327`, PKG `7e9c502d-04aa-43d8-817a-156faeed2842`, DMG `a28c6073-33d6-4f32-8d59-0d4637a5ee88` |
| Hosted asset set | PASS | Exactly six contracted assets; no stale extras |
| Fresh hosted download | PASS | `verify-hosted-release.sh v1.0.0`; downloaded DMG and PKG match the local manifest and pass fresh validation |
| Required post-publish CI | PASS | Run `30687751753`: Linux, macOS, Windows all completed `success`; credential-free release gates completed `success` |
| Optional signed CI duplicate | WARNING | Historical run `30687751754` failed only at missing signing secrets; `ENABLE_SIGNED_RELEASE` is unset/false, so the duplicate is not a required product gate. Main commit `e6fe996` gates it correctly for future tags. |
| Installed/deployed identity | PASS | Public artifact install/upgrade smoke; 0 failures, 0 skipped; installed AU/VST3 direct hashes match build; receipts and `auval` pass |
| Uninstall/rollback | NOT RUN | No destructive uninstall or rollback was run after the final install; the verified release was intentionally left installed |
| DAW host smoke/soak | NOT RUN | No host was restarted and no DAW session was used for this release proof |
| Store review/hardware equivalence | NOT APPLICABLE to GitHub publication / HUMAN NEEDED | No store submission; hardware/perceptual equivalence remains a separate listening claim |

## Artifact identity

| Artifact | Size | SHA-256 | Notarization |
| --- | ---: | --- | --- |
| `SendBloom-1.0.0-macOS.dmg` | 20,411,152 bytes | `779165a715772a97c175b7ddcb8d36fcb5acabcc0ec363a024dbb01d98bbc10e` | stapled-valid |
| `SendBloom-1.0.0.pkg` | 20,357,491 bytes | `39fa63bb00d23230c2c33ee567cdfc7857dcc510b1df70f1e538e9b97693f1cd` | stapled-valid |

The release contains `SHA256SUMS.txt`, `release-manifest.json`,
`provenance.json`, and `SendBloom-1.0.0-sbom.json`. The manifest binds the
artifacts to source commit `e61576572a0b99950a891261df9d9df61955089f`.

## Installed truth

The final public artifact was installed under administrator approval. Direct
binary identity comparison is:

| Component | Build SHA-256 | Installed SHA-256 |
| --- | --- | --- |
| VST3 | `7541f06d2c500d05a1d8ceb1ed59540e9963f124849d13db39585d5915f2dd0e` | `7541f06d2c500d05a1d8ceb1ed59540e9963f124849d13db39585d5915f2dd0e` |
| AU | `9096815f228690cb6b4226c49291359f5f1a02112155fea634958a96c59f1b46` | `9096815f228690cb6b4226c49291359f5f1a02112155fea634958a96c59f1b46` |

`verify-installed-macos.sh` reported `0 failure(s), 0 skipped`; the installed
AU validated as type `aumf`, subtype `SbLm`, manufacturer `NkMo`. A process
named `ableton-cubase-keys` was present and was reported by the generic live
process check; it is not treated as DAW proof.

## Reproducible verification commands

```text
RELEASE_MODE=public SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
  bash scripts/release/verify-hosted-release.sh v1.0.0

RELEASE_MODE=public SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
  bash scripts/release/verify-installed-macos.sh
```

The hosted verifier finished `PASS`; it deliberately kept its fresh download
directory for inspection. The final hosted check list included Linux/macOS/
Windows success and the optional signed-build warning above.

## Product and listening caveats

The Reverb-X public-reference catalog, objective engine measurements, and
structured interactive listening remain in the referenced evidence documents.
The current engine is the listening-preferred baseline. Those references are
qualitative/original-inspired direction, not a claim of hardware identity or a
null-tested transfer fit. Any final perceptual/hardware-equivalence statement
is `human_needed`.

## Post-publication hardening

- `bdb3fe4` makes the PKG a required hosted asset for the `pkg-in-dmg`
  contract; the exact PKG was uploaded and verified on `v1.0.0`.
- `e6fe996` makes the duplicate signed GitHub job explicitly opt-in via
  `ENABLE_SIGNED_RELEASE=true` and documents the local maintainer pipeline.
- The public tag remains immutable at `e615765...`; these hardening commits are
  on `main` for subsequent releases and do not rewrite the shipped bytes.

## Final claim

SendBloom 1.0.0 is publicly available and its hosted bytes, signatures,
notarization, package payload, required CI, and installed AU/VST3 identity are
verified. The release is `READY WITH CAVEATS`; `RELEASE VERIFIED` is not claimed
because optional CI, DAW-host, uninstall/rollback, store, and
hardware-equivalence evidence are intentionally kept separate.
