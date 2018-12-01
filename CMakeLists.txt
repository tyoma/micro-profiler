cmake_minimum_required(VERSION 2.8)

project(micro-profiler)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include_directories(.)
include_directories(./libraries/strmd)
include_directories(./libraries/utee)

add_definitions(-DMP_MT_GENERIC)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -std=c++11 -frtti")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wno-deprecated-declarations -Wno-unknown-pragmas")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING)
endif()

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
endif()

add_subdirectory(collector/src)
add_subdirectory(collector/tests)
add_subdirectory(common/src)
add_subdirectory(common/tests)
add_subdirectory(libraries/utee/src)
add_subdirectory(mt/src)
add_subdirectory(test-helpers)
add_subdirectory(patcher/src.x86)
add_subdirectory(patcher/tests)
