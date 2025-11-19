
# GameLogic – world simulation and gameplay entities

file(GLOB gamelogic_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB gamelogic_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(GameLogic ${gamelogic_sources} ${gamelogic_headers})
add_library(InterstellarOutpost::GameLogic ALIAS GameLogic)

target_include_directories(GameLogic
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/libs/NeuronCore>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/GameLogic>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronCore>
)

target_link_libraries(GameLogic
    PUBLIC
        InterstellarOutpost::NeuronCore
)

ApplyTargetDefaults(GameLogic)
EnablePch(GameLogic "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

install(TARGETS GameLogic
    EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    ARCHIVE DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INTERSTELLAROUTPOST_INSTALL_BINDIR}
    COMPONENT Libraries
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/GameLogic
    COMPONENT Headers
    FILES_MATCHING
        PATTERN "*.h"
)

set_target_properties(GameLogic PROPERTIES FOLDER "Libraries")
