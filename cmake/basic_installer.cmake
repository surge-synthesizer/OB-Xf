# A basic installer setup.
#
# This cmake file introduces two targets
#  obxd-gpl3-staged:      moves all the built assets to a well named directory
#  obxd-gpl3-installer:   depends on obxd-gpl3-staged, builds an installer
#
# Right now obxd-gpl3-installer builds just the crudest zip file but this is the target
# on which we will hang the proper installers later

set(OBXDGPL3_PRODUCT_DIR ${CMAKE_BINARY_DIR}/obxd_gpl3_products)
file(MAKE_DIRECTORY ${OBXDGPL3_PRODUCT_DIR})

add_custom_target(obxd-gpl3-staged)
add_custom_target(obxd-gpl3-installer)

set(TARGET_BASE OB-Xd)

function(obxd_gpl3_package format)
    get_target_property(output_dir ${TARGET_BASE} RUNTIME_OUTPUT_DIRECTORY)

    if(TARGET ${TARGET_BASE}_${format})
        add_dependencies(obxd-gpl3-staged ${TARGET_BASE}_${format})
        add_custom_command(
                TARGET obxd-gpl3-staged
                POST_BUILD
                WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
                COMMAND echo "Installing ${output_dir}/${format} to ${OBXDGPL3_PRODUCT_DIR}"
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${output_dir}/${format}/ ${OBXDGPL3_PRODUCT_DIR}/
        )
    endif()
endfunction()

obxd_gpl3_package(VST3)
obxd_gpl3_package(AU)
obxd_gpl3_package(CLAP)
obxd_gpl3_package(Standalone)

if (WIN32)
    message(STATUS "Including special windows cleanup installer stage")
    add_custom_command(TARGET obxd-gpl3-staged
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            COMMAND ${CMAKE_COMMAND} -E echo "Cleaning up windows goobits"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${OBXDGPL3_PRODUCT_DIR}/obxd-gpl3.exp"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${OBXDGPL3_PRODUCT_DIR}/obxd-gpl3.ilk"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${OBXDGPL3_PRODUCT_DIR}/obxd-gpl3.lib"
            COMMAND ${CMAKE_COMMAND} -E rm -f "${OBXDGPL3_PRODUCT_DIR}/obxd-gpl3.pdb"
            )
endif ()

add_dependencies(obxd-gpl3-installer obxd-gpl3-staged)

add_custom_command(
        TARGET obxd-gpl3-installer
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMAND echo "Installing ${output_dir}/${format} to ${OBXDGPL3_PRODUCT_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/LICENSE-gpl3 ${OBXDGPL3_PRODUCT_DIR}/
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/resources/installer/ZipReadme.txt ${OBXDGPL3_PRODUCT_DIR}/Readme.txt
)


find_package(Git)


string(TIMESTAMP OBXDGPL3_DATE "%Y-%m-%d")
set(OBXDGPL3_ZIP obxd-gpl3-${OBXDGPL3_DATE}-${GIT_COMMIT_HASH}-${CMAKE_SYSTEM_NAME}${ELFIN_EXTRA_ZIP_NAME}.zip)
message(STATUS "Zip File Name is ${OBXDGPL3_ZIP}")

if (APPLE)
    message(STATUS "Configuring for mac installer")
    add_custom_command(
            TARGET obxd-gpl3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_SOURCE_DIR}/libs/sst/sst-plugininfra/scripts/installer_mac/make_installer.sh "Elfin Controller" ${OBXDGPL3_PRODUCT_DIR} ${CMAKE_SOURCE_DIR}/resources/installer_mac ${CMAKE_BINARY_DIR}/installer "${OBXDGPL3_DATE}-${GIT_COMMIT_HASH}"
    )
elseif (WIN32)
    message(STATUS "Configuring for win installer")
    add_custom_command(
            TARGET obxd-gpl3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND 7z a -r installer/${OBXDGPL3_ZIP} ${OBXDGPL3_PRODUCT_DIR}/
            COMMAND ${CMAKE_COMMAND} -E echo "ZIP Installer in: installer/${OBXDGPL3_ZIP}")
    message(STATUS "Skipping NuGet for now")
    #find_program(OBXDGPL3_NUGET_EXE nuget.exe PATHS ENV "PATH")
    #if(OBXDGPL3_NUGET_EXE)
    #    message(STATUS "NuGet found at ${OBXDGPL3_NUGET_EXE}")
    #    add_custom_command(
    #        TARGET obxd-gpl3-installer
    #        POST_BUILD
    #        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    #        COMMAND ${OBXDGPL3_NUGET_EXE} install Tools.InnoSetup -version 6.2.1
    #        COMMAND Tools.InnoSetup.6.2.1/tools/iscc.exe /O"installer" /DOBXDGPL3_SRC="${CMAKE_SOURCE_DIR}" /DOBXDGPL3_BIN="${CMAKE_BINARY_DIR}" /DMyAppVersion="${OBXDGPL3_DATE}-${GIT_COMMIT_HASH}" "${CMAKE_SOURCE_DIR}/resources/installer_win/monique${BITS}.iss")
    #else()
    #    message(STATUS "NuGet not found")
    #endif()
else ()
    message(STATUS "Basic Installer: Target is installer/${OBXDGPL3_ZIP}")
    add_custom_command(
            TARGET obxd-gpl3-installer
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            COMMAND ${CMAKE_COMMAND} -E make_directory installer
            COMMAND ${CMAKE_COMMAND} -E tar cvf installer/${OBXDGPL3_ZIP} --format=zip ${OBXDGPL3_PRODUCT_DIR}/
            COMMAND ${CMAKE_COMMAND} -E echo "Installer in: installer/${OBXDGPL3_ZIP}")
endif ()
