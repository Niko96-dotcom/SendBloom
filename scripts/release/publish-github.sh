#!/usr/bin/env bash
# Create the tag and the GitHub Release.
#
#   scripts/release/publish-github.sh              # dry run, changes nothing
#   scripts/release/publish-github.sh --publish    # actually publishes
#
# Publication is the one irreversible, outward-facing step in this pipeline, so
# it is opt-in and never the default. A dry run prints the exact tag, commit,
# asset list and release body that --publish would create.
#
# Refuses outright when:
#   * RELEASE_MODE is not public (rehearsal artifacts must never be hosted)
#   * the working tree is dirty
#   * the artifacts are not signed, notarized, stapled and validated
#   * the tag already exists somewhere other than the intended commit
#   * a release already exists for the tag (use the documented re-release
#     procedure in docs/rollback.md instead of silently overwriting)

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init
require_cmd gh git python3

PUBLISH=0
for arg in "$@"; do
  case "$arg" in
    --publish) PUBLISH=1 ;;
    --dry-run) PUBLISH=0 ;;
    *) die "unknown argument: $arg" ;;
  esac
done

ASSETS=("$DMG_PATH" "$CHECKSUM_PATH" "$MANIFEST_PATH" "$PROVENANCE_PATH" "$SBOM_PATH")
# The pkg-in-dmg contract ships the DMG as the primary user download, but the
# manifest and checksum file also describe the signed installer. Attach that
# exact PKG so a fresh hosted download can validate every listed artifact.
if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  ASSETS+=("$PKG_PATH")
fi

step "Publish preflight — $RELEASE_TAG"

# --- mode ------------------------------------------------------------------
if [[ "$RELEASE_MODE" != "public" ]]; then
  die "RELEASE_MODE=$RELEASE_MODE cannot publish. Local rehearsal artifacts are ad-hoc signed and unnotarized; hosting them would ship an unusable, untrusted build."
fi
ok "release mode is public"

# --- source state ----------------------------------------------------------
[[ "$SOURCE_DIRTY" == "0" ]] || die "working tree is dirty; refusing to publish a build whose source is not committed"
ok "working tree clean at $SOURCE_COMMIT"

# --- assets ----------------------------------------------------------------
for a in "${ASSETS[@]}"; do
  [[ -f "$a" ]] || die "missing release asset: $a"
done
ok "${#ASSETS[@]} assets present"

# --- artifacts are finalised ----------------------------------------------
require_macos
for a in "$DMG_PATH"; do
  xcrun stapler validate "$a" >/dev/null 2>&1 \
    || die "$(basename "$a") has no valid stapled notarization ticket; it must not be published"
done
ok "artifacts are notarized and stapled"

# --- manifest agrees with what we are about to upload ----------------------
manifest_version="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["release_version"])' "$MANIFEST_PATH")"
manifest_commit="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source_commit"])' "$MANIFEST_PATH")"
[[ "$manifest_version" == "$RELEASE_VERSION" ]] || die "manifest is for $manifest_version, not $RELEASE_VERSION"
[[ "$manifest_commit" == "$SOURCE_COMMIT" ]] || die "manifest was built from $manifest_commit but HEAD is $SOURCE_COMMIT"
ok "manifest matches version $RELEASE_VERSION at $SOURCE_COMMIT"

compared=0
while IFS=$'\t' read -r name sha; do
  [[ -n "$name" ]] || continue
  [[ -f "$DIST_DIR/$name" ]] || die "manifest lists $name but it is not in $DIST_DIR"
  actual="$(sha256_of "$DIST_DIR/$name")"
  [[ "$actual" == "$sha" ]] || die "$name changed since the manifest was written (manifest=$sha actual=$actual)"
  compared=$((compared + 1))
done < <(python3 -c '
import json, sys
for a in json.load(open(sys.argv[1]))["artifacts"]:
    print(a["name"] + "\t" + a["sha256"])
' "$MANIFEST_PATH")
# A loop that compared nothing is not a passing comparison. Say so loudly:
# a silently-empty check is how an unverified artifact gets published.
[[ "$compared" -gt 0 ]] || die "the manifest listed no artifacts to compare; refusing to publish an unverified release"
ok "artifact hashes still match the manifest ($compared compared)"

# --- tag state -------------------------------------------------------------
gh auth status >/dev/null 2>&1 || die "gh is not authenticated"
REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"

existing_local="$(git -C "$ROOT" rev-list -n1 "$RELEASE_TAG" 2>/dev/null || true)"
if [[ -n "$existing_local" && "$existing_local" != "$SOURCE_COMMIT" ]]; then
  die "tag $RELEASE_TAG already exists locally and points at $existing_local, not $SOURCE_COMMIT.
       Retagging a published version silently changes what users get. See docs/rollback.md."
fi

if ! git -C "$ROOT" remote get-url origin >/dev/null 2>&1; then
  die "no 'origin' remote is configured; there is nowhere to publish to"
fi
existing_remote="$( { git -C "$ROOT" ls-remote --tags origin "refs/tags/$RELEASE_TAG^{}" 2>/dev/null || true; } | awk '{print $1}')"
if [[ -n "$existing_remote" && "$existing_remote" != "$SOURCE_COMMIT" ]]; then
  die "remote tag $RELEASE_TAG points at $existing_remote, not $SOURCE_COMMIT. See docs/rollback.md."
fi

if gh release view "$RELEASE_TAG" >/dev/null 2>&1; then
  die "a GitHub Release already exists for $RELEASE_TAG. Publishing again would replace what users already downloaded. See docs/rollback.md."
fi
ok "tag and release name are free (or already point at $SOURCE_COMMIT)"

# --- release body ----------------------------------------------------------
BODY_FILE="$DIST_DIR/release-body.md"
{
  cat "$ROOT/RELEASE_NOTES.md"
  cat <<EOF

---

## Verify this download

\`\`\`bash
shasum -a 256 -c $CHECKSUM_NAME
\`\`\`

| | |
|---|---|
| Version | \`$RELEASE_VERSION\` |
| Tag | \`$RELEASE_TAG\` |
| Commit | \`$SOURCE_COMMIT\` |
| Build ID | \`$BUILD_ID\` |
| Artifact contract | \`$ARTIFACT_CONTRACT\` |

\`$MANIFEST_NAME\` and \`$PROVENANCE_NAME\` record the hash, signing identity
and notarization submission of every asset. \`$SBOM_NAME\` is the dependency
and licence inventory.
EOF
} >"$BODY_FILE"

PRERELEASE_FLAG=()
[[ "$RELEASE_IS_PRERELEASE" == "1" ]] && PRERELEASE_FLAG=(--prerelease)

# ---------------------------------------------------------------------------
step "Plan"
# ---------------------------------------------------------------------------

cat <<EOF
      repository:   $REPO
      tag:          $RELEASE_TAG
      commit:       $SOURCE_COMMIT
      title:        SendBloom $RELEASE_VERSION
      prerelease:   $RELEASE_IS_PRERELEASE
      assets:
$(for _a in "${ASSETS[@]}"; do printf '        %s\n' "$(basename "$_a")"; done)
      body:         $BODY_FILE
EOF

if [[ "$PUBLISH" != "1" ]]; then
  echo
  warn "DRY RUN — nothing was tagged, uploaded or published."
  warn "Re-run with --publish to create the tag and the GitHub Release."
  echo
  printf '%spublish-github: DRY RUN OK%s\n' "$_c_yel" "$_c_off"
  exit 0
fi

# ---------------------------------------------------------------------------
step "Publishing"
# ---------------------------------------------------------------------------

if [[ -z "$existing_local" ]]; then
  git -C "$ROOT" tag -a "$RELEASE_TAG" -m "SendBloom $RELEASE_VERSION" "$SOURCE_COMMIT"
  ok "created tag $RELEASE_TAG at $SOURCE_COMMIT"
else
  ok "tag $RELEASE_TAG already at $SOURCE_COMMIT"
fi

git -C "$ROOT" push origin "refs/tags/$RELEASE_TAG"
ok "pushed tag"

gh release create "$RELEASE_TAG" \
  --title "SendBloom $RELEASE_VERSION" \
  --notes-file "$BODY_FILE" \
  --target "$SOURCE_COMMIT" \
  ${PRERELEASE_FLAG[@]+"${PRERELEASE_FLAG[@]}"} \
  "${ASSETS[@]}"
ok "created the GitHub Release with ${#ASSETS[@]} assets"

echo
warn "Local success is not hosted truth. Run:"
warn "  scripts/release/verify-hosted-release.sh $RELEASE_TAG"
echo
printf '%spublish-github: PUBLISHED%s %s\n' "$_c_grn" "$_c_off" "$RELEASE_TAG"
