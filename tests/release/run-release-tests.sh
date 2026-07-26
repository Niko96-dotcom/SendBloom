#!/usr/bin/env bash
# Regression tests for the SendBloom release scripts.
#
# The release pipeline is the thing that decides whether a bad artifact
# reaches users, so it needs tests of its own. These run with no credentials,
# no network and no Apple involvement: every platform tool that would need one
# is replaced by a fake on PATH (tests/release/fakebin) that records what it
# was asked to do and can be told to fail in specific, realistic ways.
#
# Each test builds a throwaway repository fixture, runs the real release
# scripts against it, and asserts on the outcome.
#
#   bash tests/release/run-release-tests.sh            # all tests
#   bash tests/release/run-release-tests.sh T09        # one test by id

set -uo pipefail

TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$TESTS_DIR/../.." && pwd)"
FAKEBIN="$TESTS_DIR/fakebin"

FILTER="${1:-}"

pass_count=0
fail_count=0
skip_count=0
declare -a FAILED=()

c_red=$'\033[31m'; c_grn=$'\033[32m'; c_yel=$'\033[33m'; c_bld=$'\033[1m'; c_off=$'\033[0m'
[[ -t 1 ]] || { c_red=""; c_grn=""; c_yel=""; c_bld=""; c_off=""; }

# ---------------------------------------------------------------------------
# Fixture
# ---------------------------------------------------------------------------

make_plist() {
  local out="$1" version="$2"
  cat >"$out" <<PLIST
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
    <key>CFBundleIdentifier</key><string>com.nikoaudiolabs.sendbloom</string>
    <key>CFBundleName</key><string>SendBloom</string>
    <key>CFBundleShortVersionString</key><string>$version</string>
    <key>CFBundleVersion</key><string>$version</string>
</dict>
</plist>
PLIST
}

# make_sandbox <version> [--with-build] [--tag <tag>]
make_sandbox() {
  local version="$1"; shift
  local with_build=0
  local -a tags=()
  while [[ $# -gt 0 ]]; do
    case "$1" in
      --with-build) with_build=1; shift ;;
      --tag) tags+=("$2"); shift 2 ;;
      *) shift ;;
    esac
  done

  local sb
  sb="$(mktemp -d -t sendbloom-relbox.XXXXXX)"

  mkdir -p "$sb/scripts/release" "$sb/cmake-local" "$sb/docs" "$sb/.release" "$sb/tests/release"
  cp "$REPO_ROOT"/scripts/release/*.sh "$sb/scripts/release/"
  cp "$REPO_ROOT"/cmake-local/SendBloomVersion.cmake "$sb/cmake-local/"
  chmod +x "$sb"/scripts/release/*.sh

  printf '%s\n' "$version" >"$sb/VERSION"

  cat >"$sb/CMakeLists.txt" <<'CM'
cmake_minimum_required(VERSION 3.25)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake-local")
include(SendBloomVersion)
project(SendBloom VERSION ${CURRENT_VERSION})
CM

  printf '# SendBloom %s\n\nFixture release notes.\n' "$version" >"$sb/RELEASE_NOTES.md"
  printf '# Changelog\n\n## [%s]\n\nFixture entry.\n' "$version" >"$sb/CHANGELOG.md"
  printf 'MIT License — fixture\n' >"$sb/LICENSE"
  printf '# Third party\n\nr8brain (MIT), JUCE, Catch2.\n' >"$sb/docs/THIRD_PARTY_LICENSES.md"

  # Dependency pins the SBOM generator reads.
  mkdir -p "$sb/cmake"
  cat >"$sb/cmake-local/R8brain.cmake" <<'R8'
CPMAddPackage(
    NAME r8brain
    GITHUB_REPOSITORY avaneev/r8brain-free-src
    GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
    DOWNLOAD_ONLY YES
)
R8
  printf 'CPMAddPackage("gh:catchorg/Catch2@3.8.1")\n' >"$sb/cmake/Tests.cmake"
  # Test scaffolding lives in the sandbox root; it must not make the fixture
  # look like a dirty working tree to the release scripts.
  printf 'dist/\nbuild-release/\norder.log\nfakestate/\n.origin.git/\n' >"$sb/.gitignore"

  cat >"$sb/.release/version-allowlist.txt" <<'AL'
# Fixture allowlist.
CHANGELOG.md | * | Historical changelog entries survive a version bump by design.
AL

  if [[ "$with_build" == "1" ]]; then
    local art="$sb/build-release/SendBloom_artefacts/Release"
    mkdir -p "$art/VST3/SendBloom.vst3/Contents/MacOS" \
             "$art/AU/SendBloom.component/Contents/MacOS"
    local core="${version%%-*}"
    make_plist "$art/VST3/SendBloom.vst3/Contents/Info.plist" "$core"
    make_plist "$art/AU/SendBloom.component/Contents/Info.plist" "$core"
    printf 'fake mach-o\n' >"$art/VST3/SendBloom.vst3/Contents/MacOS/SendBloom"
    printf 'fake mach-o\n' >"$art/AU/SendBloom.component/Contents/MacOS/SendBloom"
    printf '%s\n' "$version" >"$sb/build-release/VERSION"
  fi

  (
    cd "$sb"
    git init -q
    git config user.email fixture@example.invalid
    git config user.name Fixture
    git add -A
    git commit -q -m "fixture"
    # A local bare remote, so publish preflight can inspect "origin" without
    # any network access.
    git init -q --bare "$sb/.origin.git"
    git remote add origin "$sb/.origin.git"
    for t in "${tags[@]:-}"; do
      [[ -n "$t" ]] && git tag -a "$t" -m "$t" >/dev/null
    done
  ) >/dev/null 2>&1

  printf '%s' "$sb"
}

# ---------------------------------------------------------------------------
# Harness
# ---------------------------------------------------------------------------

CURRENT_ID=""
CURRENT_NAME=""
CURRENT_FAILURES=()

check() {
  local desc="$1"; shift
  if "$@"; then
    printf '      %sok%s   %s\n' "$c_grn" "$c_off" "$desc"
  else
    printf '      %sFAIL%s %s\n' "$c_red" "$c_off" "$desc"
    CURRENT_FAILURES+=("$desc")
  fi
}

check_contains() {
  local desc="$1" haystack="$2" needle="$3"
  if [[ "$haystack" == *"$needle"* ]]; then
    printf '      %sok%s   %s\n' "$c_grn" "$c_off" "$desc"
  else
    printf '      %sFAIL%s %s\n' "$c_red" "$c_off" "$desc"
    printf '            expected to find: %s\n' "$needle"
    CURRENT_FAILURES+=("$desc")
  fi
}

check_not_contains() {
  local desc="$1" haystack="$2" needle="$3"
  if [[ "$haystack" != *"$needle"* ]]; then
    printf '      %sok%s   %s\n' "$c_grn" "$c_off" "$desc"
  else
    printf '      %sFAIL%s %s\n' "$c_red" "$c_off" "$desc"
    printf '            did not expect: %s\n' "$needle"
    CURRENT_FAILURES+=("$desc")
  fi
}

check_before() {
  local desc="$1" log="$2" first="$3" second="$4"
  local a b
  a="$(grep -n -m1 -F "$first" "$log" | cut -d: -f1)"
  b="$(grep -n -m1 -F "$second" "$log" | cut -d: -f1)"
  if [[ -n "$a" && -n "$b" && "$a" -lt "$b" ]]; then
    printf '      %sok%s   %s\n' "$c_grn" "$c_off" "$desc"
  else
    printf '      %sFAIL%s %s (%s@%s, %s@%s)\n' "$c_red" "$c_off" "$desc" "$first" "${a:-absent}" "$second" "${b:-absent}"
    CURRENT_FAILURES+=("$desc")
  fi
}

test_case() {
  CURRENT_ID="$1"
  CURRENT_NAME="$2"
  CURRENT_FAILURES=()
  if [[ -n "$FILTER" && "$FILTER" != "$CURRENT_ID" ]]; then
    return 1
  fi
  printf '%s%s %s%s\n' "$c_bld" "$CURRENT_ID" "$CURRENT_NAME" "$c_off"
  return 0
}

end_case() {
  if [[ ${#CURRENT_FAILURES[@]} -eq 0 ]]; then
    pass_count=$((pass_count + 1))
  else
    fail_count=$((fail_count + 1))
    FAILED+=("$CURRENT_ID $CURRENT_NAME")
  fi
  echo
}

# Variables the release scripts export for their own sub-scripts. When this
# suite runs as a gate inside release-all.sh they are already in the
# environment and would override the sandbox's own derived paths — a sandbox
# would then sign the real repository's build tree and write to the real dist
# directory. Every test runs with them cleared.
RELEASE_ENV_TO_CLEAR=(
  ROOT RELEASE_MODE ARTIFACT_CONTRACT SENDBLOOM_ARTIFACT_CONTRACT
  REQUIRE_SIGNED ARTIFACT_SUFFIX ALLOW_DIRTY
  RELEASE_VERSION VERSION_CORE VERSION_PRERELEASE VERSION_MAJOR VERSION_MINOR
  VERSION_PATCH RELEASE_TAG RELEASE_IS_PRERELEASE PREVIOUS_VERSION
  BUILD_ID SOURCE_COMMIT SOURCE_COMMIT_SHORT SOURCE_DIRTY
  RELEASE_BUILD_DIR DIST_DIR BUILD_TYPE ARTEFACTS_DIR VST3_BUNDLE AU_BUNDLE
  PKG_NAME DMG_NAME NOTARIZE_ZIP_NAME CHECKSUM_NAME MANIFEST_NAME
  PROVENANCE_NAME SBOM_NAME PKG_PATH DMG_PATH CHECKSUM_PATH MANIFEST_PATH
  PROVENANCE_PATH SBOM_PATH MANIFEST_IN NOTARY_RECORD_DIR ALLOWLIST
  BUNDLE_ID PKG_IDENTIFIER VST3_INSTALL_DIR AU_INSTALL_DIR
  DEVELOPER_ID_APPLICATION DEVELOPER_ID_INSTALLER NOTARY_PROFILE
  KEEP_BUILD_DIR PLUGINVAL_BIN
)

CLEAR_ARGS=()
for _v in "${RELEASE_ENV_TO_CLEAR[@]}"; do CLEAR_ARGS+=(-u "$_v"); done

# Run a release script inside a sandbox with the fakes on PATH.
run_in() {
  local sb="$1"; shift
  ( cd "$sb" && env "${CLEAR_ARGS[@]}" PATH="$FAKEBIN:$PATH" NO_COLOR=1 "$@" 2>&1 )
}

# ===========================================================================
printf '%sSendBloom release-script regression tests%s\n' "$c_bld" "$c_off"
printf 'fakes: %s\n\n' "$FAKEBIN"
# ===========================================================================

# --- T01 -------------------------------------------------------------------
if test_case T01 "canonical version parses and derives every field"; then
  sb="$(make_sandbox 1.4.2)"
  out="$(run_in "$sb" bash scripts/release/version.sh)"
  check_contains "RELEASE_VERSION is the VERSION file contents" "$out" "RELEASE_VERSION=1.4.2"
  check_contains "numeric core derived" "$out" "VERSION_CORE=1.4.2"
  check_contains "tag derived from version" "$out" "RELEASE_TAG=v1.4.2"
  check_contains "not a prerelease" "$out" "RELEASE_IS_PRERELEASE=0"
  check_contains "build id carries the commit" "$out" "BUILD_ID=1.4.2+"
  rm -rf "$sb"
  end_case
fi

# --- T02 -------------------------------------------------------------------
if test_case T02 "prerelease label parses and does not leak into the numeric core"; then
  sb="$(make_sandbox 2.0.0-rc3)"
  out="$(run_in "$sb" bash scripts/release/version.sh)"
  check_contains "full version keeps the label" "$out" "RELEASE_VERSION=2.0.0-rc3"
  check_contains "core drops the label (Apple bundle metadata needs numeric)" "$out" "VERSION_CORE=2.0.0"
  check_contains "prerelease captured" "$out" "VERSION_PRERELEASE=rc3"
  check_contains "marked as prerelease" "$out" "RELEASE_IS_PRERELEASE=1"
  rm -rf "$sb"
  end_case
fi

# --- T03 -------------------------------------------------------------------
if test_case T03 "a malformed VERSION file is rejected, not guessed at"; then
  for bad in "1.0" "v1.0.0" "1.0.0.0" "" "1.0.0-"; do
    sb="$(make_sandbox 1.0.0)"
    printf '%s\n' "$bad" >"$sb/VERSION"
    out="$(run_in "$sb" bash scripts/release/version.sh)"
    rc=$?
    check "rejects VERSION='$bad'" test "$rc" -ne 0
    rm -rf "$sb"
  done
  end_case
fi

# --- T04 -------------------------------------------------------------------
if test_case T04 "a stale previous version anywhere product-facing blocks the release"; then
  sb="$(make_sandbox 1.1.0 --tag v1.0.0)"
  printf 'Install SendBloom 1.0.0 from the disk image.\n' >"$sb/docs/install.md"
  ( cd "$sb" && git add -A && git commit -q -m docs )
  out="$(run_in "$sb" bash scripts/release/check-version-consistency.sh)"
  rc=$?
  check "gate fails" test "$rc" -ne 0
  check_contains "names the stale version" "$out" "still mentions the previous release 1.0.0"
  check_contains "names the offending file" "$out" "docs/install.md"

  # With a documented allowlist entry the same tree passes.
  printf 'docs/install.md | 1.0.0 | Upgrade instructions must keep naming the version being upgraded from.\n' \
    >>"$sb/.release/version-allowlist.txt"
  ( cd "$sb" && git add -A && git commit -q -m allowlist )
  out2="$(run_in "$sb" bash scripts/release/check-version-consistency.sh)"
  rc2=$?
  check "gate passes once the exemption is documented" test "$rc2" -eq 0
  rm -rf "$sb"
  end_case
fi

# --- T05 -------------------------------------------------------------------
if test_case T05 "an allowlist entry without a reason is a config error"; then
  sb="$(make_sandbox 1.0.0)"
  printf 'docs/install.md | 0.9.0 |\n' >>"$sb/.release/version-allowlist.txt"
  out="$(run_in "$sb" bash scripts/release/check-version-consistency.sh)"
  rc=$?
  check "gate fails" test "$rc" -ne 0
  check_contains "says the exemption must explain itself" "$out" "has no reason"
  rm -rf "$sb"
  end_case
fi

# --- T06 -------------------------------------------------------------------
if test_case T06 "a public release with no signing identity fails, and signs nothing"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  out="$(run_in "$sb" env RELEASE_MODE=public FAKE_LOG="$log" \
        bash scripts/release/sign-macos.sh)"
  rc=$?
  check "sign fails" test "$rc" -ne 0
  check_contains "explains which credential is missing" "$out" "DEVELOPER_ID_APPLICATION is not set"
  check "nothing was signed" test ! -s "$log"
  rm -rf "$sb"
  end_case
fi

# --- T07 -------------------------------------------------------------------
if test_case T07 "pkg-in-dmg without an installer identity fails with the actual remedy"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  out="$(run_in "$sb" env RELEASE_MODE=public \
        SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
        DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
        FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
        NOTARY_PROFILE=fake-profile \
        bash scripts/release/package-macos.sh)"
  rc=$?
  check "packaging fails" test "$rc" -ne 0
  check_contains "names the missing identity" "$out" "DEVELOPER_ID_INSTALLER is not set"
  check_contains "offers the documented alternative contract" "$out" "SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg"
  rm -rf "$sb"
  end_case
fi

# --- T08 -------------------------------------------------------------------
if test_case T08 "the local-only override is loudly labelled and cannot publish"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  out="$(run_in "$sb" env RELEASE_MODE=local-unsigned \
        SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg FAKE_LOG="$log" \
        bash scripts/release/package-macos.sh)"
  rc=$?
  check "rehearsal packaging succeeds" test "$rc" -eq 0
  check_contains "banner names the mode" "$out" "LOCAL REHEARSAL BUILD — NOT A RELEASE"
  check_contains "says it must not be published" "$out" "MUST NOT be published"
  check "artifact name cannot be mistaken for a release" \
        test -f "$sb/dist/1.0.0/SendBloom-1.0.0-macOS-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE.dmg"
  check_not_contains "never contacted the notary service" "$(cat "$log" 2>/dev/null)" "notarytool"

  pub="$(run_in "$sb" env RELEASE_MODE=local-unsigned \
        SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg \
        bash scripts/release/publish-github.sh --publish)"
  prc=$?
  check "publishing a rehearsal build is refused" test "$prc" -ne 0
  check_contains "explains why" "$pub" "cannot publish"
  rm -rf "$sb"
  end_case
fi

# --- T09 -------------------------------------------------------------------
if test_case T09 "a rejected notarization blocks the release and is never stapled"; then
  for status in Invalid Rejected; do
    sb="$(make_sandbox 1.0.0 --with-build)"
    log="$sb/order.log"
    out="$(run_in "$sb" env RELEASE_MODE=public \
          SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg \
          DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
          FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
          NOTARY_PROFILE=fake-profile \
          FAKE_LOG="$log" FAKE_NOTARY_STATUS="$status" FAKE_STATE="$sb/fakestate" \
          bash scripts/release/package-macos.sh)"
    rc=$?
    check "status=$status blocks packaging" test "$rc" -ne 0
    check_contains "status=$status is reported" "$out" "$status"
    check_not_contains "status=$status: nothing was stapled" "$(cat "$log" 2>/dev/null)" "stapler:staple"
    check "status=$status: no disk image was produced" \
          test ! -f "$sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg"
    rm -rf "$sb"
  done
  end_case
fi

# --- T10 -------------------------------------------------------------------
if test_case T10 "checksums refuse to describe an artifact that is not stapled"; then
  sb="$(make_sandbox 1.0.0)"
  mkdir -p "$sb/dist/1.0.0"
  printf 'not really a disk image\n' >"$sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg"
  out="$(run_in "$sb" env RELEASE_MODE=public FAKE_STATE="$sb/fakestate" \
        bash scripts/release/checksums.sh)"
  rc=$?
  check "checksum generation fails" test "$rc" -ne 0
  check_contains "explains the ordering rule" "$out" "must not be generated before stapling"
  check "no checksum file was written" test ! -f "$sb/dist/1.0.0/SHA256SUMS.txt"
  rm -rf "$sb"
  end_case
fi

# --- T11 -------------------------------------------------------------------
if test_case T11 "checksums use basenames and verify after the files move"; then
  sb="$(make_sandbox 1.0.0)"
  mkdir -p "$sb/dist/1.0.0"
  printf 'disk image bytes\n' >"$sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg"
  out="$(run_in "$sb" env RELEASE_MODE=local-unsigned \
        bash scripts/release/checksums.sh)"
  rc=$?
  # local-unsigned names the artifact with the suffix, so create that one too.
  if [[ "$rc" -ne 0 ]]; then
    mv "$sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg" \
       "$sb/dist/1.0.0/SendBloom-1.0.0-macOS-LOCAL-UNSIGNED-DO-NOT-DISTRIBUTE.dmg"
    out="$(run_in "$sb" env RELEASE_MODE=local-unsigned bash scripts/release/checksums.sh)"
    rc=$?
  fi
  check "checksums generated" test "$rc" -eq 0
  sums="$sb/dist/1.0.0/SHA256SUMS.txt"
  check "checksum file exists" test -f "$sums"
  if [[ -f "$sums" ]]; then
    check_not_contains "no absolute path leaked" "$(cat "$sums")" "$sb"
    check_not_contains "no directory separator in any entry" "$(awk '{print $2}' "$sums")" "/"
    check_contains "harness proved it verifies from elsewhere" "$out" "verifies after the artifacts are moved"

    elsewhere="$(mktemp -d)"
    cp "$sb/dist/1.0.0/"*.dmg "$elsewhere/"
    cp "$sums" "$elsewhere/"
    ( cd "$elsewhere" && shasum -a 256 -c SHA256SUMS.txt >/dev/null 2>&1 )
    check "independently verifies in an unrelated directory" test $? -eq 0
    rm -rf "$elsewhere"
  fi
  rm -rf "$sb"
  end_case
fi

# --- T12 -------------------------------------------------------------------
if test_case T12 "signature verification accepts every valid codesign output shape"; then
  for variant in runtime-flag runtime-line combined-flags; do
    sb="$(make_sandbox 1.0.0 --with-build)"
    out="$(run_in "$sb" env RELEASE_MODE=public \
          DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
          FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
          FAKE_CODESIGN_VARIANT="$variant" \
          bash scripts/release/sign-macos.sh)"
    rc=$?
    check "accepts hardened runtime reported as '$variant'" test "$rc" -eq 0
    rm -rf "$sb"
  done

  for variant in no-runtime adhoc no-timestamp; do
    sb="$(make_sandbox 1.0.0 --with-build)"
    out="$(run_in "$sb" env RELEASE_MODE=public \
          DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
          FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
          FAKE_CODESIGN_VARIANT="$variant" \
          bash scripts/release/sign-macos.sh)"
    rc=$?
    check "rejects '$variant'" test "$rc" -ne 0
    rm -rf "$sb"
  done
  end_case
fi

# --- T13 -------------------------------------------------------------------
if test_case T13 "the pipeline runs build, sign, notarize, staple, package, checksum in that order"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  out="$(run_in "$sb" env RELEASE_MODE=public \
        SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
        DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
        DEVELOPER_ID_INSTALLER="Developer ID Installer: Fake Maintainer (TEAM123456)" \
        FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
        FAKE_INSTALLER_IDENTITY="Developer ID Installer: Fake Maintainer (TEAM123456)" \
        NOTARY_PROFILE=fake-profile FAKE_LOG="$log" FAKE_STATE="$sb/fakestate" \
        bash scripts/release/package-macos.sh)"
  rc=$?
  check "packaging succeeds" test "$rc" -eq 0
  if [[ ! -f "$log" ]]; then
    CURRENT_FAILURES+=("no order log produced")
    printf '%s\n' "$out" | tail -20
  else
    check_before "bundles are signed before they are submitted" "$log" \
      "codesign:sign" "notarytool:submit"
    check_before "bundles are notarized before they are stapled" "$log" \
      "notarytool:submit" "stapler:staple"
    check_before "bundles are stapled before the installer is built" "$log" \
      "stapler:staple" "pkgbuild:"
    check_before "the installer is signed before it is notarized" "$log" \
      "productsign:" "notarytool:submit SendBloom-1.0.0.pkg"
    check_before "the installer is finalised before the disk image is built" "$log" \
      "stapler:staple SendBloom-1.0.0.pkg" "hdiutil:create"
    check_before "the disk image is signed before it is notarized" "$log" \
      "codesign:sign $sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg" \
      "notarytool:submit SendBloom-1.0.0-macOS.dmg"
    check_before "the disk image is notarized before Gatekeeper assessment" "$log" \
      "notarytool:submit SendBloom-1.0.0-macOS.dmg" "spctl:assess SendBloom-1.0.0-macOS.dmg"
  fi

  # Checksums come after everything above, never before.
  cout="$(run_in "$sb" env RELEASE_MODE=public FAKE_LOG="$log" FAKE_STATE="$sb/fakestate" \
        bash scripts/release/checksums.sh)"
  crc=$?
  check "checksums succeed once artifacts are stapled" test "$crc" -eq 0
  check_before "hashing happens after stapling" "$log" "stapler:staple" "stapler:validate"
  rm -rf "$sb"
  end_case
fi

# --- T14 -------------------------------------------------------------------
if test_case T14 "the shipped disk image really contains the contracted payload"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  common_env=(RELEASE_MODE=public
    DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)"
    DEVELOPER_ID_INSTALLER="Developer ID Installer: Fake Maintainer (TEAM123456)"
    FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)"
    FAKE_INSTALLER_IDENTITY="Developer ID Installer: Fake Maintainer (TEAM123456)"
    NOTARY_PROFILE=fake-profile FAKE_LOG="$log" FAKE_STATE="$sb/fakestate")

  run_in "$sb" env "${common_env[@]}" SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
    bash scripts/release/package-macos.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
    bash scripts/release/checksums.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
    bash scripts/release/manifest.sh >/dev/null 2>&1

  vout="$(run_in "$sb" env "${common_env[@]}" SENDBLOOM_ARTIFACT_CONTRACT=pkg-in-dmg \
        bash scripts/release/validate-artifacts.sh)"
  vrc=$?
  check "validation of the final artifact passes" test "$vrc" -eq 0
  check_contains "the disk image was mounted, not guessed at" "$vout" "mounted at"
  check_contains "the installer is inside the disk image" "$vout" "disk image contains the installer"
  check_contains "the payload carries the plug-ins" "$vout" "payload contains SendBloom.vst3"
  check_contains "the payload version is the release version" "$vout" "reports version 1.0.0"
  check_contains "checksums verify from the artifact directory" "$vout" "verifies"
  check_contains "hashes match the manifest" "$vout" "matches the manifest hash"
  if [[ "$vrc" -ne 0 ]]; then printf '%s\n' "$vout" | tail -30; fi
  rm -rf "$sb"
  end_case
fi

# --- T15 -------------------------------------------------------------------
if test_case T15 "a corrupted artifact is caught by manifest comparison"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  common_env=(RELEASE_MODE=public SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg
    DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)"
    FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)"
    NOTARY_PROFILE=fake-profile FAKE_LOG="$log" FAKE_STATE="$sb/fakestate")

  run_in "$sb" env "${common_env[@]}" bash scripts/release/package-macos.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/checksums.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/sbom.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/manifest.sh >/dev/null 2>&1

  # Tamper with the artifact after the manifest was written.
  printf 'tampered\n' >>"$sb/dist/1.0.0/SendBloom-1.0.0-macOS.dmg"

  vout="$(run_in "$sb" env "${common_env[@]}" bash scripts/release/validate-artifacts.sh)"
  vrc=$?
  check "validation fails on the tampered artifact" test "$vrc" -ne 0
  check_contains "checksum mismatch is reported" "$vout" "does not verify"
  check_contains "manifest mismatch is reported" "$vout" "hash mismatch"

  pout="$(run_in "$sb" env "${common_env[@]}" bash scripts/release/publish-github.sh --publish)"
  prc=$?
  check "publishing the tampered artifact is refused" test "$prc" -ne 0
  check_contains "explains what changed" "$pout" "changed since the manifest was written"
  rm -rf "$sb"
  end_case
fi

# --- T16 -------------------------------------------------------------------
if test_case T16 "a dry-run publish creates no tag and no release"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  log="$sb/order.log"
  common_env=(RELEASE_MODE=public SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg
    DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)"
    FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)"
    NOTARY_PROFILE=fake-profile FAKE_LOG="$log" FAKE_STATE="$sb/fakestate")

  run_in "$sb" env "${common_env[@]}" bash scripts/release/package-macos.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/checksums.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/sbom.sh >/dev/null 2>&1
  run_in "$sb" env "${common_env[@]}" bash scripts/release/manifest.sh >/dev/null 2>&1

  : >"$log"
  out="$(run_in "$sb" env "${common_env[@]}" bash scripts/release/publish-github.sh)"
  rc=$?
  check "dry run succeeds" test "$rc" -eq 0
  check_contains "says nothing was published" "$out" "DRY RUN"
  check_contains "shows the exact plan" "$out" "tag:          v1.0.0"
  check_not_contains "no release was created" "$(cat "$log" 2>/dev/null)" "gh:release-create"
  tags="$( cd "$sb" && git tag -l )"
  check "no tag was created" test -z "$tags"
  rm -rf "$sb"
  end_case
fi

# --- T17 -------------------------------------------------------------------
if test_case T17 "hygiene blocks committed debris, keys and credential-shaped strings"; then
  sb="$(make_sandbox 1.0.0)"
  printf 'junk\n' >"$sb/.DS_Store"
  mkdir -p "$sb/secrets"
  printf -- '-----BEGIN RSA PRIVATE KEY-----\nZmFrZQ==\n' >"$sb/secrets/dev.pem"
  ( cd "$sb" && git add -f .DS_Store secrets/dev.pem && git commit -q -m junk )
  out="$(run_in "$sb" env ALLOW_DIRTY=1 bash scripts/release/check-tree-hygiene.sh)"
  rc=$?
  check "hygiene fails" test "$rc" -ne 0
  check_contains "flags the OS debris" "$out" ".DS_Store"
  check_contains "flags the key file" "$out" "secrets/dev.pem"
  rm -rf "$sb"
  end_case
fi

# --- T18 -------------------------------------------------------------------
if test_case T18 "a dirty working tree blocks a public release"; then
  sb="$(make_sandbox 1.0.0 --with-build)"
  printf 'uncommitted change\n' >>"$sb/RELEASE_NOTES.md"
  out="$(run_in "$sb" env RELEASE_MODE=public \
        SENDBLOOM_ARTIFACT_CONTRACT=bundles-in-dmg \
        DEVELOPER_ID_APPLICATION="Developer ID Application: Fake Maintainer (TEAM123456)" \
        FAKE_APP_IDENTITY="Developer ID Application: Fake Maintainer (TEAM123456)" \
        NOTARY_PROFILE=fake-profile FAKE_STATE="$sb/fakestate" \
        bash scripts/release/package-macos.sh)"
  rc=$?
  check "packaging refuses" test "$rc" -ne 0
  check_contains "explains why" "$out" "dirty working tree"
  rm -rf "$sb"
  end_case
fi

# ===========================================================================
printf '%s\n' "----------------------------------------"
printf 'passed: %d   failed: %d   skipped: %d\n' "$pass_count" "$fail_count" "$skip_count"
if [[ "$fail_count" -ne 0 ]]; then
  printf '%sfailed cases:%s\n' "$c_red" "$c_off"
  printf '  %s\n' "${FAILED[@]}"
  exit 1
fi
printf '%srelease-script tests: PASS%s\n' "$c_grn" "$c_off"
