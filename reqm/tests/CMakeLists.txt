cmake_minimum_required(VERSION 2.8)

include(test)

set(FRONTEND_TESTS_SOURCES
	MultiplexingRequestTests.cpp
)

add_library(reqm.tests SHARED ${FRONTEND_TESTS_SOURCES})

target_link_libraries(reqm.tests)
