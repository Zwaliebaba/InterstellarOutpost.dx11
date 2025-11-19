# Installation and Packaging Configuration
# CPack configuration for creating installers

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT DEFINED INTERSTELLAROUTPOST_EXPORT_NAME)
    set(INTERSTELLAROUTPOST_EXPORT_NAME "InterstellarOutpostTargets")
endif()

# Installation and Packaging Configuration
# CPack configuration for creating installers

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(NOT DEFINED INTERSTELLAROUTPOST_EXPORT_NAME)
    set(INTERSTELLAROUTPOST_EXPORT_NAME "InterstellarOutpostTargets")
endif()

if(NOT DEFINED INTERSTELLAROUTPOST_INSTALL_BINDIR)
    set(INTERSTELLAROUTPOST_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
endif()

if(NOT DEFINED INTERSTELLAROUTPOST_INSTALL_LIBDIR)
    set(INTERSTELLAROUTPOST_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
endif()

if(NOT DEFINED INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR)
    set(INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
endif()

if(NOT DEFINED INTERSTELLAROUTPOST_INSTALL_DATADIR)
    set(INTERSTELLAROUTPOST_INSTALL_DATADIR "${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}")
endif()

# Installation directories
set(INSTALL_BINDIR ${INTERSTELLAROUTPOST_INSTALL_BINDIR})
set(INSTALL_LIBDIR ${INTERSTELLAROUTPOST_INSTALL_LIBDIR})
set(INSTALL_INCLUDEDIR ${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR})
set(INSTALL_DATADIR ${INTERSTELLAROUTPOST_INSTALL_DATADIR})
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# CPack configuration
set(CPACK_PACKAGE_NAME "${PROJECT_NAME}")
set(CPACK_PACKAGE_VENDOR "InterstellarOutpost Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "${PROJECT_NAME}")

# Optional resource files
if(EXISTS "${CMAKE_SOURCE_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/README.md")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
endif()

# Platform-specific packaging
if(WIN32)
    find_program(_io_nsis_makensis NAMES makensis)
    if(_io_nsis_makensis)
        set(CPACK_GENERATOR "NSIS;ZIP")
        set(CPACK_NSIS_MAKENSIS_EXECUTABLE "${_io_nsis_makensis}")
    else()
        message(WARNING "NSIS makensis executable not found; generating ZIP package only.")
        set(CPACK_GENERATOR "ZIP")
    endif()
else()
    set(CPACK_GENERATOR "ZIP")
endif()

if(WIN32 AND _io_nsis_makensis)
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
    set(CPACK_NSIS_MODIFY_PATH ON)
    set(CPACK_NSIS_DISPLAY_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_PACKAGE_NAME "${PROJECT_NAME}")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/yourusername/${PROJECT_NAME}")

    set(CPACK_NSIS_CREATE_ICONS_EXTRA "")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA "")
    foreach(_io_name IN ITEMS "${APP_CLIENT_OUTPUT_NAME}" "${APP_SERVER_OUTPUT_NAME}")
        if(NOT _io_name STREQUAL "")
            string(APPEND CPACK_NSIS_CREATE_ICONS_EXTRA
                "CreateShortCut '$DESKTOP\\\\${_io_name}.lnk' '$INSTDIR\\\\bin\\\\${_io_name}.exe'\n")
            string(APPEND CPACK_NSIS_DELETE_ICONS_EXTRA
                "Delete '$DESKTOP\\\\${_io_name}.lnk'\n")
        endif()
    endforeach()
endif()

# Components
set(CPACK_COMPONENTS_ALL Client Server Libraries Headers Symbols)

set(CPACK_COMPONENT_CLIENT_DISPLAY_NAME "Client Application")
set(CPACK_COMPONENT_SERVER_DISPLAY_NAME "Dedicated Server")
set(CPACK_COMPONENT_SHARED_DATA_DISPLAY_NAME "Game Data")
set(CPACK_COMPONENT_LIBRARIES_DISPLAY_NAME "Runtime Libraries")
set(CPACK_COMPONENT_HEADERS_DISPLAY_NAME "C++ Headers")
set(CPACK_COMPONENT_SYMBOLS_DISPLAY_NAME "Debug Symbols")

set(CPACK_COMPONENT_CLIENT_DESCRIPTION "DirectX 12 client executable and dependencies")
set(CPACK_COMPONENT_SERVER_DESCRIPTION "Headless simulation/server executable")
set(CPACK_COMPONENT_SHARED_DATA_DESCRIPTION "Packaged gameplay data and assets")
set(CPACK_COMPONENT_LIBRARIES_DESCRIPTION "Reusable engine libraries")
set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Development headers and CMake package files")
set(CPACK_COMPONENT_SYMBOLS_DESCRIPTION "PDB symbol files for diagnostics")

set(CPACK_COMPONENT_CLIENT_DEPENDS Libraries)
set(CPACK_COMPONENT_SERVER_DEPENDS Libraries)
set(CPACK_COMPONENT_SYMBOLS_DEPENDS Client;Server)
set(CPACK_COMPONENT_SYMBOLS_HIDDEN TRUE)

set(CPACK_ARCHIVE_COMPONENT_INSTALL ON)

# Export and package configuration
set(_io_config_in "${CMAKE_CURRENT_LIST_DIR}/InterstellarOutpostConfig.cmake.in")
set(_io_config_out "${CMAKE_CURRENT_BINARY_DIR}/InterstellarOutpostConfig.cmake")
set(_io_version_out "${CMAKE_CURRENT_BINARY_DIR}/InterstellarOutpostConfigVersion.cmake")

configure_package_config_file(
    "${_io_config_in}"
    "${_io_config_out}"
    INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
)

write_basic_package_version_file(
    "${_io_version_out}"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    NAMESPACE InterstellarOutpost::
    DESTINATION ${INSTALL_CONFIGDIR}
    FILE InterstellarOutpostTargets.cmake
    COMPONENT Headers
)

install(FILES
    "${_io_config_out}"
    "${_io_version_out}"
    DESTINATION ${INSTALL_CONFIGDIR}
    COMPONENT Headers
)

if(MSVC)
    foreach(_io_symbol_target ${APP_CLIENT} ${APP_SERVER})
        if(TARGET ${_io_symbol_target})
            install(FILES "$<TARGET_PDB_FILE:${_io_symbol_target}>"
                DESTINATION ${INSTALL_BINDIR}
                CONFIGURATIONS Debug RelWithDebInfo
                COMPONENT Symbols
                OPTIONAL)
        endif()
    endforeach()
endif()

include(CPack)
