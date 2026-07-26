#!/usr/bin/env bash
# Emit the release manifest and the provenance record.
#
# The manifest is the single answer to "what exactly is this release?" —
# artifact names, sizes, hashes, version, commit, tag, build id, signing
# identity (name only, never a secret), notarization submission ids, and the
# validation status of each artifact. Everything downstream (validation,
# hosted revalidation, the final report) compares against this file rather
# than recomputing its own idea of the truth.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init
require_cmd shasum python3

[[ -d "$DIST_DIR" ]] || die "no dist directory: $DIST_DIR"

step "Release manifest for $RELEASE_VERSION"

ARTIFACTS=()
while IFS= read -r _a; do
  [[ -n "$_a" ]] && ARTIFACTS+=("$_a")
done < <(
  cd "$DIST_DIR" && find . -maxdepth 1 -type f \
    \( -name '*.dmg' -o -name '*.pkg' \) \
    -exec basename {} \; | sort
)
[[ ${#ARTIFACTS[@]} -gt 0 ]] || die "no distributable artifacts found in $DIST_DIR"

signing_identity_for() {
  # Report the identity NAME recorded in the signature. Never a credential.
  local path="$1"
  if [[ "$path" == *.pkg ]] && command -v pkgutil >/dev/null 2>&1; then
    pkgutil --check-signature "$path" 2>/dev/null \
      | sed -nE 's/^[[:space:]]*1\. (Developer ID Installer:.*)$/\1/p' | head -n1
  elif command -v codesign >/dev/null 2>&1; then
    codesign --display --verbose=4 "$path" 2>&1 \
      | sed -nE 's/^Authority=(Developer ID Application:.*)$/\1/p' | head -n1
  fi
}

staple_status_for() {
  local path="$1"
  if [[ "$RELEASE_MODE" != "public" ]]; then
    printf 'not-attempted-local-unsigned'
    return
  fi
  if xcrun stapler validate "$path" >/dev/null 2>&1; then
    printf 'stapled-valid'
  else
    printf 'NOT-STAPLED'
  fi
}

notary_submission_for() {
  local base="$1"
  [[ -f "$DIST_DIR/notarization.txt" ]] || return 0
  awk -v want="$base" '
    /^artifact=/      { a=substr($0,10) }
    /^submission_id=/ { if (a == want) print substr($0,15) }
  ' "$DIST_DIR/notarization.txt" | tail -n1
}

# Build the JSON with python3 so every value is correctly escaped.
{
  printf 'schema\thttps://sendbloom.dev/release-manifest/v1\n'
  printf 'product\tSendBloom\n'
  printf 'release_version\t%s\n' "$RELEASE_VERSION"
  printf 'version_core\t%s\n' "$VERSION_CORE"
  printf 'prerelease\t%s\n' "${VERSION_PRERELEASE:-}"
  printf 'tag\t%s\n' "$RELEASE_TAG"
  printf 'build_id\t%s\n' "$BUILD_ID"
  printf 'source_commit\t%s\n' "$SOURCE_COMMIT"
  printf 'source_dirty\t%s\n' "$SOURCE_DIRTY"
  printf 'release_mode\t%s\n' "$RELEASE_MODE"
  printf 'artifact_contract\t%s\n' "$ARTIFACT_CONTRACT"
  printf 'primary_artifact\t%s\n' "$(basename "$(primary_artifact_path)")"
  printf 'generated_at\t%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
  printf 'ci_run\t%s\n' "${GITHUB_RUN_ID:+${GITHUB_SERVER_URL:-https://github.com}/${GITHUB_REPOSITORY:-}/actions/runs/$GITHUB_RUN_ID}"
  for a in "${ARTIFACTS[@]}"; do
    p="$DIST_DIR/$a"
    printf 'artifact\t%s\t%s\t%s\t%s\t%s\t%s\n' \
      "$a" \
      "$(sha256_of "$p")" \
      "$(wc -c <"$p" | tr -d ' ')" \
      "$(signing_identity_for "$p")" \
      "$(staple_status_for "$p")" \
      "$(notary_submission_for "$a")"
  done
} | python3 -c '
import json, sys

meta = {}
artifacts = []
for raw in sys.stdin.read().splitlines():
    if not raw:
        continue
    parts = raw.split("\t")
    if parts[0] == "artifact":
        _, name, sha, size, identity, staple, submission = (parts + [""] * 7)[:7]
        artifacts.append({
            "name": name,
            "sha256": sha,
            "size_bytes": int(size) if size else None,
            "signing_identity": identity or None,
            "notarization": {
                "submission_id": submission or None,
                "staple": staple,
            },
        })
    else:
        meta[parts[0]] = parts[1] if len(parts) > 1 and parts[1] != "" else None

meta["source_dirty"] = meta.get("source_dirty") == "1"
meta["artifacts"] = artifacts
json.dump(meta, sys.stdout, indent=2, sort_keys=False)
sys.stdout.write("\n")
' >"$MANIFEST_PATH"

python3 -c 'import json,sys; json.load(open(sys.argv[1]))' "$MANIFEST_PATH" \
  || die "generated manifest is not valid JSON"

sed 's/^/      /' "$MANIFEST_PATH"
ok "wrote $MANIFEST_PATH"

# ---------------------------------------------------------------------------
# Provenance
# ---------------------------------------------------------------------------

step "Provenance"

python3 - "$PROVENANCE_PATH" "$MANIFEST_PATH" <<'PY'
import json, os, subprocess, sys

out_path, manifest_path = sys.argv[1], sys.argv[2]
manifest = json.load(open(manifest_path))

def sh(*cmd):
    try:
        return subprocess.check_output(cmd, text=True, stderr=subprocess.DEVNULL).strip()
    except Exception:
        return None

root = os.environ["ROOT"]

provenance = {
    "schema": "https://sendbloom.dev/release-provenance/v1",
    "product": "SendBloom",
    "release_version": manifest["release_version"],
    "tag": manifest["tag"],
    "build_id": manifest["build_id"],
    "source": {
        "repository": sh("git", "-C", root, "config", "--get", "remote.origin.url"),
        "commit": manifest["source_commit"],
        "commit_authored_at": sh("git", "-C", root, "show", "-s", "--format=%cI", "HEAD"),
        "dirty": manifest["source_dirty"],
        "submodules": [
            line.strip() for line in (sh("git", "-C", root, "submodule", "status") or "").splitlines()
        ],
    },
    "build": {
        "mode": manifest["release_mode"],
        "artifact_contract": manifest["artifact_contract"],
        "packaging_command": "scripts/release/release-all.sh",
        "environment": dict(
            line.split("=", 1)
            for line in open(os.path.join(os.path.dirname(out_path), "build-environment.txt"))
            .read().splitlines()
            if "=" in line
        ) if os.path.exists(os.path.join(os.path.dirname(out_path), "build-environment.txt")) else {},
    },
    "ci_run": manifest.get("ci_run"),
    "generated_at": manifest["generated_at"],
    "artifacts": manifest["artifacts"],
}

with open(out_path, "w") as fh:
    json.dump(provenance, fh, indent=2)
    fh.write("\n")
print(f"      wrote {out_path}")
PY

ok "wrote $PROVENANCE_PATH"

echo
printf '%smanifest: PASS%s  %s\n' "$_c_grn" "$_c_off" "$MANIFEST_PATH"
