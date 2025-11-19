# This modules provides variables with recommended Compiler and Linker switches
# for building InterstellarOutpost with MSVC toolchain.

set(COMPILER_DEFINES "")
set(COMPILER_SWITCHES "")
set(LINKER_SWITCHES "")

if(NOT MSVC)
  message(FATAL_ERROR "InterstellarOutpost requires the MSVC toolchain.")
endif()

#--- Determines target architecture if not explicitly set
if(DEFINED VCPKG_TARGET_ARCHITECTURE)
    set(APP_ARCH ${VCPKG_TARGET_ARCHITECTURE})
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Ww][Ii][Nn]32$")
    set(APP_ARCH x86)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Xx]64$")
    set(APP_ARCH x64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]$")
    set(APP_ARCH arm)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64$")
    set(APP_ARCH arm64)
elseif(CMAKE_GENERATOR_PLATFORM MATCHES "^[Aa][Rr][Mm]64EC$")
    set(APP_ARCH arm64ec)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Ww][Ii][Nn]32$")
    set(APP_ARCH x86)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Xx]64$")
    set(APP_ARCH x64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]$")
    set(APP_ARCH arm)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64$")
    set(APP_ARCH arm64)
elseif(CMAKE_VS_PLATFORM_NAME_DEFAULT MATCHES "^[Aa][Rr][Mm]64EC$")
    set(APP_ARCH arm64ec)
elseif(NOT (DEFINED APP_ARCH))
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64|aarch64|arm64")
        set(APP_ARCH arm64)
    else()
        set(APP_ARCH x64)
    endif()
endif()

#--- Determines host architecture
if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "[Aa][Rr][Mm]64|aarch64|arm64")
    set(DIRECTX_HOST_ARCH arm64)
else()
    set(DIRECTX_HOST_ARCH x64)
endif()

#--- Check APP_ARCH value
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    if(APP_ARCH MATCHES "x86|^arm$")
      message(FATAL_ERROR "64-bit toolset mismatch detected for APP_ARCH setting")
    endif()
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    if(APP_ARCH MATCHES "x64|arm64")
      message(FATAL_ERROR "32-bit toolset mismatch detected for APP_ARCH setting")
    endif()
endif()

#--- Build with Unicode Win32 APIs per "UTF-8 Everywhere"
if(WIN32)
  list(APPEND COMPILER_DEFINES _UNICODE UNICODE)
endif()

#--- General MSVC SDL options
list(APPEND COMPILER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:cf>")
list(APPEND LINKER_SWITCHES /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO)

if(WINDOWS_STORE)
  list(APPEND COMPILER_SWITCHES /bigobj)
  list(APPEND LINKER_SWITCHES /APPCONTAINER /MANIFEST:NO)
endif()

if((${APP_ARCH} STREQUAL "x86")
   OR ((CMAKE_SIZEOF_VOID_P EQUAL 4) AND (NOT (${APP_ARCH} MATCHES "^arm"))))
  list(APPEND LINKER_SWITCHES /SAFESEH)
endif()

if((MSVC_VERSION GREATER_EQUAL 1928)
   AND (CMAKE_SIZEOF_VOID_P EQUAL 8)
   AND (NOT (TARGET OpenEXR::OpenEXR)))
      list(APPEND COMPILER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
      list(APPEND LINKER_SWITCHES "$<$<NOT:$<CONFIG:DEBUG>>:/guard:ehcont>")
endif()

#--- Target architecture switches
if(XBOX_CONSOLE_TARGET STREQUAL "scarlett")
    list(APPEND COMPILER_SWITCHES $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX2,-march=znver2>)
elseif(XBOX_CONSOLE_TARGET MATCHES "xboxone|durango")
    list(APPEND COMPILER_SWITCHES  $<IF:$<CXX_COMPILER_ID:MSVC>,/favor:AMD64 /arch:AVX,-march=btver2>)
elseif(NOT (${APP_ARCH} MATCHES "^arm"))
    if((${APP_ARCH} STREQUAL "x86") OR (CMAKE_SIZEOF_VOID_P EQUAL 4))
        list(APPEND COMPILER_SWITCHES /arch:SSE2)
    endif()
endif()

#--- Compiler-specific switches for MSVC
list(APPEND COMPILER_SWITCHES /sdl /Zc:inline /fp:fast)

if(WINDOWS_STORE)
  list(APPEND COMPILER_SWITCHES /await)
endif()

if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
  message(STATUS "Building using Whole Program Optimization")
  list(APPEND COMPILER_SWITCHES $<$<NOT:$<CONFIG:Debug>>:/Gy /Gw>)
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.10)
  list(APPEND COMPILER_SWITCHES /permissive-)
endif()

if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.11)
   AND (OpenMP_CXX_FOUND
       OR (XBOX_CONSOLE_TARGET STREQUAL "durango")))
  # OpenMP in MSVC is not compatible with /permissive- unless you disable two-phase lookup
  list(APPEND COMPILER_SWITCHES /Zc:twoPhase-)
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.14)
  list(APPEND COMPILER_SWITCHES /Zc:__cplusplus)
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.15)
  list(APPEND COMPILER_SWITCHES /JMC-)
endif()

if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.20)
   AND (XBOX_CONSOLE_TARGET STREQUAL "durango"))
    list(APPEND COMPILER_SWITCHES /d2FH4-)
    if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
      list(APPEND LINKER_SWITCHES -d2:-FH4-)
    endif()
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.24)
  list(APPEND COMPILER_SWITCHES /ZH:SHA_256)
endif()

if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.26)
   AND (NOT (XBOX_CONSOLE_TARGET STREQUAL "durango")))
    list(APPEND COMPILER_SWITCHES /Zc:preprocessor /wd5104 /wd5105)
endif()

if((CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.27) AND (NOT (${APP_ARCH} MATCHES "^arm")))
  list(APPEND LINKER_SWITCHES /CETCOMPAT)
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.28)
  list(APPEND COMPILER_SWITCHES /Zc:lambda)
endif()

if(WINDOWS_STORE AND (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.32))
  list(APPEND COMPILER_SWITCHES /wd5246)
endif()

if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.35)
  if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    list(APPEND COMPILER_SWITCHES $<$<NOT:$<CONFIG:Debug>>:/Zc:checkGwOdr>)
  endif()

  if(NOT (DEFINED XBOX_CONSOLE_TARGET))
    list(APPEND COMPILER_SWITCHES $<$<VERSION_GREATER_EQUAL:${CMAKE_VS_WINDOWS_TARGET_PLATFORM_VERSION},10.0.22000>:/Zc:templateScope>)
  endif()
endif()

#--- Windows API Family
include(CheckIncludeFileCXX)

if(WINDOWS_STORE)
    list(APPEND COMPILER_DEFINES WINAPI_FAMILY=WINAPI_FAMILY_APP)
endif()

#--- Address Sanitizer switches
set(ASAN_SWITCHES /fsanitize=address /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
set(ASAN_LIBS sancov.lib)

#--- Code coverage
if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 19.33)
    set(COV_LINKER_SWITCHES /PROFILE)
endif()
