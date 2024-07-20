# Cmake script for project AddIn
# autor: 1C
cmake_minimum_required (VERSION 3.6)

if(NOT UNIX)
    # Placing automatically generated targets (ALL_BUILD, ZERO_CHECK, etc) into separate folder
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if (WIN32)
    if ("${CMAKE_GENERATOR}" MATCHES "Visual Studio ([0-9]+)")
        if (NOT ${CMAKE_MATCH_1} GREATER_EQUAL 15)
            message(FATAL_ERROR "Supported Visual Studio: 2017 or above with v141_xp toolset")
        endif()
    endif()
    IF(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.15.0)
        cmake_policy(SET CMP0091 NEW)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"  CACHE STRING "Runtime lib" FORCE)
    endif()
endif(WIN32)
SET(CMAKE_CONFIGURATION_TYPES "Release;Debug" CACHE STRING "Build configurations" FORCE)
if (NOT CMAKE_BUILD_TYPE)
    SET(CMAKE_BUILD_TYPE Release CACHE STRING "Build configurations" FORCE)
endif()

project (AddIn)
list(APPEND CMAKE_VS_GLOBALS "XPDeprecationWarning=false")
set(CMAKE_INCLUDE_CURRENT_DIR ON)
include(${CMAKE_SOURCE_DIR}/settings.cmake)
set(VS_TOOLSET_FOR_BROWSERS_REQUIRED "v141_xp")
#-----------------------------------------------------------------------------
# Minimum compiler version check: Microsoft C/C++ >= 18.0 (aka VS 2013 aka VS 12.0)
if (MSVC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 19.0)
  message(FATAL_ERROR "Only Microsoft Visual Studio 2017 or later is currently supported.")
endif ()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    IF(${CMAKE_VERSION} VERSION_LESS 3.15.0)
        foreach(flag
           CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE
           CMAKE_C_FLAGS_MINSIZEREL CMAKE_C_FLAGS_RELWITHDEBINFO
            CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
            CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
            if(${flag} MATCHES "/MD")
                string(REGEX REPLACE "/MD" "/MT" ${flag} "${${flag}}")
                set(${flag} ${${flag}} CACHE "STRING" "Compilation flags" FORCE)
            endif()
        endforeach()
    endif()
endif()

set(_target OFF)
if (WIN32)
    if (${CMAKE_SIZEOF_VOID_P} EQUAL 8 OR "${CMAKE_CXX_COMPILER_ARCHITECTURE_ID}" STREQUAL "x64")
        set(_target OFF)
    else()
        set(_target ON)
    endif()
else()
    set(_target OFF)
endif()

OPTION(TARGET_PLATFORM_32 "Build target for i386" ${_target})

MARK_AS_ADVANCED(TARGET_PLATFORM_32)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#-----------------------------------------------------------------------------
# Minimum compiler version check: GCC >= 4.8
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.8)
  message(FATAL_ERROR "GCC 4.8 or later is required.")
endif ()

if (NOT UNIX)
    IF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
       SET(CMAKE_BUILD_TYPE "Release" CACHE STRING "Compilator type" FORCE)
    ENDIF(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
endif(NOT UNIX)

set(ARCH "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "SYSTEM PROCESSOR")
message("Processor: ${ARCH}")

if (UNIX)
    set(MySuffix1 "Lin" CACHE STRING "OS")
    if (APPLE)
        set(MySuffix1 "Mac" CACHE STRING "OS" FORCE)
    endif(APPLE)
else(UNIX)
    set(MySuffix1 "Win" CACHE STRING "OS")
endif(UNIX)

set(CMAKE_SKIP_RPATH ON CACHE BOOL "")

if(UNIX AND NOT APPLE)
    set(CMAKE_LINK_WHAT_YOU_USE ON)
endif()

set(MySuffix2Append "" CACHE STRING "") # common

if (ARCH STREQUAL aarch64)
    set(MySuffix2 "ARM64" CACHE STRING "Arch")
    SET(ARCH_FLAG "")
elseif (ARCH STREQUAL e2k)
    set(MySuffix2 "E2K" CACHE STRING "Arch") # short suffix
    set(MySuffix2Append "-8C"  CACHE STRING "Arch suffix" FORCE) # full suffix
    SET(ARCH_FLAG "")
elseif (TARGET_PLATFORM_32)
    set(MySuffix2 "32" CACHE STRING "Arch")
    if (UNIX)
        SET(ARCH_FLAG "-m32")
    endif()
else()
    set(MySuffix2 "64" CACHE STRING "Arch")
    if (UNIX)
        SET(ARCH_FLAG "-m64")
    endif()
endif()

if (UNIX  AND NOT APPLE AND NOT ARCH STREQUAL aarch64)
    foreach(flag
        CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_C_FLAGS CMAKE_CXX_FLAGS )
        if(NOT "${flag}" MATCHES "${ARCH_FLAG}")
            set(${flag} ${${flag}} ${ARCH_FLAG} CACHE "STRING" "Linker flags" FORCE)
        endif()
    endforeach()
endif()

set(LIBSUFFIX ${CMAKE_SHARED_LIBRARY_SUFFIX})
set(LIBPREFIX ${CMAKE_SHARED_LIBRARY_PREFIX})
set(EXESUFFIX ${CMAKE_EXECUTABLE_SUFFIX})

set(OS ${MySuffix1})
set(ARCH ${MySuffix2})
set(ARCHSUFFIX ${MySuffix2Append})
if (NOT EXESUFFIX)
    set(EXESUFFIX "")
endif()

set(LIB_DIR ${CMAKE_SOURCE_DIR}/bin)
set(EXE_DIR ${CMAKE_SOURCE_DIR}/bin)
set(ARCH_DIR ${CMAKE_SOURCE_DIR}/bin)
set(PDB_DIR ${CMAKE_SOURCE_DIR}/debug_info)
set(PKG_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/pkg)

file(MAKE_DIRECTORY ${LIB_DIR})
file(MAKE_DIRECTORY ${EXE_DIR})
file(MAKE_DIRECTORY ${ARCH_DIR})
file(MAKE_DIRECTORY ${PDB_DIR})
file(MAKE_DIRECTORY ${PKG_OUTPUT_DIR})

set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${LIB_DIR})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${EXE_DIR})
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${ARCH_DIR})
set (CMAKE_PDB_OUTPUT_DIRECTORY ${PDB_DIR})

IF(WIN32 AND MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /Zi /FS")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} /DEBUG /OPT:REF /OPT:ICF")
ENDIF()
#==============================================================================#
# identification you AddIn
SET(MYCOMPANY "Softonit LLC")
SET(ADN_NAME "AddInClip1C")
string(TIMESTAMP YEAR "%Y")
#==============================================================================#

option(SIGN_SIGNING "Signing binaries" ON)
if (SIGN_SIGNING)
    if(APPLE)
        SET(SIGN_CERT "1C-SOFT, OOO") #certificate name
        find_program(SIGN_BAT NAMES sign.sh HINTS ENV PATH)
        find_program(NOTARY_BAT NAMES notary.sh HINTS ENV PATH)
        find_program(PKG_SIGN NAMES pkgsign.sh HINTS ENV PATH)
    elseif(WIN32)
        find_program(SIGN_BAT NAMES sign.bat HINTS ENV PATH)
        #example sign.bat
        #signtool sign /f certificate.pfx /p password /t http://timestamp.verisign.com/scripts/timstamp.dll /v %1
        #signtool sign /fd SHA256 /f certificate.pfx /p password /tr http://sha256timestamp.ws.symantec.com/sha256/timestamp /td SHA256 /as /v %1
        if (NOT SIGN_BAT)
            find_program(SIGN_TOOL NAMES signtool.exe HINTS ENV PATH)

            if (SIGN_TOOL)
                SET(SIGN_CERT "my.pfx" CACHE FILEPATH "Certificate path" FORCE) #certificate path
                SET(SIGN_CERT_PWD "password" CACHE STRING "Password for certificate" FORCE)
                SET(SIGN_URL_SHA1 "http://timestamp.verisign.com/scripts/timstamp.dll" CACHE STRING "SHA1 timestamp url" FORCE)
                SET(SIGN_URL_SHA256 "http://sha256timestamp.ws.symantec.com/sha256/timestamp" CACHE STRING "SHA256 timestamp url" FORCE)

                SET(SIGN_SHA1 "${SIGN_TOOL}" sign /f "${SIGN_CERT}" /p "${SIGN_CERT_PWD}" /t ${SIGN_URL_SHA1} /v) 
                SET(SIGN_SHA256 "${SIGN_TOOL}" sign  sign /fd SHA256 /f "${SIGN_CERT}" /p "${SIGN_CERT_PWD}" /tr ${SIGN_URL_SHA256} /td SHA256 /as /v)
                if (NOT EXISTS ${SIGN_CERT})
                    message(WARNING "Certificate file not found. Disabled.")
                    set(SIGN_SIGNING OFF)
                endif()
            else()
                message(WARNING "Signing tools not found. Disabled.")
                set(SIGN_SIGNING OFF)
            endif()
        endif()
    endif(APPLE)
endif()
#==============================================================================#
#seting you version file
IF(EXISTS "${CMAKE_SOURCE_DIR}/AddInVersion.txt")
    FILE(STRINGS "${CMAKE_SOURCE_DIR}/AddInVersion.txt" APP_VERSION LIMIT_COUNT 1)
ELSE()
    SET(APP_VERSION 01)
ENDIF()
SET(Addn_VERSION ${APP_VERSION} CACHE STRING "Version of external component" )
SET(ADDNVER "${Addn_VERSION}")
set(Addn_VERSION_FULL
    "${Addn_VERSION_MAJOR}.${Addn_VERSION_MINOR}.${Addn_VERSION_BUILD}.${Addn_VERSION}" CACHE 
    STRING "Full version of external component")
STRING(REPLACE "." "," Addn_VERSION_FULL_C ${Addn_VERSION_FULL})
STRING(REPLACE "." "_" Addn_VERSION_FULL_U ${Addn_VERSION_FULL})
#==============================================================================#
string(TOLOWER ${MYCOMPANY} prefix_lower)
SET(PKG_ID "com.${prefix_lower}.pkg.${Addn_ChromeSetup}-${Addn_VERSION_FULL}")
#==============================================================================#
#seting projects names
IF (NOT Addn_Native)
    SET(Addn_Native "AddInNative" CACHE STRING "Clip 1C" )
ENDIF()
IF (NOT Addn_Mainfest)
    SET(Addn_Mainfest "ru.softonit.addin.clip1c" CACHE STRING "Name of manifest for Chrome and Firefox" )
ENDIF()
IF (NOT Addn_MimeType)
    SET(Addn_MimeType "application/component-example-1" CACHE STRING "MIMEType for NPAPI project" )
ENDIF()
#==============================================================================#
SET(ADDNNATIVE ${Addn_Native})

include(CMakePrintHelpers)
cmake_print_variables(MySuffix2Append MySuffix2)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf_data_in/AddInVersion.h.in ${CMAKE_BINARY_DIR}/AddInVersion.h @ONLY)
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/conf_data_in/MANIFEST.XML.in ${PKG_OUTPUT_DIR}/MANIFEST.XML @ONLY)

UNSET(MIMITYPE)
UNSET(ADDNCHROME)
UNSET(ADDNNATIVE)
UNSET(ADDNIE)
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR})

if(UNIX AND NOT APPLE)
    SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
    find_program(STRIP NAME eu-strip HINTS ENV PATH
        DOC "eu-strip executable" )
    if (NOT STRIP)
        message(WARNING "eu-strip not found!")
    endif()
    
    SET(TMP_DIR ${CMAKE_BINARY_DIR}/${MySuffix2})
elseif(APPLE)
    SET(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
    find_program(STRIP NAME dsymutil HINTS ENV PATH
        DOC "dsymutil executable" )
    if (NOT STRIP)
        message(WARNING "dsymutil not found!")
    else()
        find_program(STRIP2 NAME strip HINTS ENV PATH
        DOC "strip executable" )
    endif()
endif()
SET(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads REQUIRED)

add_subdirectory (NativeAPI)
