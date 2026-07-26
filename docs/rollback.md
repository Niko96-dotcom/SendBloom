# Rollback plan

What to do when a published SendBloom release turns out to be wrong.

The governing rule: **never change what a published tag or asset points at.**
Someone has already downloaded it, and a silent replacement means two different
binaries share one version number forever. Roll forward to a new version, or
withdraw the release outright. Both are covered below.

## Decide which case you are in

| Situation | Action |
|---|---|
| Bad artifact, product itself is fine (wrong file uploaded, corrupt asset, missing asset) | **Withdraw and re-release** as a new patch version |
| Product defect that ships wrong audio, crashes, or damages sessions | **Withdraw immediately**, then fix and release a new patch version |
| Signing or notarization problem discovered after publishing | **Withdraw immediately** — users will hit Gatekeeper failures |
| Cosmetic issue in release notes or docs | Edit the release body and the repository. Do **not** touch the assets or the tag |
| Wrong tag target (tag points at the wrong commit) | **Withdraw**, delete the tag, re-release as a new version. Never move the tag |

## Withdraw a published release

This makes the assets unavailable while preserving the history of what
happened. Do it as soon as you are confident the release is bad; a slow
withdrawal costs more than a hasty one.

```bash
# 1. Record what was published, before removing anything.
gh release view v<bad-version> --json tagName,publishedAt,assets > withdrawn-v<bad-version>.json

# 2. Take the assets out of circulation.
gh release delete v<bad-version> --yes
```

Keep the tag in place if it points at the right commit — it is a truthful
record of what was built. Delete the tag only when it points somewhere wrong:

```bash
git push origin :refs/tags/v<bad-version>
git tag -d v<bad-version>
```

Then say so publicly. Add a note to `CHANGELOG.md` under the withdrawn
version explaining what was wrong and what users should do, and open an issue
so the reason is searchable.

## Roll forward

1. Fix the defect on `main`.
2. Bump `VERSION` to the next patch — never reuse the withdrawn number, even if
   nobody downloaded it. A number that was ever public is spent.
3. Add a `CHANGELOG.md` entry that names the withdrawn version and says what
   was wrong with it.
4. Rewrite `RELEASE_NOTES.md` for the new version, including a short "if you
   installed `<bad-version>`" paragraph.
5. Add the withdrawn version to `.release/version-allowlist.txt` with that
   reason, so the changelog and notes may keep naming it.
6. Run the full pipeline:

   ```bash
   RELEASE_MODE=public scripts/release/release-all.sh --publish
   scripts/release/verify-hosted-release.sh v<new-version>
   ```

## Valid rollback targets

A user who needs to go back can install any earlier release that is still
published. Right now that set is:

- **1.0.0** — the first public release. There is nothing published before it,
  so there is no earlier version to fall back to. `1.0.0-rc0` was never
  published as a hosted release; its tag points at a superseded commit and it
  must not be offered to anyone as a rollback target.

Update this list whenever a release is published or withdrawn.

## Telling users to downgrade

Downgrading is the same as upgrading, in reverse: install the older release
over the newer one and restart the host. See
[docs/install.md](install.md#upgrading).

Two things to say explicitly whenever you ask users to downgrade:

- Sessions saved with the newer build will still open, because plug-in state is
  read forward and unknown parameters are discarded — but any parameter that
  only exists in the newer build will fall back to its default.
- They should verify what they end up with:

  ```bash
  plutil -extract CFBundleShortVersionString raw -o - \
    /Library/Audio/Plug-Ins/VST3/SendBloom.vst3/Contents/Info.plist
  ```

## If the mistake is caught before anyone downloads

There is no safe way to know that. Treat every published release as
downloaded and follow the withdraw-and-roll-forward path above.

## Local rehearsal artifacts

Artifacts from `RELEASE_MODE=local-unsigned` are named
`…-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE`, are ad-hoc signed and unnotarized, and
carry a `DO-NOT-DISTRIBUTE.txt` inside the disk image. If one ever escapes,
there is nothing to roll back — it was never a release. Delete it, and tell
anyone who received it to remove it and install a real release, because
Gatekeeper will refuse it on their machine anyway.
