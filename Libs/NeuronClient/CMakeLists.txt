# NeuronClient – presentation, audio, and input subsystems

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Graphics/Shaders)

file(GLOB neuronclient_sources CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB neuronclient_headers CONFIGURE_DEPENDS 
  "${CMAKE_CURRENT_SOURCE_DIR}/*.h"
)

add_library(NeuronClient ${neuronclient_sources} ${neuronclient_headers})
add_library(InterstellarOutpost::NeuronClient ALIAS NeuronClient)

target_include_directories(NeuronClient
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs/NeuronCore>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronClient>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronCore>
)

target_link_libraries(NeuronClient
    PUBLIC
        InterstellarOutpost::NeuronCore
)

ApplyTargetDefaults(NeuronClient)
EnablePch(NeuronClient "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

install(TARGETS NeuronClient
    EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    ARCHIVE DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INTERSTELLAROUTPOST_INSTALL_BINDIR}
    COMPONENT Libraries
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronClient
    COMPONENT Headers
    FILES_MATCHING
        PATTERN "*.h"
)

set_target_properties(NeuronClient PROPERTIES FOLDER "Libraries")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${neuronclient_sources})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${neuronclient_headers})
