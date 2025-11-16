cmake_minimum_required(VERSION 3.15)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.14 CACHE STRING "Build for 10.14")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TODO Remove This before we ship non beta version. Remember
# the AU rescans only when versions change even if binaries are
# different.
string(TIMESTAMP DAY_OF_YEAR "%j")
string(TIMESTAMP YEAR "%Y")
math(EXPR PART0 "${YEAR}-2025 + 8")
math(EXPR PART1 "${DAY_OF_YEAR} + 1")

project(OB-Xf VERSION 0.${PART0}.${PART1} LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

message(STATUS "OB-Xf CMake Build: Version=${PROJECT_VERSION}")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)
option(COPY_PLUGIN_AFTER_BUILD "Copy plugin after build is complete" OFF)

include(libs/sst/sst-plugininfra/cmake/git-version-functions.cmake)
version_from_versionfile_or_git()
message(STATUS "DISPLAY_VERSION=${GIT_IMPLIED_DISPLAY_VERSION}; COMMIT_HASH=${GIT_COMMIT_HASH}; BRANCH=${GIT_BRANCH}")

add_subdirectory(libs)
message(STATUS "After libs, MODULE_PATH is ${CMAKE_MODULE_PATH}")
version_from_versionfile_or_git()

if(ENABLE_ASAN AND ENABLE_TSAN)
    message(FATAL_ERROR "Cannot enable both AddressSanitizer and ThreadSanitizer at the same time")
endif()

if(ENABLE_ASAN)
    message(STATUS "Building with AddressSanitizer enabled")

    if(MSVC)
        add_compile_options(/fsanitize=address)
        message(STATUS "Note: For MSVC, you may need to set _NO_DEBUG_HEAP=1 environment variable")
    else()
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address)
    endif()
endif()

if(ENABLE_TSAN)
    if(WIN32)
        message(WARNING "ThreadSanitizer is not supported on Windows. Disabling TSAN.")
        set(ENABLE_TSAN OFF)
    else()
        message(STATUS "Building with ThreadSanitizer enabled")
        add_compile_options(-fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)

        if(CMAKE_BUILD_TYPE STREQUAL "Debug")
            message(STATUS "Setting optimization level for ThreadSanitizer")
            add_compile_options(-O1)
        endif()
    endif()
endif()

set(ENABLE_ASIO OFF)
set(SSTPI_ASIOSDK "${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/libs/asiosdk")

if(EXISTS "${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/libs/asiosdk")
    if(WIN32)
        message(STATUS "ASIO SDK located in sst-plugininfra. Enabling!")
        add_compile_definitions(JUCE_ASIO=1)
        set(ENABLE_ASIO ON)
    else()
        message(STATUS "ASIO is only available on Windows!")
        add_compile_definitions(JUCE_ASIO=0)
    endif()
else()
    message(STATUS "ASIO SDK not found! Disabling ASIO support.")
endif()

add_compile_definitions(JUCE_WEB_BROWSER=0)
add_compile_definitions(JUCE_USE_CURL=0)

list(APPEND OBXF_JUCE_FORMATS VST3 Standalone)

if(APPLE)
    list(APPEND OBXF_JUCE_FORMATS AU)
endif()

if(UNIX AND NOT APPLE)
    list(APPEND OBXF_JUCE_FORMATS LV2)
endif()

juce_add_plugin(OB-Xf
    COMPANY_NAME "Surge Synth Team"
    BUNDLE_ID "org.surge-synth-team.OB-Xf-rework"
    LV2URI "urn:org.surge-synth-team.OB-Xf-rework"
    PLUGIN_MANUFACTURER_CODE SSTx
    PLUGIN_CODE OBXR
    FORMATS ${OBXF_JUCE_FORMATS}
    ICON_BIG "assets/binary/icon.svg"
    PRODUCT_NAME "OB-Xf (Rework)"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_AFTER_BUILD}
    JUCE_VST3_CAN_REPLACE_VST2 FALSE
    VST3_AUTO_MANIFEST FALSE
)

target_sources(OB-Xf PRIVATE
    src/PluginProcessor.cpp
    src/PluginEditor.cpp
    src/Utils.cpp
    src/utilities/KeyCommandHandler.cpp
    src/state/StateManager.cpp
    src/midi/MidiHandler.cpp
    src/parameter/ParameterManager.cpp
    src/parameter/ParameterAdapter.cpp
    src/components/ScalingImageCache.cpp
)

target_include_directories(OB-Xf PRIVATE
    src
    src/components
    src/engine
    src/gui
    src/MTS
    src/interface
    src/core
    src/midi
    src/utilities
    src/parameter
    src/state
    $<$<BOOL:${ENABLE_ASIO}>:${SSTPI_ASIOSDK}/common>
)

target_link_libraries(OB-Xf PRIVATE
    $<$<CONFIG:Debug>:melatonin_inspector>
    juce::juce_audio_basics
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_gui_basics
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_dsp
    BinaryData

    mts-client

    version_information

    simde
    fmt
    sst-cpputils
    sst-basic-blocks

    sst-plugininfra::filesystem
    sst-plugininfra
)

target_compile_definitions(OB-Xf PRIVATE JUCE_VST3_CAN_REPLACE_VST2=0 OBXF_VERSION_STR="${PROJECT_VERSION}")

if(MSVC)
    target_compile_options(OB-Xf PRIVATE /W2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(OB-Xf PRIVATE
        -Wall
        # -Wextra
        # -Wpedantic
        -Werror
        -Wno-language-extension-token
        -Wno-macro-redefined
    )
else()
    if(${CMAKE_CXX_COMPILER_VERSION} LESS 13)
        message(STATUS "gcc version < 13; dont run werror in release.")
        add_compile_options($<$<CONFIG:Release>:-w>)
    else()
        target_compile_options(OB-Xf PRIVATE -Wall -Werror) # -Wextra -Wpedantic -Werror)
    endif()
endif()

clap_juce_extensions_plugin(TARGET OB-Xf
    CLAP_ID "org.surge-synth-team.OB-Xf-rework"
    CLAP_FEATURES instrument synthesizer "virtual analog" analog)

target_compile_definitions(OB-Xf PRIVATE)

add_subdirectory(assets)

include(basic_installer)
