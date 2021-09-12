cmake_minimum_required(VERSION 2.8)

include(test)

set(MICROPROFILER_TEST_SOURCES
	CollectorIntegrationTests.cpp
	mock_frontend.cpp
)

add_library(micro-profiler.tests SHARED ${MICROPROFILER_TEST_SOURCES})

target_link_libraries(micro-profiler.tests ipc logger common test-helpers)
