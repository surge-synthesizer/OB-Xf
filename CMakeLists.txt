    cmake_minimum_required(VERSION 3.15)

    project(OB-Xd VERSION 2.11.0 LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    option(ENABLE_ASAN "Enable Address Sanitizer" OFF)
    option(ENABLE_TSAN "Enable Thread Sanitizer" OFF)
    option(ENABLE_ASIO "Enable ASIO support" OFF)

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

    if(ENABLE_ASIO)
        if(WIN32)
            if(DEFINED ENV{ASIO_SDK_HOME})
                set(ASIO_HOME $ENV{ASIO_SDK_HOME})
                if(EXISTS "${ASIO_HOME}/common/iasiodrv.h")
                    message(STATUS "ASIO SDK found at: ${ASIO_HOME}")
                    add_compile_definitions(JUCE_ASIO=1)
                else()
                    message(WARNING "ASIO headers not found at ${ASIO_HOME}, disabling ASIO")
                    set(ENABLE_ASIO OFF)
                    add_compile_definitions(JUCE_ASIO=0)
                endif()
            else()
                message(WARNING "ASIO_SDK_HOME not set, disabling ASIO")
                set(ENABLE_ASIO OFF)
                add_compile_definitions(JUCE_ASIO=0)
            endif()
        else()
            message(STATUS "ASIO support only available on Windows")
            set(ENABLE_ASIO OFF)
            add_compile_definitions(JUCE_ASIO=0)
        endif()
    else()
        add_compile_definitions(JUCE_ASIO=0)
    endif()

    add_compile_definitions(JUCE_WEB_BROWSER=0)
    add_compile_definitions(JUCE_USE_CURL=0)

    include(FetchContent)
    FetchContent_Declare(
      JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG        master
    )
    FetchContent_MakeAvailable(JUCE)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        Include (FetchContent)
        FetchContent_Declare(melatonin_inspector
                GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
                GIT_TAG origin/main
                SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)
        FetchContent_MakeAvailable(melatonin_inspector)
    endif()

    FetchContent_Declare(
            clap_juce_extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            GIT_TAG main
    )
    FetchContent_MakeAvailable(clap_juce_extensions)

    juce_add_plugin(OB-Xd
        COMPANY_NAME "sanctuarydsp"
        BUNDLE_ID "com.sanctuarydsp.Obxd"
        LV2URI "urn:com.sanctuarydsp.Obxd"
        PLUGIN_MANUFACTURER_CODE SDSP
        PLUGIN_CODE Obxd
        FORMATS AU VST3 LV2 Standalone
        PRODUCT_NAME "OBXD"
        IS_SYNTH TRUE
        NEEDS_MIDI_INPUT TRUE
        NEEDS_MIDI_OUTPUT FALSE
        IS_MIDI_EFFECT FALSE
        COPY_PLUGIN_AFTER_BUILD TRUE
    )

    target_sources(OB-Xd PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Utils.cpp
        Source/Utilities/KeyCommandHandler.cpp
        Source/Components/ScaleComponent.cpp
        Source/Components/PresetBar.cpp
        Source/Components/SetPresetNameWindow.cpp
        Source/MTS/libMTSClient.cpp
        Source/State/StateManager.cpp
        Source/Midi/MidiHandler.cpp
        Source/Parameter/ParameterManager.cpp
    )

    target_include_directories(OB-Xd PRIVATE
        Source
        Source/Components
        Source/Engine
        Source/Gui
        Source/MTS
        Source/Interface
        Source/Core
        Source/Midi
        Source/Utilities
        Source/Parameter
        Source/State
        $<$<BOOL:${ENABLE_ASIO}>:${ASIO_HOME}/common>
    )


    target_link_libraries(OB-Xd PRIVATE
        $<$<CONFIG:Debug>:melatonin_inspector>
        BinaryData
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_audio_utils
        juce::juce_audio_devices

    )

    if(MSVC)
        target_compile_options(OB-Xd PRIVATE /W2)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(OB-Xd PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Werror
                -Wno-language-extension-token
                $<$<CONFIG:Release>:-O2>
                $<$<AND:$<CONFIG:Release>,$<NOT:$<AND:$<PLATFORM_ID:Darwin>,$<STREQUAL:${CMAKE_SYSTEM_PROCESSOR},arm64>>>>:-march=x86-64-v3>
        )
    else()
        target_compile_options(OB-Xd PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_subdirectory(Images)

    clap_juce_extensions_plugin(TARGET OB-Xd
            CLAP_ID "com.sanctuarydsp.Obxd"
            CLAP_FEATURES instrument synthesizer "virtual analog" analog)

    target_compile_definitions(OB-Xd PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)
