project(obxfpy)

add_subdirectory(${CMAKE_SOURCE_DIR}/libs/pybind11 pybind11)
pybind11_add_module(${PROJECT_NAME})

target_sources(${PROJECT_NAME} PRIVATE
    ObxfPython.cpp
    ${OBXF_ENGINE_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:OB-Xf,INCLUDE_DIRECTORIES>
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    $<TARGET_PROPERTY:OB-Xf,COMPILE_DEFINITIONS>
    JUCE_HEADLESS_PLUGIN_CLIENT=1
    JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=0
    OBXF_HEADLESS
)

target_link_libraries(${PROJECT_NAME} PRIVATE obxf-engine-deps)

if(UNIX AND NOT APPLE)
    find_package(Threads REQUIRED)

    if(SKBUILD)
        target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads)
    else()
        target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads ${PYTHON_LIBRARIES})
    endif()

    if(CMAKE_SYSTEM_NAME MATCHES "BSD")
        target_link_libraries(${PROJECT_NAME} PRIVATE execinfo)
    endif()
endif()

if(SKBUILD)
    message(STATUS "Building Python package with scikit-build")
    target_compile_definitions(${PROJECT_NAME} PRIVATE SKBUILD)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "_")
    install(TARGETS ${PROJECT_NAME} DESTINATION ${PROJECT_NAME})
endif()