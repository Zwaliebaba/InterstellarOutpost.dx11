# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.21)

set(APP_CLIENT "IOCClient")
set(APP_SERVER "IOServer")
set(APP_CLIENT_OUTPUT_NAME "InterstellarOutpostClient")
set(APP_SERVER_OUTPUT_NAME "InterstellarOutpostServer")
set(INTERSTELLAROUTPOST_EXPORT_NAME "InterstellarOutpostTargets")

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# Integrate vcpkg toolchain if available
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VCPKG_ROOT})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
  elseif(EXISTS "C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake")
    set(CMAKE_TOOLCHAIN_FILE "C:/Program Files/Microsoft Visual Studio/18/Community/VC/vcpkg/scripts/buildsystems/vcpkg.cmake"
        CACHE STRING "Vcpkg toolchain file")
  endif()
endif()

set(INTERSTELLAROUTPOST_VERSION 1.0.0)

project ("InterstellarOutpost"
  VERSION ${INTERSTELLAROUTPOST_VERSION}
  DESCRIPTION "Interstellar Outpost"
  LANGUAGES CXX)

# Build settings

  # Enable building of unit tests
  option(BUILD_TESTING "Build unit tests" OFF)

# Enable Spectre Mitigations for MSVC compilers
# https://devblogs.microsoft.com/cppblog/spectre-mitigations-in-msvc/
option(ENABLE_SPECTRE_MITIGATION "Build using /Qspectre for MSVC" OFF)

# Disable iterator debugging in Debug configurations with the MSVC CRT
option(DISABLE_MSVC_ITERATOR_DEBUGGING "Disable iterator debugging in Debug configurations with the MSVC CRT" OFF)

# Enable Static Code Analysis
option(ENABLE_CODE_ANALYSIS "Use Static Code Analysis on build" OFF)

# Enable Code Coverage
option(ENABLE_CODE_COVERAGE "Build with code-coverage" OFF)

# Enable Code Profiling
option(ENABLE_CODE_PROFILING "Build for profiling" OFF)

# Enable Fuzz Testing
option(BUILD_FUZZING "Build for fuzz testing" OFF)

# Enable PIX instrumentation support
option(ENABLE_PIX_PROFILING "Link against WinPixEventRuntime for GPU instrumentation" ON)

# Set C++ standard to C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include standard project settings and compiler warnings.
list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CompilerAndLinker)
include(GNUInstallDirs)
include(StandardProjectSettings)

set(INTERSTELLAROUTPOST_INSTALL_BINDIR ${CMAKE_INSTALL_BINDIR})
set(INTERSTELLAROUTPOST_INSTALL_LIBDIR ${CMAKE_INSTALL_LIBDIR})
set(INTERSTELLAROUTPOST_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}")
set(INTERSTELLAROUTPOST_INSTALL_CLIENT_BINDIR "${INTERSTELLAROUTPOST_INSTALL_BINDIR}/Client")
set(INTERSTELLAROUTPOST_INSTALL_SERVER_BINDIR "${INTERSTELLAROUTPOST_INSTALL_BINDIR}/Server")

# Include sub-projects (libraries before executables).
add_subdirectory ("Libs/NeuronCore")
add_subdirectory ("Libs/NeuronClient")
add_subdirectory ("Libs/NeuronServer")
add_subdirectory ("Libs/GameLogic")
add_subdirectory ("IOClient")
add_subdirectory ("IOServer")

if(BUILD_TESTING)
  # Enable usage of CTest for testing
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()

# Installation and packaging configuration (must appear after defining install() rules).
include(InstallConfig)
