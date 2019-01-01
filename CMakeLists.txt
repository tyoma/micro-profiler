cmake_minimum_required(VERSION 2.8)

project(micro-profiler)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


# Setup include directories
include_directories(.)
include_directories(./libraries/agge)
include_directories(./libraries/strmd)
include_directories(./libraries/utee)
include_directories(./libraries/wpl)
if (WIN32)
	include_directories(./compat)
endif()


# Add definitions: global, platform-based, compiler-based
if(UNIX)
	set(MP_MT_GENERIC 1)
	add_definitions(-DMP_MT_GENERIC)
	add_definitions(-DMP_NO_EXCEPTIONS) # Before DWARF exception model is supported...
endif()
if (MSVC)
	add_definitions(-Dsnprintf=_snprintf)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING -D_WINSOCK_DEPRECATED_NO_WARNINGS)
endif()


# Setup global compiler/linker settings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -frtti -fvisibility=hidden -Wl,--no-undefined")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations -Wno-unknown-pragmas")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set(configurations
		CMAKE_C_FLAGS_DEBUG
		CMAKE_C_FLAGS_MINSIZEREL
		CMAKE_C_FLAGS_RELEASE
		CMAKE_C_FLAGS_RELWITHDEBINFO
		CMAKE_CXX_FLAGS_DEBUG
		CMAKE_CXX_FLAGS_MINSIZEREL
		CMAKE_CXX_FLAGS_RELEASE
		CMAKE_CXX_FLAGS_RELWITHDEBINFO
	)
	foreach (configuration ${configurations})
		string(REGEX REPLACE "/MDd" "/MTd" ${configuration} "${${configuration}}")
		string(REGEX REPLACE "/MD" "/MT" ${configuration} "${${configuration}}")
	endforeach()
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /OPT:REF")
endif()
if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()


# Add subdirectories
add_subdirectory(collector/src)
add_subdirectory(collector/tests)
add_subdirectory(common/src)
add_subdirectory(common/tests)
add_subdirectory(frontend/src)
add_subdirectory(frontend/tests)
add_subdirectory(ipc/src)
add_subdirectory(ipc/tests)
add_subdirectory(libraries/utee/src)
add_subdirectory(micro-profiler.tests/guineapigs)
add_subdirectory(micro-profiler.tests)
add_subdirectory(mt/src)
add_subdirectory(test-helpers)
add_subdirectory(patcher/src.x86)
add_subdirectory(patcher/tests)

add_subdirectory(playground)

if (WIN32)
	add_subdirectory(mt/tests)
endif()
