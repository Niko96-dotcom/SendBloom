# SendBloom canonical version parser.
#
# The repository has exactly ONE release version truth source: the top-level
# VERSION file. It holds a full semver string and may carry a prerelease
# label, e.g.
#
#     X.Y.Z
#     X.Y.Z-rcN
#
# Apple bundle metadata and CMake's project(VERSION ...) only accept the
# numeric core, so this module splits the canonical string into:
#
#   CURRENT_VERSION            numeric core X.Y.Z   (CMake + JUCE + plists)
#   SENDBLOOM_RELEASE_VERSION  full canonical string, prerelease included
#   SENDBLOOM_PRERELEASE       prerelease label, or empty for a stable release
#
# Nothing else in the tree may hard-code a release number. scripts/release/
# derives artifact names, installer metadata, checksum filenames, release
# note titles and docs examples from these values, and
# scripts/release/check-version-consistency.sh fails the release if any
# product-facing number drifts.
#
# This replaces the upstream PamplejuceVersion module (which cannot parse a
# prerelease label). The auto-patch-bump option is deliberately not carried
# over: an automatic version is not a truth source.

# Build system depends on this file, copy it into the build dir.
configure_file(VERSION VERSION COPYONLY)

file(STRINGS "${CMAKE_CURRENT_BINARY_DIR}/VERSION" SENDBLOOM_RELEASE_VERSION LIMIT_COUNT 1)
string(STRIP "${SENDBLOOM_RELEASE_VERSION}" SENDBLOOM_RELEASE_VERSION)

if (SENDBLOOM_RELEASE_VERSION STREQUAL "")
    message(FATAL_ERROR "VERSION file is empty; it is the canonical release version source")
endif ()

# major.minor.patch[-prerelease] — prerelease is dot-separated alphanumerics.
if (NOT SENDBLOOM_RELEASE_VERSION MATCHES
        "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-([0-9A-Za-z]+(\\.[0-9A-Za-z]+)*))?$")
    message(FATAL_ERROR
        "VERSION must be MAJOR.MINOR.PATCH with an optional -prerelease label, got: '${SENDBLOOM_RELEASE_VERSION}'")
endif ()

set(CURRENT_VERSION "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
set(SENDBLOOM_PRERELEASE "${CMAKE_MATCH_5}")
set(MAJOR_VERSION "${CMAKE_MATCH_1}")

if (SENDBLOOM_PRERELEASE STREQUAL "")
    message(STATUS "SendBloom version: ${CURRENT_VERSION} (stable)")
else ()
    message(STATUS "SendBloom version: ${CURRENT_VERSION} (prerelease ${SENDBLOOM_PRERELEASE})")
endif ()
