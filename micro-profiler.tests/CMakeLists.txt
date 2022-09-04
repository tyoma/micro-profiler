cmake_minimum_required(VERSION 3.13)

set(MICROPROFILER_TEST_SOURCES
	CollectorIntegrationTests.cpp
)

add_library(micro-profiler.tests SHARED ${MICROPROFILER_TEST_SOURCES})
target_link_libraries(micro-profiler.tests ipc logger common test-helpers)
