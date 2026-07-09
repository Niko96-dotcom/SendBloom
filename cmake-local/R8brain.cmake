# Fetch r8brain-free-src (MIT) for ProperSRC host ↔ 32,768 Hz conversion.
# Kept in cmake-local/ so the sudara cmake-includes submodule stays unmodified.
# At GIT_TAG e71c31b the library is header-only (r8bbase.cpp removed upstream).

include(CPM)

CPMAddPackage(
    NAME r8brain
    GITHUB_REPOSITORY avaneev/r8brain-free-src
    GIT_TAG e71c31bf320f84210bb4bdcb57e296c39ce940f9
    DOWNLOAD_ONLY YES
)

if (r8brain_ADDED)
    add_library(r8brain INTERFACE)
    target_include_directories(r8brain INTERFACE "${r8brain_SOURCE_DIR}")
endif()
