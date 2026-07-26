#!/usr/bin/env bash
# Public-tree hygiene and secret scan.
#
# Everything this checks is about what the public repository and the release
# payload contain. It is fail-closed: a finding blocks the release.
#
#   1. No tracked OS/editor debris (.DS_Store, Thumbs.db, editor swap files).
#   2. No tracked credential material (.env, PEM keys, PKCS#12, certificate
#      requests, provisioning profiles, keychains, notary logs).
#   3. No tracked build output or local caches.
#   4. No credential-shaped strings in tracked text.
#   5. .gitignore actually covers the local-state directories this project
#      uses, so they cannot be committed by accident later.
#   6. No untracked, unignored files at release time — an unignored stray is
#      either missing from the release or about to be committed by accident.
#
# This scanner necessarily contains the very patterns it hunts for, so its own
# source is excluded from the content scan. That exclusion is explicit and is
# the only one.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

SELF_REL="scripts/release/check-tree-hygiene.sh"
findings=0

fail() { printf '%sFAIL%s %s\n' "$_c_red" "$_c_off" "$*" >&2; findings=$((findings + 1)); }

cd "$ROOT"

# ---------------------------------------------------------------------------
step "Tracked OS and editor debris"
# ---------------------------------------------------------------------------

debris="$(git ls-files | grep -E '(^|/)(\.DS_Store|Thumbs\.db|desktop\.ini|\._[^/]+|.*\.swp|.*~)$' || true)"
if [[ -n "$debris" ]]; then
  while IFS= read -r f; do fail "tracked OS/editor debris: $f"; done <<<"$debris"
else
  ok "no tracked OS or editor debris"
fi

# ---------------------------------------------------------------------------
step "Tracked credential material"
# ---------------------------------------------------------------------------

cred_paths="$(git ls-files | grep -iE '(^|/)(\.env(\..*)?|.*\.pem|.*\.p12|.*\.pfx|.*\.key|.*\.csr|.*\.certSigningRequest|.*\.mobileprovision|.*\.provisionprofile|.*\.keychain(-db)?|.*id_rsa.*|.*id_ed25519.*|notary.*\.(log|json)|.*\.jks|.*\.keystore)$' || true)"
if [[ -n "$cred_paths" ]]; then
  while IFS= read -r f; do fail "tracked credential-shaped file: $f"; done <<<"$cred_paths"
else
  ok "no tracked credential-shaped files"
fi

# ---------------------------------------------------------------------------
step "Tracked build output and local caches"
# ---------------------------------------------------------------------------

artifact_paths="$(git ls-files \
  | grep -vE '^(JUCE|cmake)/' \
  | grep -vE '(^|/)\.gitkeep$' \
  | grep -E '^(Builds|build|build-release|dist|artifacts|\.deslop|\.serena|\.claude|node_modules|__pycache__)/|(^|/)__pycache__/|\.(o|a|so|dylib|dSYM|pyc)$|(^|/)(Builds-[^/]+)/' || true)"
if [[ -n "$artifact_paths" ]]; then
  while IFS= read -r f; do fail "tracked build output or local cache: $f"; done <<<"$artifact_paths"
else
  ok "no tracked build output or local caches"
fi

large="$(git ls-files -z | xargs -0 -I{} sh -c 'test -f "{}" && printf "%s %s\n" "$(wc -c <"{}")" "{}"' 2>/dev/null \
  | awk '$1 > 5242880 {print}' | grep -vE ' (JUCE|cmake)/' || true)"
if [[ -n "$large" ]]; then
  while IFS= read -r l; do fail "tracked file over 5 MiB (should this be in the repo?): $l"; done <<<"$large"
else
  ok "no oversized tracked files outside submodules"
fi

# ---------------------------------------------------------------------------
step "Credential-shaped strings in tracked text"
# ---------------------------------------------------------------------------

# Patterns are assembled from fragments so this file does not trip its own
# scan when someone removes the self-exclusion below.
declare -a SECRET_PATTERNS=(
  'AKIA[0-9A-Z]{16}'
  'gh[pousr]_[A-Za-z0-9]{30,}'
  'github''_pat_[A-Za-z0-9_]{20,}'
  'xox[baprs]-[A-Za-z0-9-]{10,}'
  '-----BEGIN [A-Z ]*PRIVATE KEY-----'
  'ASC_API_KEY_[A-Za-z0-9]{8,}'
  '(app[_-]?specific[_-]?password|APP_SPECIFIC_PASSWORD)[[:space:]]*[=:][[:space:]]*[^$\{"'"'"'[:space:]][^[:space:]]*'
  # Apple app-specific password shape. Anchored on both sides so it cannot
  # match a substring of a longer hyphenated slug such as a phase directory
  # name ("...eder-tank-core-extr...").
  '(^|[^A-Za-z0-9-])[a-z]{4}-[a-z]{4}-[a-z]{4}-[a-z]{4}([^A-Za-z0-9-]|$)'
  '(AC_PASSWORD|NOTARY_PASSWORD|ALTOOL_PASSWORD)[[:space:]]*=[[:space:]]*[^$\{"'"'"'[:space:]][^[:space:]]*'
  '(password|passwd|secret|token|api[_-]?key)[[:space:]]*[=:][[:space:]]*["'"'"'][^"'"'"'$\{][^"'"'"']{7,}["'"'"']'
)

secret_hits=0
while IFS= read -r f; do
  [[ "$f" == "$SELF_REL" ]] && continue
  [[ -f "$f" ]] || continue
  # Skip binaries.
  if [[ -n "$(LC_ALL=C grep -lI '' "$f" 2>/dev/null)" ]]; then
    for pat in "${SECRET_PATTERNS[@]}"; do
      while IFS= read -r hit; do
        [[ -z "$hit" ]] && continue
        fail "credential-shaped string in $f:${hit%%:*}"
        secret_hits=$((secret_hits + 1))
      done < <(LC_ALL=C grep -nE "$pat" "$f" 2>/dev/null || true)
    done
  fi
done < <(git ls-files | grep -vE '^(JUCE|cmake)/')

if [[ "$secret_hits" -eq 0 ]]; then
  ok "no credential-shaped strings in tracked text"
fi

# ---------------------------------------------------------------------------
step ".gitignore covers local state"
# ---------------------------------------------------------------------------

declare -a MUST_IGNORE=(
  ".DS_Store"
  "dist/x"
  "build-release/x"
  "Builds/x"
  ".claude/x"
  ".serena/x"
  "artifacts/x"
  "repomix-output.md"
)

for p in "${MUST_IGNORE[@]}"; do
  if git check-ignore -q "$p"; then
    ok "ignored: $p"
  else
    fail ".gitignore does not cover '$p'; local state can be committed by accident"
  fi
done

# ---------------------------------------------------------------------------
step "Working tree is clean"
# ---------------------------------------------------------------------------

# ALLOW_DIRTY is honoured only so a maintainer can rehearse locally. A public
# release run never sets it (release-all.sh refuses).
ALLOW_DIRTY="${ALLOW_DIRTY:-0}"

modified="$(git status --porcelain --untracked-files=normal || true)"
if [[ -z "$modified" ]]; then
  ok "working tree clean, no untracked or unignored strays"
elif [[ "$ALLOW_DIRTY" == "1" ]]; then
  warn "working tree is DIRTY and ALLOW_DIRTY=1 — permitted for local rehearsal only"
  printf '%s\n' "$modified" | sed 's/^/      /' >&2
else
  fail "working tree is dirty; a release must build from a committed state"
  printf '%s\n' "$modified" | sed 's/^/      /' >&2
fi

# ---------------------------------------------------------------------------

echo
if [[ "$findings" -ne 0 ]]; then
  die "tree hygiene: $findings finding(s) — release blocked"
fi
printf '%stree hygiene: PASS%s\n' "$_c_grn" "$_c_off"
