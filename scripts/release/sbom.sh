#!/usr/bin/env bash
# Dependency and licence inventory (CycloneDX 1.5 JSON).
#
# SendBloom links a small, fully pinned set of third-party sources, all of
# them resolved at configure time by CPM or by git submodule. This produces a
# machine-readable inventory of exactly those pins so a consumer can answer
# "what is inside this binary and under what licence" without reading CMake.

set -euo pipefail
source "$(dirname "${BASH_SOURCE[0]}")/lib.sh"

release_init
require_cmd python3 git

mkdir -p "$DIST_DIR"

step "SBOM / dependency inventory"

# Every pin lookup is best-effort about *availability* but never about
# correctness: a missing pin surfaces as "unknown" in the SBOM rather than
# aborting the release, and the licence-citation check below is what actually
# fails closed.
submodule_commit() {
  git -C "$ROOT" submodule status "$1" 2>/dev/null \
    | awk '{gsub(/^[-+U]/,"",$1); print $1}' || true
}

cpm_pin() {
  # Pull the pinned ref out of the CPM lock file the release build produced,
  # falling back to the declaration in cmake-local when no build is present.
  local name="$1"
  local lock="$RELEASE_BUILD_DIR/cpm-package-lock.cmake"
  if [[ -f "$lock" ]]; then
    awk -v n="$name" 'BEGIN{IGNORECASE=1} $0 ~ "NAME " n {found=1} found && /GIT_TAG|VERSION/ {print; exit}' "$lock" \
      | sed -E 's/.*(GIT_TAG|VERSION)[[:space:]]+([^[:space:])]+).*/\2/'
  fi
}

JUCE_COMMIT="$(submodule_commit JUCE)"
JUCE_TAG="$( { git -C "$ROOT/JUCE" describe --tags --exact-match 2>/dev/null \
             || git config -f "$ROOT/.gitmodules" submodule.JUCE.branch 2>/dev/null \
             || echo unknown; } | head -n1 )"
R8BRAIN_PIN="$( { grep -oE 'GIT_TAG[[:space:]]+[0-9a-f]{40}' "$ROOT/cmake-local/R8brain.cmake" 2>/dev/null || true; } | awk '{print $2}' | head -n1 )"
CATCH2_PIN="$( { grep -oE 'catchorg/Catch2@[0-9.]+' "$ROOT/cmake/Tests.cmake" 2>/dev/null || true; } | cut -d@ -f2 | head -n1 )"

: "${JUCE_COMMIT:=unknown}"
: "${JUCE_TAG:=unknown}"
: "${R8BRAIN_PIN:=unknown}"
: "${CATCH2_PIN:=unknown}"

python3 - "$SBOM_PATH" <<PY
import json, sys, datetime

out = sys.argv[1]

def component(name, version, purl, licence, description, kind="library"):
    c = {
        "type": kind,
        "name": name,
        "version": version or "unknown",
        "description": description,
        "purl": purl,
    }
    if licence:
        c["licenses"] = [{"license": {"id": licence}}]
    return c

doc = {
    "bomFormat": "CycloneDX",
    "specVersion": "1.5",
    "version": 1,
    "metadata": {
        "timestamp": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
        "component": {
            "type": "application",
            "name": "SendBloom",
            "version": "$RELEASE_VERSION",
            "description": "Gated dirty ambience guitar effect (AU / VST3)",
            "licenses": [{"license": {"id": "MIT"}}],
            "publisher": "Niko Audio Labs",
        },
        "properties": [
            {"name": "sendbloom:build_id", "value": "$BUILD_ID"},
            {"name": "sendbloom:source_commit", "value": "$SOURCE_COMMIT"},
        ],
    },
    "components": [
        component(
            "JUCE", "$JUCE_TAG",
            "pkg:github/juce-framework/JUCE@$JUCE_COMMIT",
            None,
            "Audio application framework. Dual-licensed; SendBloom ships under the commercial path recorded in docs/LICENSING_DECISION.md.",
            kind="framework",
        ),
        component(
            "r8brain-free-src", "$R8BRAIN_PIN",
            "pkg:github/avaneev/r8brain-free-src@$R8BRAIN_PIN",
            "MIT",
            "High-quality sample rate conversion used by FixedRateAdapter for the host <-> 32,768 Hz sandwich.",
        ),
        component(
            "Catch2", "$CATCH2_PIN",
            "pkg:github/catchorg/Catch2@v$CATCH2_PIN",
            "BSL-1.0",
            "Test framework. Build-time and test-time only; not linked into shipped artifacts.",
        ),
    ],
}

with open(out, "w") as fh:
    json.dump(doc, fh, indent=2)
    fh.write("\n")
PY

python3 -c 'import json,sys; json.load(open(sys.argv[1]))' "$SBOM_PATH" \
  || die "generated SBOM is not valid JSON"

info "JUCE:            $JUCE_TAG ($JUCE_COMMIT)"
info "r8brain:         ${R8BRAIN_PIN:-unknown}"
info "Catch2:          ${CATCH2_PIN:-unknown} (test only, not shipped)"
ok "wrote $SBOM_PATH"

# Licence citations must exist for everything that ships.
step "Licence citations"
for dep in r8brain JUCE; do
  grep -qi "$dep" "$ROOT/docs/THIRD_PARTY_LICENSES.md" \
    || die "$dep is not cited in docs/THIRD_PARTY_LICENSES.md"
  ok "$dep cited in docs/THIRD_PARTY_LICENSES.md"
done

echo
printf '%ssbom: PASS%s  %s\n' "$_c_grn" "$_c_off" "$SBOM_PATH"
