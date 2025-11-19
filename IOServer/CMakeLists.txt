# InterstellarOutpost Server executable and runtime wiring

file(GLOB app_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB app_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_executable(${APP_SERVER} ${app_sources} ${app_headers})

target_link_libraries(${APP_SERVER}
    PRIVATE
        InterstellarOutpost::NeuronCore
        InterstellarOutpost::NeuronServer
        InterstellarOutpost::GameLogic
)

target_include_directories(${APP_SERVER}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

ApplyTargetDefaults(${APP_SERVER})
EnablePch(${APP_SERVER} "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

set(_io_server_output_dir "${PROJECT_SOURCE_DIR}/bin/Server")
set_target_properties(${APP_SERVER} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${_io_server_output_dir}"
)

if(CMAKE_CONFIGURATION_TYPES)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${config}" config_upper)
        set_target_properties(${APP_SERVER} PROPERTIES
            "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${_io_server_output_dir}/${config}"
        )
    endforeach()
endif()

if(MSVC)
    if(CMAKE_CONFIGURATION_TYPES)
        foreach(config ${CMAKE_CONFIGURATION_TYPES})
            string(TOUPPER "${config}" config_upper)
            set_property(TARGET ${APP_SERVER} PROPERTY
                VS_DEBUGGER_WORKING_DIRECTORY_${config_upper} "${_io_server_output_dir}/${config}")
        endforeach()
    else()
        set_property(TARGET ${APP_SERVER} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${_io_server_output_dir}")
    endif()
endif()

set_target_properties(${APP_SERVER} PROPERTIES
    OUTPUT_NAME "${APP_SERVER_OUTPUT_NAME}"
    FOLDER "Applications"
)

set(_io_server_data_dir "${CMAKE_CURRENT_SOURCE_DIR}/GameData")
if(EXISTS "${_io_server_data_dir}")
    add_custom_command(TARGET ${APP_SERVER} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${_io_server_data_dir}"
            "$<TARGET_FILE_DIR:${APP_SERVER}>/GameData"
        COMMENT "Copying IOServer GameData assets"
    )

    install(DIRECTORY "${_io_server_data_dir}/"
        DESTINATION "${INTERSTELLAROUTPOST_INSTALL_SERVER_BINDIR}/GameData"
        COMPONENT Server
    )
endif()

install(TARGETS ${APP_SERVER}
    EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    RUNTIME DESTINATION ${INTERSTELLAROUTPOST_INSTALL_SERVER_BINDIR}
    COMPONENT Server
)
