# SendBloom release-readiness report — 1.0.0 candidate

- Project: SendBloom
- Date: 2026-08-01 (Europe/Berlin)
- Scope: macOS AU/VST3 candidate, `pkg-in-dmg` contract
- Proof level: `CANDIDATE`
- Verdict: `NOT READY`
- Release verified: `NO`
- Version: `1.0.0`
- Source commit: `2bb6fd4f22834b2fb68772698bf7adbe04d5eb9b`
- Tag/channel: intended `v1.0.0`; local unsigned rehearsal only

## Decision

The SendBloom 1.0.0 code and candidate package layout pass the local release
gates, including a clean universal build, the full test suite, pluginval,
checksums, manifest, SBOM, and fresh-directory DMG inspection. This is not a
public-release proof: the rehearsal is explicitly ad-hoc signed, neither
notarized nor stapled, was not published, and was not installed for direct
AU/host identity checks.

The current engine is the restored baseline. The 2026 two-allpass density and
5x wet-dirt candidates are retained only as historical measurements; structured
interactive listening preferred the baseline in all four level-matched cells.

## Release Contract

| Item | Expected |
| --- | --- |
| Product/components | SendBloom AU component and VST3 bundle, packaged in a macOS installer DMG |
| Platforms/architectures | macOS 11+; universal `arm64` + `x86_64` |
| Distribution channels | Local candidate now; signed/notarized GitHub release intended |
| Public artifacts | Versioned DMG containing the versioned PKG; checksum, manifest, provenance, and SBOM alongside it |
| Install/deploy targets | `/Library/Audio/Plug-Ins/VST3` and `/Library/Audio/Plug-Ins/Components` |
| Upgrade/uninstall/rollback | Release scripts define the path; not run in this non-destructive candidate check |

## Gate Evidence

| Gate | Required | Status | Evidence | Notes |
| --- | --- | --- | --- | --- |
| Release contract | Yes | PASS | `docs/release.md`, `scripts/release/lib.sh` | AU/VST3 and `pkg-in-dmg` contract are explicit |
| Version/source identity | Yes | PASS | `VERSION=1.0.0`; `dist/1.0.0/build-environment.txt`; `release-manifest.json` | Build is clean and bound to source commit `2bb6fd4` |
| Tests and CI | Yes | PASS | CTest: 284 discovered, 283 passed, 1 capability skip; release-script tests 18/18; ENAB PASS; pluginval strictness 10 PASS | Local evidence only; hosted CI matrix was not rechecked here |
| Security and hygiene | Yes | PASS | tree-hygiene PASS; legal-metadata PASS; reference-claims PASS; SBOM PASS; `git diff --check` PASS | Local evidence only; public hosted/security status remains unverified |
| Clean release build | Yes | PASS | `scripts/release/build-macos.sh`; `arm64+x86_64`; build ID `1.0.0+2bb6fd4f2283` | Release output was cleared before build |
| Signing/attestation | As applicable | BLOCKED | Bundle inspection reports `Signature=adhoc`, `TeamIdentifier=not set` | Developer ID credentials were intentionally not used |
| Notarization/store/registry validation | As applicable | BLOCKED | `not-attempted-local-unsigned` in manifest | Public notarization/stapling was not attempted |
| Final artifact layout/payload | Yes | PASS | `validate-artifacts.sh` PASS in `dist/1.0.0` and a fresh temporary directory | DMG mounted; license/install notes, PKG, AU, and VST3 payloads found; versions and install paths match |
| Portable checksums and manifest | Yes | PASS | `SHA256SUMS.txt` verifies after relocation; manifest hashes match | Entries use artifact basenames |
| Upgrade/uninstall/rollback | As applicable | NOT RUN | No installation was performed | Requires a separate controlled install test |
| Tag and publication metadata | PUBLISHED | NOT RUN | No tag or upload performed | Candidate scope only |
| Fresh hosted download and revalidation | PUBLISHED | NOT RUN | No hosted artifact exists | Requires publication first |
| Installed/deployed identity | PUBLISHED, as applicable | NOT RUN | `SENDBLOOM_INSTALL_SMOKE` was unset | No installed AU/VST3 or `auval` claim |
| Post-publish CI/security status | PUBLISHED | NOT RUN | No publication | Requires hosted release |

## Artifact Identity

| Artifact | Size | SHA-256/digest | Signature/attestation | Hosted match |
| --- | ---: | --- | --- | --- |
| `SendBloom-1.0.0-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE.pkg` | 20,339,447 bytes | `9439cc83ceb86cd4ad97c415a9ef77118a973163e0be53491b0d51829ebfb3aa` | Local ad-hoc rehearsal; no public identity | N/A |
| `SendBloom-1.0.0-macOS-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE.dmg` | 20,383,169 bytes | `9c3a88203b2302bce1cc0af82ba137c3608f3acf763262db7c19252aef6afe6d` | Local ad-hoc rehearsal; not notarized/stapled | N/A |

Nested AU and VST3 executables are universal `x86_64 arm64`, report bundle ID
`com.nikoaudiolabs.sendbloom`, version `1.0.0`, and pass recursive local
`codesign --verify --deep --strict`; their signature is ad-hoc only.

## Installed or Deployed Truth

Not established. The rehearsal did not install either bundle, did not run
`auval` against the installed AU, and did not restart or inspect a DAW host.

## Failures and Caveats

- Public signing, notarization, stapling, and Gatekeeper assessment are blocked
  until the Developer ID and notary credentials are available.
- Nothing was published, so tag immutability, hosted bytes, release metadata,
  and post-publish status are unproven.
- Install smoke, installed identity, AU validation, and upgrade/rollback were
  intentionally not run.
- Windows/Linux CI, DAW host coverage, Developer ID/notarization, and hardware
  equivalence remain separate human or external-environment evidence. The
  four-cell interactive screen is complete and preferred the baseline in all
  four cells; it does not prove hardware equivalence.
- The generated DMG is labelled `LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE` and must not
  be shared as a release.

## Commands and Evidence Sources

- `RELEASE_MODE=local-unsigned SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg bash scripts/release/release-all.sh`
- `RELEASE_MODE=local-unsigned SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg bash scripts/release/validate-artifacts.sh <fresh-temporary-directory>`
- `ctest --test-dir build-release --output-on-failure -C Release`
- `docs/reports/sendbloom-reverb-x-public-reference-engine-evidence.md`
- `docs/reverb-x-public-reference-catalog.md`
- `docs/reports/data/sendbloom-engine-metrics-2026-07-30.json`
- `dist/1.0.0/release-report.md`, `build-environment.txt`, `SHA256SUMS.txt`,
  `release-manifest.json`, `provenance.json`, and the SBOM

## Final Claim

The candidate source commit proves a clean, tested, locally packaged SendBloom 1.0.0 candidate
whose exact DMG and PKG bytes validate in a fresh directory and whose current
engine is the listening-preferred baseline. It does not prove a signed,
notarized, published, installed, hosted, or hardware-equivalent release.
`RELEASE VERIFIED` is intentionally not claimed.
