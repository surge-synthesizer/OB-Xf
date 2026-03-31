
juce_add_console_app(obxf-tests PRODUCT_NAME "OB-Xf Tests")

target_sources(obxf-tests PRIVATE
    main.cpp
    osc.cpp
    filt.cpp
    env.cpp
    noise.cpp
    mpe.cpp
)

target_include_directories(obxf-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/engine
    ${CMAKE_SOURCE_DIR}/src/core
    ${CMAKE_SOURCE_DIR}/src/midi
    ${CMAKE_SOURCE_DIR}/src/MTS
    ${CMAKE_SOURCE_DIR}/src/interface
    ${CMAKE_SOURCE_DIR}/src/utilities
    ${CMAKE_SOURCE_DIR}/src/parameter
    ${CMAKE_SOURCE_DIR}/src/state
    ${CMAKE_SOURCE_DIR}/src/components
    ${CMAKE_SOURCE_DIR}/src/gui
)

target_link_libraries(obxf-tests PRIVATE
    catch2

    juce::juce_core
    juce::juce_audio_basics
    juce::juce_dsp
    juce::juce_graphics

    simde
    fmt
    sst-cpputils
    sst-basic-blocks
    sst-plugininfra

    mts-client
)

if(MSVC)
    target_compile_options(obxf-tests PRIVATE /W2)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(obxf-tests PRIVATE
        -Wall
        -Wno-language-extension-token
        -Wno-macro-redefined
    )
else()
    target_compile_options(obxf-tests PRIVATE -Wall)
endif()
