    cmake_minimum_required(VERSION 3.15)

    project(OB-Xd VERSION 2.11.0 LANGUAGES C CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)

    include(FetchContent)
    FetchContent_Declare(
      JUCE
      GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
      GIT_TAG        master
    )
    FetchContent_MakeAvailable(JUCE)

    Include (FetchContent)
    FetchContent_Declare (melatonin_inspector
            GIT_REPOSITORY https://github.com/sudara/melatonin_inspector.git
            GIT_TAG origin/main
            SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/melatonin_inspector)
    FetchContent_MakeAvailable (melatonin_inspector)

    juce_add_plugin(OB-Xd
        COMPANY_NAME "sanctuarydsp"
        BUNDLE_ID "com.sanctuarydsp.Obxd"
        PLUGIN_MANUFACTURER_CODE SDSP
        PLUGIN_CODE Obxd
        FORMATS AU VST3 Standalone
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
        Source/Components/ScaleComponent.cpp
        Source/Components/PresetBar.cpp
        Source/Components/SetPresetNameWindow.cpp
        Source/MTS/libMTSClient.cpp
    )

    target_include_directories(OB-Xd PRIVATE
        Source
        Source/Components
        Source/Engine
        Source/Gui
        Source/MTS
    )


    target_link_libraries(OB-Xd PRIVATE melatonin_inspector
        BinaryData
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_core
        juce::juce_gui_basics
        juce::juce_audio_utils
        juce::juce_audio_devices

    )

    if(MSVC)
        target_compile_options(OB-Xd PRIVATE /W4 /WX)
    else()
        target_compile_options(OB-Xd PRIVATE -Wall -Wextra -Wpedantic -Werror)
    endif()

    add_subdirectory(Images)

    target_compile_definitions(OB-Xd PRIVATE JUCE_MODAL_LOOPS_PERMITTED=1)
