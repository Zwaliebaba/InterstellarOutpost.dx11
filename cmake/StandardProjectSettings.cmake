
# Standard project settings shared by every target.

include_guard(GLOBAL)

# Choose a sensible default build type for single-config generators.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Defaulting CMAKE_BUILD_TYPE to RelWithDebInfo")
    set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo)
endif()

# Keep generated projects organised when opening in an IDE.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Normalise output directories so every generator produces the same layout.
set(_io_bin_dir "${PROJECT_SOURCE_DIR}/bin")
set(_io_lib_dir "${CMAKE_BINARY_DIR}/libs")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${_io_bin_dir}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${_io_lib_dir}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${_io_lib_dir}")

if(CMAKE_CONFIGURATION_TYPES)
    foreach(config ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${config}" config_upper)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${config_upper} "${_io_bin_dir}/${config}")
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${config_upper} "${_io_lib_dir}/${config}")
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${config_upper} "${_io_lib_dir}/${config}")
    endforeach()
endif()

# Common feature toggles.  BUILD_SHARED_LIBS remains the canonical CMake switch.
option(ENABLE_PCH "Enable target-specific precompiled headers" ON)
option(BUILD_SHARED_LIBS "Build libraries as shared objects" OFF)

if(MSVC)
    # Ensure the resource compiler also respects the UNICODE settings applied per-target.
    string(APPEND CMAKE_RC_FLAGS " /DUNICODE /D_UNICODE")
endif()

# Helper to stamp our standard compile features/definitions onto a target.
function(ApplyTargetDefaults target)
    if(NOT TARGET "${target}")
        message(FATAL_ERROR "ApplyTargetDefaults: target '${target}' does not exist")
    endif()

    target_compile_features(${target} PUBLIC cxx_std_23)
    target_compile_definitions(${target} PRIVATE ${COMPILER_DEFINES})
    target_compile_options(${target} PRIVATE ${COMPILER_SWITCHES})
    if(LINKER_SWITCHES)
        target_link_options(${target} PRIVATE ${LINKER_SWITCHES})
    endif()
endfunction()

# Helper to enable a precompiled header when requested and available.
function(EnablePch target header)
    if(NOT ENABLE_PCH)
        return()
    endif()

    if(NOT TARGET "${target}")
        message(FATAL_ERROR "EnablePch: target '${target}' does not exist")
    endif()

    if(EXISTS "${header}")
        target_precompile_headers(${target} PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:${header}>")
    endif()
endfunction()
