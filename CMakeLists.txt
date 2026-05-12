cmake_minimum_required(VERSION 3.10)

project(Clip1C LANGUAGES C CXX)
set(TARGET Clip1C)

if(APPLE)
	enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CLIP1C_SOURCES
	version.h
	src/AddInNative.cpp
	src/AddInNative.def
	src/AddInNative.h
	src/AddInNative.rc
	src/BaseHelper.h
	src/ClipboardControl.cpp
	src/ClipboardControl.h
	src/ClipboardManager.cpp
	src/ClipboardManager.h
	src/ImageHelper.cpp
	src/ImageHelper.h
	src/json.hpp
	src/stdafx.h
)

if(APPLE)
	list(APPEND CLIP1C_SOURCES src/ClipboardManager_osx.mm)
endif()

add_library(${TARGET} SHARED ${CLIP1C_SOURCES})

target_compile_definitions(${TARGET} PRIVATE UNICODE _UNICODE)
target_include_directories(${TARGET} PRIVATE include)

# Имя выходного файла: <Name>{Win|Lin|Mac}{32|64}.{dll|so|dylib}
if(WIN32)
	if(NOT MSVC)
		message(FATAL_ERROR "Под Windows проект собирается только MSVC")
	endif()
	set(MySuffixOS "Win")
elseif(APPLE)
	set(MySuffixOS "Mac")
else()
	set(MySuffixOS "Lin")
endif()

if(TARGET_PLATFORM_32)
	set(MySuffixBits "32")
else()
	set(MySuffixBits "64")
endif()

set(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin${MySuffixBits})
set(EXECUTABLE_OUTPUT_PATH ${LIBRARY_OUTPUT_PATH})
set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${LIBRARY_OUTPUT_PATH})

set_target_properties(${TARGET} PROPERTIES
	PREFIX ""
	OUTPUT_NAME "${PROJECT_NAME}${MySuffixOS}${MySuffixBits}"
)

if(WIN32)
	set(CMAKE_SUPPRESS_REGENERATION 1)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
	add_definitions(/MT)
	target_compile_definitions(${TARGET} PRIVATE _WINDOWS
		_SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING)
	target_compile_options(${TARGET} PRIVATE /utf-8)
else()
	# На macOS архитектура задаётся через CMAKE_OSX_ARCHITECTURES, флаг -m32/-m64 не нужен
	if(NOT APPLE)
		if(TARGET_PLATFORM_32)
			set(CMAKE_LIBRARY_PATH "/usr/lib/i386-linux-gnu" CACHE PATH "")
			set(ARCH_FLAG "-m32")
		else()
			set(ARCH_FLAG "-m64")
		endif()
		foreach(flag
			CMAKE_EXE_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS
			CMAKE_C_FLAGS CMAKE_CXX_FLAGS)
			if(NOT ${flag} MATCHES ${ARCH_FLAG})
				set(${flag} "${${flag}} ${ARCH_FLAG}" CACHE STRING "Linker flags" FORCE)
			endif()
		endforeach()
	endif()

	# Сборка библиотеки clip (используется на Linux; на macOS подключим Cocoa напрямую)
	if(NOT APPLE)
		target_include_directories(${TARGET} PRIVATE "${CMAKE_SOURCE_DIR}/3rdparty/clip-1.3")
		add_subdirectory("${CMAKE_SOURCE_DIR}/3rdparty/clip-1.3")
		target_link_libraries(${TARGET} clip)
	endif()

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -s")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -s")
endif()

if(UNIX AND NOT APPLE)
	include(FindX11)
	find_package(X11 REQUIRED)
	include_directories(${X11_INCLUDE_DIR})
	target_link_libraries(${TARGET} ${X11_LIBRARIES})

	set_target_properties(${TARGET} PROPERTIES
		LINK_FLAGS "-Wl,--no-undefined -Xlinker --version-script -Xlinker ${CMAKE_CURRENT_SOURCE_DIR}/src/version.script"
	)

	set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH ON)
	find_program(STRIP NAME eu-strip HINTS ENV PATH DOC "eu-strip executable")
	if(STRIP)
		set(STRIP_CMD ${STRIP} $<TARGET_FILE:${PROJECT_NAME}>
			-f ${LIBRARY_OUTPUT_PATH}/$<TARGET_FILE_NAME:${PROJECT_NAME}>.debug)
		add_custom_command(TARGET ${PROJECT_NAME}
			POST_BUILD COMMAND ${STRIP_CMD}
			COMMENT "Strip ${PROJECT_NAME}"
			MAIN_DEPENDENCY ${PROJECT_NAME}
		)
	else()
		message(WARNING "eu-strip не найден — отладочные символы не будут отделены")
	endif()
endif()

if(APPLE)
	find_library(COCOA_LIBRARY Cocoa REQUIRED)
	find_library(APPKIT_LIBRARY AppKit REQUIRED)
	find_library(FOUNDATION_LIBRARY Foundation REQUIRED)
	target_link_libraries(${TARGET}
		${COCOA_LIBRARY}
		${APPKIT_LIBRARY}
		${FOUNDATION_LIBRARY}
	)
	set_source_files_properties(src/ClipboardManager_osx.mm
		PROPERTIES COMPILE_FLAGS "-fobjc-arc")
	set_target_properties(${TARGET} PROPERTIES
		SUFFIX ".dylib"
	)
endif()
