cmake_minimum_required(VERSION 2.8)

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

set(MICROPROFILER_TEST_SOURCES
	CollectorIntegrationTests.cpp
	main.cpp
)

add_library(micro-profiler.tests SHARED ${MICROPROFILER_TEST_SOURCES})

target_link_libraries(micro-profiler.tests ipc common test-helpers utee)
