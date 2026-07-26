#!/usr/bin/env bash
# The single maintainer release command.
#
#   scripts/release/release-all.sh                 # local rehearsal, no publish
#   RELEASE_MODE=public scripts/release/release-all.sh
#   RELEASE_MODE=public scripts/release/release-all.sh --publish
#
# Runs every release gate in the only correct order and stops at the first
# failure. There is no "best effort" anywhere in here: a step that cannot be
# performed is a failure, and a step that is deliberately skipped is recorded
# as SKIPPED in the final report and never reported as passed.
#
# Order (each depends on the previous):
#   preflight -> hygiene -> version -> legal -> release-script tests
#     -> clean build -> unit tests -> pluginval
#     -> sign -> notarize -> staple -> package -> sbom
#     -> checksums -> manifest -> validate final artifact
#     -> publish (opt-in) -> hosted revalidation -> installed truth
#     -> report

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

RELEASE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
release_init

PUBLISH=0
SKIP_TESTS=0
for arg in "$@"; do
  case "$arg" in
    --publish)     PUBLISH=1 ;;
    --skip-tests)  SKIP_TESTS=1 ;;
    -h|--help)
      sed -n '2,30p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
      exit 0 ;;
    *) die "unknown argument: $arg" ;;
  esac
done

START_TS="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
REPORT_DIR="$DIST_DIR"
mkdir -p "$REPORT_DIR"
REPORT="$REPORT_DIR/release-report.md"
LOGDIR="$REPORT_DIR/logs"
mkdir -p "$LOGDIR"

declare -a STEP_NAME=() STEP_STATUS=() STEP_DETAIL=() STEP_CMD=()

record() {
  STEP_NAME+=("$1"); STEP_STATUS+=("$2"); STEP_DETAIL+=("$3"); STEP_CMD+=("${4:-}")
}

# Run a gate. Any non-zero exit aborts the release immediately.
gate() {
  local name="$1"; shift
  local logfile="$LOGDIR/${name}.log"
  step "$name"
  if "$@" >"$logfile" 2>&1; then
    tail -n 3 "$logfile" | sed 's/^/      /'
    record "$name" "PASS" "$(tail -n 1 "$logfile" | tr -d '\r')" "$*"
    ok "$name"
  else
    local rc=$?
    sed 's/^/      /' "$logfile" >&2
    record "$name" "FAIL" "exit $rc — see $logfile" "$*"
    write_report "NOT READY"
    die "$name FAILED (exit $rc). Release blocked. Full log: $logfile"
  fi
}

# Record a check that was deliberately not run. Never counts as a pass.
skipped() {
  record "$1" "SKIPPED" "$2" "${3:-}"
  printf '%sSKIP%s %s — %s\n' "$_c_yel" "$_c_off" "$1" "$2" >&2
}

write_report() {
  local overall="${1:-NOT READY — aborted}"
  {
    echo "# SendBloom release report — $RELEASE_VERSION"
    echo
    echo "| | |"
    echo "|---|---|"
    echo "| Version | \`$RELEASE_VERSION\` |"
    echo "| Tag | \`$RELEASE_TAG\` |"
    echo "| Commit | \`$SOURCE_COMMIT\` |"
    echo "| Build ID | \`$BUILD_ID\` |"
    echo "| Release mode | \`$RELEASE_MODE\` |"
    echo "| Artifact contract | \`$ARTIFACT_CONTRACT\` |"
    echo "| Published | $([[ "$PUBLISH" == "1" ]] && echo yes || echo "no (dry run)") |"
    echo "| Started | $START_TS |"
    echo "| Finished | $(date -u +%Y-%m-%dT%H:%M:%SZ) |"
    echo "| Host | $(sw_vers -productName 2>/dev/null || uname -s) $(sw_vers -productVersion 2>/dev/null || uname -r) |"
    echo
    echo "## Gates"
    echo
    echo "| Gate | Status | Detail |"
    echo "|---|---|---|"
    local i
    for i in "${!STEP_NAME[@]}"; do
      printf '| %s | %s | %s |\n' "${STEP_NAME[$i]}" "${STEP_STATUS[$i]}" "$(printf '%s' "${STEP_DETAIL[$i]}" | tr '|' '/' | cut -c1-160)"
    done
    echo
    echo "## Commands run"
    echo
    echo '```'
    for i in "${!STEP_NAME[@]}"; do
      [[ -n "${STEP_CMD[$i]}" ]] && printf '%s\n' "${STEP_CMD[$i]}"
    done
    echo '```'
    echo
    echo "## Artifacts"
    echo
    if [[ -f "$CHECKSUM_PATH" ]]; then
      echo '```'
      cat "$CHECKSUM_PATH"
      echo '```'
    else
      echo "_No checksum file was produced._"
    fi
    echo
    if [[ -f "$MANIFEST_PATH" ]]; then
      echo "Manifest: \`$MANIFEST_NAME\` · Provenance: \`$PROVENANCE_NAME\` · SBOM: \`$SBOM_NAME\`"
      echo
    fi
    echo "## Skipped checks and residual risk"
    echo
    local any_skipped=0
    for i in "${!STEP_NAME[@]}"; do
      if [[ "${STEP_STATUS[$i]}" == "SKIPPED" ]]; then
        any_skipped=1
        printf -- '- **%s** — %s\n' "${STEP_NAME[$i]}" "${STEP_DETAIL[$i]}"
      fi
    done
    [[ "$any_skipped" == "0" ]] && echo "_None._"
    echo
    echo "Standing caveats for every SendBloom release are listed in"
    echo "\`docs/release-validation.md\` under \"Known caveats\"."
    echo
    echo "## Readiness"
    echo
    echo "**${overall:-UNKNOWN}**"
  } >"$REPORT"
}

# Always leave a report behind, even on an abort.
trap 'write_report "NOT READY — aborted"' ERR

# ---------------------------------------------------------------------------
step "SendBloom release — $RELEASE_VERSION"
# ---------------------------------------------------------------------------

info "mode:      $RELEASE_MODE"
info "contract:  $ARTIFACT_CONTRACT"
info "build id:  $BUILD_ID"
info "publish:   $([[ "$PUBLISH" == "1" ]] && echo "YES" || echo "no (dry run)")"
info "report:    $REPORT"
echo

if [[ "$RELEASE_MODE" != "public" ]]; then
  unsigned_banner
  if [[ "$PUBLISH" == "1" ]]; then
    die "--publish requires RELEASE_MODE=public. A rehearsal build must never be hosted."
  fi
fi

# ---------------------------------------------------------------------------
# Preflight: fail before doing expensive work if credentials are missing
# ---------------------------------------------------------------------------

step "Preflight"
if [[ "$RELEASE_MODE" == "public" ]]; then
  require_macos
  require_application_identity
  require_notary_profile
  [[ "$ARTIFACT_CONTRACT" == "pkg-in-dmg" ]] && require_installer_identity
  [[ "$SOURCE_DIRTY" == "0" ]] || die "a public release must be built from a committed tree"
  [[ "${ALLOW_DIRTY:-0}" == "0" ]] || die "ALLOW_DIRTY must not be set for a public release"
  record "preflight" "PASS" "credentials present; tree clean at $SOURCE_COMMIT"
  ok "credentials present, working tree clean"
else
  record "preflight" "PASS" "local rehearsal; signing and notarization credentials not required"
  ok "local rehearsal preflight"
fi
echo

# ---------------------------------------------------------------------------
# Repository gates
# ---------------------------------------------------------------------------

gate "tree-hygiene"        bash "$RELEASE_DIR/check-tree-hygiene.sh"
gate "version-consistency" bash "$RELEASE_DIR/check-version-consistency.sh"
gate "legal-metadata"      bash "$ROOT/scripts/check-legal-metadata.sh"
gate "reference-claims"    bash "$ROOT/scripts/verify-reference-claims.sh"

# ---------------------------------------------------------------------------
# The release scripts themselves
# ---------------------------------------------------------------------------

gate "release-script-tests" bash "$ROOT/tests/release/run-release-tests.sh"

# ---------------------------------------------------------------------------
# Build and product tests
# ---------------------------------------------------------------------------

gate "clean-build" bash "$RELEASE_DIR/build-macos.sh"

# Re-run the version gate now that bundles exist, so the bundle metadata check
# is a real result rather than the earlier skip.
gate "version-consistency-built" bash "$RELEASE_DIR/check-version-consistency.sh"

if [[ "$SKIP_TESTS" == "1" ]]; then
  skipped "unit-tests" "--skip-tests was passed. The product test suite did NOT run for this build."
else
  gate "unit-tests" ctest --test-dir "$RELEASE_BUILD_DIR" -C "$BUILD_TYPE" --output-on-failure
fi

gate "enab-acceptance" env BUILD_DIR="$RELEASE_BUILD_DIR" bash "$ROOT/scripts/enab-acceptance-gates.sh"

# pluginval on the exact VST3 that is about to be packaged.
PLUGINVAL_BIN="${PLUGINVAL_BIN:-}"
if [[ -z "$PLUGINVAL_BIN" ]]; then
  for candidate in \
    "$(command -v pluginval 2>/dev/null || true)" \
    "/Applications/pluginval.app/Contents/MacOS/pluginval" \
    "$ROOT/pluginval.app/Contents/MacOS/pluginval"; do
    [[ -n "$candidate" && -x "$candidate" ]] && { PLUGINVAL_BIN="$candidate"; break; }
  done
fi

if [[ -n "$PLUGINVAL_BIN" && -x "$PLUGINVAL_BIN" ]]; then
  gate "pluginval-vst3" "$PLUGINVAL_BIN" --strictness-level "${STRICTNESS_LEVEL:-10}" --validate "$VST3_BUNDLE"
else
  if [[ "$RELEASE_MODE" == "public" ]]; then
    record "pluginval-vst3" "FAIL" "pluginval not found; a public release cannot skip plug-in validation"
    write_report "NOT READY"
    die "pluginval is required for a public release. Set PLUGINVAL_BIN or install pluginval."
  fi
  skipped "pluginval-vst3" "pluginval binary not found on this machine (rehearsal run)"
fi

# auval can only see Audio Units that are installed in a standard location, so
# it cannot validate the component sitting in the build tree. It runs for real
# in the installed-truth stage below, against the copy the release actually
# put on the machine. Recording it here as skipped keeps that honest instead
# of letting a stale installed copy pass as validation of this build.
skipped "auval-au-prepackage" "auval only inspects installed Audio Units; the built component is validated after install smoke, not here"

# ---------------------------------------------------------------------------
# Sign, notarize, staple, package
# ---------------------------------------------------------------------------

gate "package" bash "$RELEASE_DIR/package-macos.sh"

# ---------------------------------------------------------------------------
# Inventory, checksums, manifest — strictly after packaging is finalised
# ---------------------------------------------------------------------------

gate "sbom"      bash "$RELEASE_DIR/sbom.sh"
gate "checksums" bash "$RELEASE_DIR/checksums.sh"
gate "manifest"  bash "$RELEASE_DIR/manifest.sh"

# ---------------------------------------------------------------------------
# Validate the artifact a user receives
# ---------------------------------------------------------------------------

gate "validate-artifacts" bash "$RELEASE_DIR/validate-artifacts.sh" "$DIST_DIR"

# ---------------------------------------------------------------------------
# Publish
# ---------------------------------------------------------------------------

if [[ "$PUBLISH" == "1" ]]; then
  gate "publish" bash "$RELEASE_DIR/publish-github.sh" --publish
  gate "hosted-verification" bash "$RELEASE_DIR/verify-hosted-release.sh" "$RELEASE_TAG"
elif [[ "$RELEASE_MODE" == "public" ]]; then
  gate "publish-dry-run" bash "$RELEASE_DIR/publish-github.sh"
  skipped "hosted-verification" "nothing was published, so there is no hosted artifact to download and revalidate"
else
  # A rehearsal build has nothing publishable, by design. Running the publish
  # preflight here would be asking it to refuse and then treating that correct
  # refusal as a pipeline failure.
  skipped "publish-dry-run" "RELEASE_MODE=$RELEASE_MODE produces no publishable artifact; the publish path is exercised by the release-script tests and by a public-mode dry run"
  skipped "hosted-verification" "nothing was published, so there is no hosted artifact to download and revalidate"
fi

# ---------------------------------------------------------------------------
# Installed truth
# ---------------------------------------------------------------------------

if [[ "${SENDBLOOM_INSTALL_SMOKE:-0}" == "1" ]]; then
  gate "install-smoke" bash "$RELEASE_DIR/install-smoke-macos.sh" --i-understand
  gate "installed-truth" bash "$RELEASE_DIR/verify-installed-macos.sh"
else
  skipped "install-smoke" "SENDBLOOM_INSTALL_SMOKE was not set. Nothing was installed, so the installed product was NOT proven to be this build."
  skipped "installed-truth" "depends on install smoke; no installation was performed by this run"
fi

# ---------------------------------------------------------------------------
# Readiness
# ---------------------------------------------------------------------------

trap - ERR

any_fail=0
any_skip=0
for i in "${!STEP_STATUS[@]}"; do
  [[ "${STEP_STATUS[$i]}" == "FAIL" ]] && any_fail=1
  [[ "${STEP_STATUS[$i]}" == "SKIPPED" ]] && any_skip=1
done

if [[ "$any_fail" == "1" ]]; then
  OVERALL="NOT READY"
elif [[ "$RELEASE_MODE" != "public" ]]; then
  OVERALL="NOT READY — local rehearsal only; no signed, notarized or published artifact exists"
elif [[ "$PUBLISH" != "1" ]]; then
  OVERALL="READY WITH CAVEATS — artifacts validated locally; nothing published, so hosted artifact truth is unproven"
elif [[ "$any_skip" == "1" ]]; then
  OVERALL="READY WITH CAVEATS — see the skipped checks above"
else
  OVERALL="READY"
fi

write_report "$OVERALL"

echo
step "Gate summary"
printf '      %-28s %-8s %s\n' "GATE" "STATUS" "DETAIL"
printf '      %-28s %-8s %s\n' "----" "------" "------"
for i in "${!STEP_NAME[@]}"; do
  printf '      %-28s %-8s %s\n' "${STEP_NAME[$i]}" "${STEP_STATUS[$i]}" "$(printf '%s' "${STEP_DETAIL[$i]}" | cut -c1-70)"
done
echo
info "report: $REPORT"
echo
printf '%s%s%s\n' "$_c_bld" "$OVERALL" "$_c_off"

[[ "$any_fail" == "0" ]]
