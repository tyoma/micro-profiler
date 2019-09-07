cmake_minimum_required(VERSION 2.8)

include(test)

set(MT_TEST_SOURCES
	ThreadTests.cpp
)

add_library(mt.tests SHARED ${MT_TEST_SOURCES})

target_link_libraries(mt.tests mt)
