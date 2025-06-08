
    include(FetchContent)
    FetchContent_Declare(
            JUCE
            GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
            GIT_TAG        8.0.7
    )
    FetchContent_MakeAvailable(JUCE)

    FetchContent_Declare(
            sstplugininfra
            GIT_REPOSITORY https://github.com/surge-synthesizer/sst-plugininfra
            GIT_TAG main
    )

    FetchContent_MakeAvailable(sstplugininfra)
    include(${sstplugininfra_SOURCE_DIR}/cmake/git-version-functions.cmake)
    version_from_versionfile_or_git()
    message(STATUS "DISPLAY_VERSION=${GIT_IMPLIED_DISPLAY_VERSION}; COMMIT_HASH=${GIT_COMMIT_HASH}; BRANCH=${GIT_BRANCH}")

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


