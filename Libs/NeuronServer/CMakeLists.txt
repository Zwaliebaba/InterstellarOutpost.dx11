# NeuronServer – server core functionality

file(GLOB neuronserver_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB neuronserver_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(NeuronServer ${neuronserver_sources} ${neuronserver_headers})
add_library(InterstellarOutpost::NeuronServer ALIAS NeuronServer)

target_include_directories(NeuronServer
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs/NeuronCore>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronServer>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronCore>
)

target_link_libraries(NeuronServer
    PUBLIC
        InterstellarOutpost::NeuronCore
)

ApplyTargetDefaults(NeuronServer)
EnablePch(NeuronServer "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

install(TARGETS NeuronServer
    EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    ARCHIVE DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INTERSTELLAROUTPOST_INSTALL_BINDIR}
    COMPONENT Libraries
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronServer
    COMPONENT Headers
    FILES_MATCHING
        PATTERN "*.h"
)

set_target_properties(NeuronServer PROPERTIES FOLDER "Libraries")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${neuronserver_sources})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${neuronserver_headers})
