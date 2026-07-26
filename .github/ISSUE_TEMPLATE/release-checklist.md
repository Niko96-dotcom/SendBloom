---
name: Release checklist
about: Track a SendBloom release through every required gate
title: "Release: v<version>"
labels: release
---

## Release

- **Version:** `<version>` (must equal the `VERSION` file)
- **Tag:** `v<version>`
- **Commit:**
- **Artifact contract:** `pkg-in-dmg` / `bundles-in-dmg`
- **Release manager:**

Full procedure: [docs/release.md](../../docs/release.md).
Gate detail: [docs/release-validation.md](../../docs/release-validation.md).

`scripts/release/release-all.sh` runs every automated item below and writes
`dist/<version>/release-report.md`. Tick a box only against evidence — a run
you actually saw, not a step you believe would pass.

## Minimum release checklist

- [ ] 1. Working tree clean, or a documented release-branch state
- [ ] 2. Version bumped once, at `VERSION` only
- [ ] 3. Stale-version search passes (`check-version-consistency.sh`)
- [ ] 4. Changelog and release notes match the version and the artifact names
- [ ] 5. Dependency, licence and security checks pass (`sbom.sh`, secret scan)
- [ ] 6. Unit, integration and product smoke tests pass (ctest, pluginval, auval)
- [ ] 7. Release-script regression tests pass (`tests/release/run-release-tests.sh`)
- [ ] 8. Public-tree hygiene passes (`check-tree-hygiene.sh`)
- [ ] 9. Clean release build from scratch succeeds (`build-macos.sh`)
- [ ] 10. Public artifacts are Developer ID signed with hardened runtime
- [ ] 11. Notarization succeeded for every submitted artifact
- [ ] 12. Every artifact is stapled and `stapler validate` passes
- [ ] 13. Checksums generated **after** stapling, basenames only
- [ ] 14. Artifact layout and installer payload validated from the shipped container
- [ ] 15. Install smoke proves the exact artifact installs and runs
- [ ] 16. Upgrade and uninstall checked (when relevant)
- [ ] 17. Tag points at the intended commit
- [ ] 18. Publish created exactly the expected hosted assets
- [ ] 19. Hosted assets downloaded to a fresh directory and revalidated
- [ ] 20. CI green on the released commit
- [ ] 21. Installed build ID matches the release
- [ ] 22. Final release report attached below

## Skipped checks

List every check that was **not** run, and why. A skipped check is not a
passed check; anything here must also appear in the release notes caveats.

| Check | Why it was skipped | Residual risk |
|---|---|---|
| | | |

## Release report

<details>
<summary>dist/&lt;version&gt;/release-report.md</summary>

```
paste here
```

</details>

## Readiness

- [ ] **READY** — every required gate passed
- [ ] **READY WITH CAVEATS** — public artifact checks passed; the skipped checks above are named
- [ ] **NOT READY** — a required gate failed
