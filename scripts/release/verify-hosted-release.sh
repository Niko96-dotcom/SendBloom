#!/usr/bin/env bash
# Hosted artifact truth.
#
#   scripts/release/verify-hosted-release.sh [tag]
#
# Local release success proves nothing about what the world can download. This
# script downloads the published GitHub Release assets into a FRESH temporary
# directory and re-runs the full artifact validation against them, plus the
# checks that only make sense against the hosted release:
#
#   * the release points at the intended tag, and the tag points at the
#     intended commit
#   * exactly the expected assets are attached — no missing ones and no
#     leftovers from an older release
#   * the downloaded bytes match the manifest that the build produced
#   * CI is green on the released commit
#
# Requires only a read token; it publishes nothing.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init
require_cmd gh git shasum python3

TAG="${1:-$RELEASE_TAG}"
failures=0
fail() { printf '%sFAIL%s %s\n' "$_c_red" "$_c_off" "$*" >&2; failures=$((failures + 1)); }

step "Hosted release verification — $TAG"

gh auth status >/dev/null 2>&1 || die "gh is not authenticated; cannot read the hosted release"

REPO="$(gh repo view --json nameWithOwner -q .nameWithOwner)"
info "repository: $REPO"

# ---------------------------------------------------------------------------
step "Release exists and points at the intended commit"
# ---------------------------------------------------------------------------

if ! gh release view "$TAG" --json tagName >/dev/null 2>&1; then
  die "no hosted release for tag $TAG"
fi

RELEASE_JSON="$(mktemp -t sendbloom-release.XXXXXX.json)"
trap 'rm -f "$RELEASE_JSON"' EXIT
gh release view "$TAG" --json tagName,isDraft,isPrerelease,targetCommitish,publishedAt,assets >"$RELEASE_JSON"

is_draft="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["isDraft"])' "$RELEASE_JSON")"
is_pre="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["isPrerelease"])' "$RELEASE_JSON")"
info "draft=$is_draft prerelease=$is_pre"

[[ "$is_draft" == "False" ]] || fail "release $TAG is still a draft; nothing is actually published"

if [[ "$RELEASE_IS_PRERELEASE" == "1" && "$is_pre" != "True" ]]; then
  fail "$RELEASE_VERSION is a prerelease but the hosted release is not marked as one"
fi
if [[ "$RELEASE_IS_PRERELEASE" == "0" && "$is_pre" == "True" ]]; then
  fail "$RELEASE_VERSION is a stable release but the hosted release is marked prerelease"
fi

git fetch --tags --force origin >/dev/null 2>&1 || warn "could not fetch tags from origin"
remote_tag_commit="$(git rev-list -n1 "$TAG" 2>/dev/null || true)"
if [[ -z "$remote_tag_commit" ]]; then
  fail "tag $TAG is not resolvable locally"
else
  info "tag $TAG -> $remote_tag_commit"
  if [[ -f "$MANIFEST_PATH" ]]; then
    manifest_commit="$(python3 -c 'import json,sys; print(json.load(open(sys.argv[1]))["source_commit"])' "$MANIFEST_PATH")"
    [[ "$remote_tag_commit" == "$manifest_commit" ]] \
      && ok "tag points at the commit the artifacts were built from" \
      || fail "tag $TAG points at $remote_tag_commit but the artifacts were built from $manifest_commit"
  else
    warn "no local manifest to compare the tag against"
  fi
fi

# ---------------------------------------------------------------------------
step "Attached assets are exactly the expected set"
# ---------------------------------------------------------------------------

HOSTED_ASSETS=()
while IFS= read -r _a; do
  [[ -n "$_a" ]] && HOSTED_ASSETS+=("$_a")
done < <(python3 -c '
import json, sys
for a in json.load(open(sys.argv[1]))["assets"]:
    print(a["name"])
' "$RELEASE_JSON" | sort)

EXPECTED_ASSETS=("$DMG_NAME" "$CHECKSUM_NAME" "$MANIFEST_NAME" "$PROVENANCE_NAME" "$SBOM_NAME")
if [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]]; then
  EXPECTED_ASSETS+=("$PKG_NAME")
fi

info "hosted assets:"
if [[ ${#HOSTED_ASSETS[@]} -eq 0 ]]; then
  fail "the hosted release has no assets attached at all"
else
  printf '      %s\n' "${HOSTED_ASSETS[@]}"
fi

for e in "${EXPECTED_ASSETS[@]}"; do
  if printf '%s\n' ${HOSTED_ASSETS[@]+"${HOSTED_ASSETS[@]}"} | grep -Fxq "$e"; then
    ok "attached: $e"
  else
    fail "expected asset is not attached to $TAG: $e"
  fi
done

for h in ${HOSTED_ASSETS[@]+"${HOSTED_ASSETS[@]}"}; do
  if printf '%s\n' "${EXPECTED_ASSETS[@]}" | grep -Fxq "$h"; then
    continue
  fi
  if [[ "$h" =~ [0-9]+\.[0-9]+\.[0-9]+ && "$h" != *"$RELEASE_VERSION"* ]]; then
    fail "obsolete asset from another release is still attached: $h"
  else
    warn "unexpected extra asset attached: $h"
  fi
done

# ---------------------------------------------------------------------------
step "Download into a fresh directory"
# ---------------------------------------------------------------------------

FRESH_DIR="$(mktemp -d -t sendbloom-hosted.XXXXXX)"
info "download directory: $FRESH_DIR"
info "(deliberately empty and unrelated to the build tree)"

gh release download "$TAG" --dir "$FRESH_DIR" --clobber \
  || die "could not download the hosted assets for $TAG"

ls -la "$FRESH_DIR" | sed 's/^/      /'
ok "downloaded $(find "$FRESH_DIR" -maxdepth 1 -type f | wc -l | tr -d ' ') file(s)"

# ---------------------------------------------------------------------------
step "Downloaded bytes match the locally built manifest"
# ---------------------------------------------------------------------------

if [[ -f "$MANIFEST_PATH" ]]; then
  compared=0
  while IFS=$'\t' read -r name sha; do
    [[ -n "$name" ]] || continue
    if [[ ! -f "$FRESH_DIR/$name" ]]; then
      fail "$name was not downloaded"
      continue
    fi
    actual="$(sha256_of "$FRESH_DIR/$name")"
    compared=$((compared + 1))
    [[ "$actual" == "$sha" ]] \
      && ok "$name is byte-identical to the artifact that was built and tested" \
      || fail "$name differs from what was built: built=$sha hosted=$actual"
  done < <(python3 -c '
import json, sys
for a in json.load(open(sys.argv[1]))["artifacts"]:
    print(a["name"] + "\t" + a["sha256"])
' "$MANIFEST_PATH")
  [[ "$compared" -gt 0 ]] || fail "the manifest listed no artifacts; the hosted bytes were never compared"
else
  warn "no local manifest — hosted bytes cannot be compared with the tested build"
fi

# ---------------------------------------------------------------------------
step "Re-run full artifact validation against the downloaded files"
# ---------------------------------------------------------------------------

if bash "$(dirname "${BASH_SOURCE[0]}")/validate-artifacts.sh" "$FRESH_DIR"; then
  ok "hosted artifacts pass the same validation as the local build"
else
  fail "hosted artifacts FAILED validation"
fi

# ---------------------------------------------------------------------------
step "CI status on the released commit"
# ---------------------------------------------------------------------------

if [[ -n "${remote_tag_commit:-}" ]]; then
  ci_out="$(gh api "repos/$REPO/commits/$remote_tag_commit/check-runs" \
    --jq '.check_runs[] | "\(.name)\t\(.status)\t\(.conclusion)"' 2>/dev/null || true)"
  if [[ -z "$ci_out" ]]; then
    warn "no check runs reported for $remote_tag_commit — CI status is UNKNOWN, not green"
  else
    printf '%s\n' "$ci_out" | sed 's/^/      /'
    if printf '%s\n' "$ci_out" | awk -F'\t' '$3 != "success" && $3 != "skipped" && $3 != "neutral" {found=1} END{exit !found}'; then
      fail "at least one check run on $remote_tag_commit is not successful"
    else
      ok "all check runs on the released commit are successful"
    fi
  fi
fi

echo
info "fresh download kept for inspection: $FRESH_DIR"
echo
if [[ "$failures" -ne 0 ]]; then
  die "hosted release verification: $failures failure(s) for $TAG"
fi
printf '%sverify-hosted-release: PASS%s  %s\n' "$_c_grn" "$_c_off" "$TAG"
