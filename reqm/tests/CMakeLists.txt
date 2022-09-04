cmake_minimum_required(VERSION 3.13)

set(FRONTEND_TESTS_SOURCES
	MultiplexingRequestTests.cpp
)

add_library(reqm.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(reqm.tests)
