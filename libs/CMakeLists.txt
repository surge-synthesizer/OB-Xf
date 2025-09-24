

add_subdirectory(sst/sst-cmake)

add_subdirectory(JUCE)
add_subdirectory(fmt)

add_library(simde INTERFACE)
target_include_directories(simde INTERFACE simde)

add_subdirectory(sst/sst-cpputils)
add_subdirectory(sst/sst-basic-blocks)
add_subdirectory(sst/sst-plugininfra)

add_subdirectory(melatonin_inspector)
add_subdirectory(clap-juce-extensions)

add_library(mts-client STATIC MTS-ESP/Client/libMTSClient.cpp)
target_include_directories(mts-client PUBLIC MTS-ESP/Client)
