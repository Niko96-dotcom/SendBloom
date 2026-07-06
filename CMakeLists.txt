cmake_minimum_required(VERSION 3.25)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(PamplejuceVersion)

include(CPM)
include(PamplejuceMacOS)
include(JUCEDefaults)

set(PROJECT_NAME "SendBloom")
set(PRODUCT_NAME "SendBloom")
set(COMPANY_NAME "Niko Audio Labs")
set(BUNDLE_ID "com.nikoaudiolabs.sendbloom")
set(FORMATS AU VST3)

set(CMAKE_POLICY_DEFAULT_CMP0141        NEW      CACHE STRING "" FORCE)
set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT Embedded CACHE STRING "" FORCE)

project(${PROJECT_NAME} VERSION ${CURRENT_VERSION})

include(PamplejuceLog)

add_subdirectory(JUCE)

juce_add_plugin("${PROJECT_NAME}"
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE NkMo
    PLUGIN_CODE SbLm
    FORMATS "${FORMATS}"
    PRODUCT_NAME "${PRODUCT_NAME}")

add_library(SharedCode INTERFACE)

include(SharedCodeDefaults)
target_compile_features(SharedCode INTERFACE cxx_std_20)

file(GLOB_RECURSE SourceFiles CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/source/*.h")
target_sources(SharedCode INTERFACE ${SourceFiles})

include(XcodePrettify)

target_compile_definitions(SharedCode
    INTERFACE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    CMAKE_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
    VERSION="${CURRENT_VERSION}"
    PRODUCT_NAME_WITHOUT_VERSION="SendBloom")

target_link_libraries(SharedCode
    INTERFACE
    juce_audio_utils
    juce_audio_processors
    juce_dsp
    juce_gui_basics
    juce_gui_extra
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

target_link_libraries("${PROJECT_NAME}" PRIVATE SharedCode)

# include(PamplejuceIPP)

include(Tests)
include(GitHubENV)
