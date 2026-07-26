#!/usr/bin/env bash
# Release-version verification gate.
#
# Fails when any product-facing release number is stale, hard-coded where it
# should be derived, or inconsistent with the canonical VERSION file.
#
# Checks
#   1. VERSION parses as MAJOR.MINOR.PATCH[-prerelease].
#   2. The build derives its version from VERSION (no literal in CMakeLists.txt
#      or in cmake-local/SendBloomVersion.cmake).
#   3. No SendBloom-scoped version literal in the scanned tree disagrees with
#      the canonical version, unless allowlisted with a reason.
#   4. The previous released version does not appear outside the allowlist.
#   5. RELEASE_NOTES.md title matches the canonical version.
#   6. CHANGELOG.md has an entry for the canonical version, at the top.
#   7. Built bundle Info.plist versions match the canonical numeric core
#      (skipped, loudly, when no build is present).
#   8. Every allowlist entry carries a reason.
#
# Exit 0 = release may proceed on version grounds. Any other exit blocks.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init

ALLOWLIST="${ALLOWLIST:-$ROOT/.release/version-allowlist.txt}"
failures=0

fail() { printf '%sFAIL%s %s\n' "$_c_red" "$_c_off" "$*" >&2; failures=$((failures + 1)); }

step "Release version consistency — canonical VERSION = $RELEASE_VERSION"
info "core=$VERSION_CORE prerelease=${VERSION_PRERELEASE:-<none>} tag=$RELEASE_TAG build_id=$BUILD_ID"

# ---------------------------------------------------------------------------
# 0. Allowlist parse — every entry must have a reason
# ---------------------------------------------------------------------------

declare -a AL_GLOB=() AL_LITERAL=()

if [[ -f "$ALLOWLIST" ]]; then
  lineno=0
  while IFS= read -r line || [[ -n "$line" ]]; do
    lineno=$((lineno + 1))
    [[ -z "${line//[[:space:]]/}" ]] && continue
    [[ "${line#"${line%%[![:space:]]*}"}" == \#* ]] && continue

    IFS='|' read -r g_glob g_literal g_reason <<<"$line"
    g_glob="$(printf '%s' "${g_glob:-}" | xargs)"
    g_literal="$(printf '%s' "${g_literal:-}" | xargs)"
    g_reason="$(printf '%s' "${g_reason:-}" | xargs)"

    if [[ -z "$g_glob" || -z "$g_literal" ]]; then
      fail "$ALLOWLIST:$lineno malformed entry (need '<glob> | <literal|*> | <reason>')"
      continue
    fi
    if [[ -z "$g_reason" ]]; then
      fail "$ALLOWLIST:$lineno allowlist entry for '$g_glob' has no reason. Every exemption must explain itself."
      continue
    fi

    AL_GLOB+=("$g_glob")
    AL_LITERAL+=("$g_literal")
  done <"$ALLOWLIST"
  ok "allowlist parsed: ${#AL_GLOB[@]} entries, all with reasons"
else
  warn "no allowlist at $ALLOWLIST — every historical version literal will block the release"
fi

is_allowlisted() {
  local path="$1" literal="$2" i
  for i in "${!AL_GLOB[@]}"; do
    # shellcheck disable=SC2053  # intentional glob match
    if [[ "$path" == ${AL_GLOB[$i]} ]]; then
      if [[ "${AL_LITERAL[$i]}" == "*" || "${AL_LITERAL[$i]}" == "$literal" ]]; then
        return 0
      fi
    fi
  done
  return 1
}

# ---------------------------------------------------------------------------
# 1. Scan scope — tracked, product-facing files only
# ---------------------------------------------------------------------------

scanned_files() {
  git -C "$ROOT" ls-files -- \
    'README.md' 'RELEASE_NOTES.md' 'CHANGELOG.md' 'CLAIM_STATUS.md' \
    'CMakeLists.txt' 'VERSION' \
    'cmake-local/*' 'docs/*' 'source/*' 'resources/*' 'scripts/*' \
    '.github/*' 'tests/*' '.release/*' \
    2>/dev/null \
  | grep -vE '^(JUCE|cmake)/' \
  | while IFS= read -r f; do
      [[ -f "$ROOT/$f" ]] || continue
      case "$f" in
        *.png|*.jpg|*.jpeg|*.zip|*.wav|*.aif|*.aiff|*.raw|*.bin|*.dmg|*.pkg) continue ;;
      esac
      # The allowlist names versions in order to exempt them; scanning it
      # would make every exemption its own violation.
      [[ "$ROOT/$f" -ef "$ALLOWLIST" ]] && continue
      printf '%s\n' "$f"
    done
}

# ---------------------------------------------------------------------------
# 2. The build must derive its version, not restate it
# ---------------------------------------------------------------------------

step "Build derives version from VERSION"

if grep -nE 'project[[:space:]]*\([^)]*VERSION[[:space:]]+[0-9]+\.[0-9]+' "$ROOT/CMakeLists.txt" >/dev/null; then
  fail "CMakeLists.txt hard-codes a project(VERSION ...) literal; it must use \${CURRENT_VERSION}"
else
  ok "CMakeLists.txt takes its version from \${CURRENT_VERSION}"
fi

if grep -q 'include(SendBloomVersion)' "$ROOT/CMakeLists.txt"; then
  ok "CMakeLists.txt includes the canonical version parser"
else
  fail "CMakeLists.txt does not include(SendBloomVersion); the canonical VERSION parse is bypassed"
fi

for f in cmake-local/SendBloomVersion.cmake; do
  if grep -nE '(CURRENT_VERSION|SENDBLOOM_RELEASE_VERSION)[[:space:]]+"?[0-9]+\.[0-9]+\.[0-9]+' "$ROOT/$f" >/dev/null 2>&1; then
    fail "$f assigns a literal version instead of parsing the VERSION file"
  fi
done
ok "no literal version assignment in the version parser"

# ---------------------------------------------------------------------------
# 3. SendBloom-scoped version literals must equal the canonical version
# ---------------------------------------------------------------------------

step "Product-facing version literals agree with $RELEASE_VERSION"

SEMVER='[0-9]+\.[0-9]+\.[0-9]+(-[0-9A-Za-z]+(\.[0-9A-Za-z]+)*)?'
# Only literals that are unmistakably OUR release number:
#   SendBloom-<semver> / SendBloom <semver> / SendBloom_<semver>
#   v<semver> (tag form)
#   version <semver> / version: <semver> / version=<semver>
SCOPED="(SendBloom[-_ ]|[Vv]ersion[[:space:]:=]+|(^|[^0-9A-Za-z.])v)${SEMVER}"

scoped_hits=0
while IFS= read -r f; do
  while IFS= read -r hit; do
    lineno="${hit%%:*}"
    text="${hit#*:}"
    # Extract every semver-looking literal on the matched line.
    while IFS= read -r literal; do
      [[ -z "$literal" ]] && continue
      [[ "$literal" == "$RELEASE_VERSION" ]] && continue
      [[ "$literal" == "$VERSION_CORE" ]] && continue
      if is_allowlisted "$f" "$literal"; then
        continue
      fi
      fail "$f:$lineno references SendBloom $literal but the canonical version is $RELEASE_VERSION"
      info "      $(printf '%s' "$text" | cut -c1-120)"
      scoped_hits=$((scoped_hits + 1))
    done < <(printf '%s' "$text" | grep -oE "$SCOPED" | grep -oE "$SEMVER" || true)
  done < <(grep -nE "$SCOPED" "$ROOT/$f" || true)
done < <(scanned_files)

if [[ "$scoped_hits" -eq 0 ]]; then
  ok "no disagreeing SendBloom-scoped version literal found"
fi

# ---------------------------------------------------------------------------
# 4. Stale previous-version search
# ---------------------------------------------------------------------------

PREVIOUS_VERSION="${PREVIOUS_VERSION:-$(bash "$ROOT/scripts/release/version.sh" previous || true)}"

step "Stale previous-version search"

if [[ -z "$PREVIOUS_VERSION" ]]; then
  info "no previous released version found in git tags — nothing to search for"
else
  info "previous released version: $PREVIOUS_VERSION"
  stale_hits=0
  while IFS= read -r f; do
    while IFS= read -r hit; do
      lineno="${hit%%:*}"
      text="${hit#*:}"
      if is_allowlisted "$f" "$PREVIOUS_VERSION"; then
        continue
      fi
      fail "$f:$lineno still mentions the previous release $PREVIOUS_VERSION — release blocked"
      info "      $(printf '%s' "$text" | cut -c1-120)"
      stale_hits=$((stale_hits + 1))
    done < <(grep -nF "$PREVIOUS_VERSION" "$ROOT/$f" || true)
  done < <(scanned_files)

  if [[ "$stale_hits" -eq 0 ]]; then
    ok "previous release $PREVIOUS_VERSION appears nowhere outside the allowlist"
  fi
fi

# ---------------------------------------------------------------------------
# 5. Release notes title
# ---------------------------------------------------------------------------

step "Release notes title"

NOTES="$ROOT/RELEASE_NOTES.md"
if [[ ! -f "$NOTES" ]]; then
  fail "RELEASE_NOTES.md is missing"
else
  notes_title="$(grep -m1 '^# ' "$NOTES" || true)"
  # Exact-literal match: a title naming a release candidate must not satisfy
  # the final release just because the core version is a substring of it.
  title_versions="$(printf '%s' "$notes_title" | grep -oE "$SEMVER" || true)"
  if [[ -z "$title_versions" ]]; then
    fail "RELEASE_NOTES.md title names no version: '$notes_title'"
  elif [[ "$(printf '%s\n' "$title_versions" | sort -u | tr '\n' ' ' | xargs)" == "$RELEASE_VERSION" ]]; then
    ok "RELEASE_NOTES.md title names exactly $RELEASE_VERSION"
  else
    fail "RELEASE_NOTES.md title names [$(printf '%s' "$title_versions" | tr '\n' ' ')] but the canonical version is $RELEASE_VERSION"
  fi
fi

# ---------------------------------------------------------------------------
# 6. Changelog
# ---------------------------------------------------------------------------

step "Changelog entry"

CHANGELOG="$ROOT/CHANGELOG.md"
if [[ ! -f "$CHANGELOG" ]]; then
  fail "CHANGELOG.md is missing"
else
  first_entry="$(grep -m1 -E '^## \[?[0-9]+\.[0-9]+\.[0-9]+' "$CHANGELOG" || true)"
  if [[ -z "$first_entry" ]]; then
    fail "CHANGELOG.md has no versioned '## <version>' entry"
  elif [[ "$first_entry" == *"$RELEASE_VERSION"* ]]; then
    ok "CHANGELOG.md top entry is $RELEASE_VERSION"
  else
    fail "CHANGELOG.md top entry is '$first_entry' but the canonical version is $RELEASE_VERSION"
  fi
fi

# ---------------------------------------------------------------------------
# 7. Built bundle metadata (when a build is present)
# ---------------------------------------------------------------------------

step "Built bundle metadata"

checked_bundles=0
for bundle in "$VST3_BUNDLE" "$AU_BUNDLE"; do
  plist="$bundle/Contents/Info.plist"
  [[ -f "$plist" ]] || continue
  checked_bundles=$((checked_bundles + 1))

  short="$(plutil -extract CFBundleShortVersionString raw -o - "$plist" 2>/dev/null || echo '<missing>')"
  ident="$(plutil -extract CFBundleIdentifier raw -o - "$plist" 2>/dev/null || echo '<missing>')"

  [[ "$short" == "$VERSION_CORE" ]] \
    || fail "$bundle CFBundleShortVersionString=$short, expected $VERSION_CORE"
  [[ "$ident" == "$BUNDLE_ID" ]] \
    || fail "$bundle CFBundleIdentifier=$ident, expected $BUNDLE_ID"
done

if [[ "$checked_bundles" -eq 0 ]]; then
  warn "no built bundles under $ARTEFACTS_DIR — bundle metadata NOT verified in this run"
  warn "this is a SKIPPED check, not a passing one; release-all.sh re-runs it after the build"
else
  ok "$checked_bundles bundle(s) carry version $VERSION_CORE and id $BUNDLE_ID"
fi

# ---------------------------------------------------------------------------

echo
if [[ "$failures" -ne 0 ]]; then
  die "version consistency: $failures failure(s) — release blocked"
fi
printf '%sversion consistency: PASS%s (canonical %s)\n' "$_c_grn" "$_c_off" "$RELEASE_VERSION"
