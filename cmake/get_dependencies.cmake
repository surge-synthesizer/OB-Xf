
    include(FetchContent)

    FetchContent_Declare(
            sst-cmake
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-cmake.git
            GIT_TAG        2b4a585aa6d80e8773fcff62831c1b10e2a5d20d
    )
    FetchContent_MakeAvailable(sst-cmake)

    FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.7
    )
    FetchContent_MakeAvailable(JUCE)


    FetchContent_Declare(
            fmt
            GIT_REPOSITORY https://github.com/fmtlib/fmt.git
            GIT_TAG 10.2.0
    )

    FetchContent_MakeAvailable(fmt)


    FetchContent_Declare(
            simde
            GIT_REPOSITORY https://github.com/simd-everywhere/simde
            GIT_TAG v0.8.2
    )

    FetchContent_MakeAvailable(simde)
    add_library(simde INTERFACE)
    target_include_directories(simde INTERFACE ${simde_SOURCE_DIR})


    FetchContent_Declare(
            sstcpp
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-cpputils
            GIT_TAG main
    )

    FetchContent_MakeAvailable(sstcpp)

    FetchContent_Declare(
            sstbb
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-basic-blocks
            GIT_TAG main
    )

    FetchContent_MakeAvailable(sstbb)

    FetchContent_Declare(
            sstbb
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-basic-blocks
            GIT_TAG main
    )

    FetchContent_MakeAvailable(sstbb)

    FetchContent_Declare(
            sstplugininfra
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-plugininfra
            GIT_TAG main
    )

    FetchContent_MakeAvailable(sstplugininfra)
    include(${sstplugininfra_SOURCE_DIR}/cmake/git-version-functions.cmake)
    version_from_versionfile_or_git()
    message(STATUS "DISPLAY_VERSION=${GIT_IMPLIED_DISPLAY_VERSION}; COMMIT_HASH=${GIT_COMMIT_HASH}; BRANCH=${GIT_BRANCH}")

    # Fetch order means the main cmake runs before git so ugh
    configure_file(${sstplugininfra_SOURCE_DIR}/src/version_information_in.cpp ${CMAKE_BINARY_DIR}/obxfi/version_information.cpp)
    file(COPY ${CMAKE_BINARY_DIR}/obxfi/version_information.cpp
            DESTINATION ${CMAKE_BINARY_DIR}/geninclude/)
    add_library(obxf_version_information STATIC)
    target_sources(obxf_version_information PRIVATE  ${CMAKE_BINARY_DIR}/obxfi/version_information.cpp)
    target_include_directories(obxf_version_information PUBLIC ${sstplugininfra_SOURCE_DIR}/include)
    target_compile_definitions(obxf_version_information PUBLIC SST_PLUGININFRA_HAS_VERSION_INFORMATION=1)

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



    FetchContent_Declare(
            mts_esp
            GIT_REPOSITORY https://github.com/oddsound/MTS-ESP/
            GIT_TAG main
    )
    FetchContent_MakeAvailable(mts_esp)
    add_library(mts-client STATIC ${mts_esp_SOURCE_DIR}/Client/libMTSClient.cpp)
    target_include_directories(mts-client PUBLIC ${mts_esp_SOURCE_DIR}/Client)


