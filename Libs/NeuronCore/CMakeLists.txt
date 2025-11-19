# NeuronCore – core engine utilities and shared systems

file(GLOB neuroncore_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB neuroncore_headers CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.h")

add_library(NeuronCore ${neuroncore_sources} ${neuroncore_headers})
add_library(InterstellarOutpost::NeuronCore ALIAS NeuronCore)

target_include_directories(NeuronCore
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronCore>
)

ApplyTargetDefaults(NeuronCore)
EnablePch(NeuronCore "${CMAKE_CURRENT_SOURCE_DIR}/pch.h")

install(TARGETS NeuronCore
    EXPORT ${INTERSTELLAROUTPOST_EXPORT_NAME}
    ARCHIVE DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${INTERSTELLAROUTPOST_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${INTERSTELLAROUTPOST_INSTALL_BINDIR}
    COMPONENT Libraries
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/
    DESTINATION ${INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR}/NeuronCore
    COMPONENT Headers
    FILES_MATCHING
        PATTERN "*.h"
)

find_package(cppwinrt CONFIG REQUIRED)
target_link_libraries(NeuronCore PUBLIC Microsoft::CppWinRT)

if(ENABLE_PIX_PROFILING)
    find_package(winpixevent CONFIG REQUIRED)
    target_link_libraries(NeuronCore PUBLIC Microsoft::WinPixEventRuntime)
    target_compile_definitions(NeuronCore PUBLIC HAVE_PIX)
endif()

set_target_properties(NeuronCore PROPERTIES FOLDER "Libraries")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${neuroncore_sources})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Header Files" FILES ${neuroncore_headers})
