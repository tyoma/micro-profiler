cmake_minimum_required(VERSION 3.13)

set(SLITEPP_TESTS_SOURCES
	DatabaseExpressionTests.cpp
	DatabaseTests.cpp
)

add_library(sqlite++.tests SHARED ${SLITEPP_TESTS_SOURCES})
target_link_libraries(sqlite++.tests sqlite++ test-helpers)
