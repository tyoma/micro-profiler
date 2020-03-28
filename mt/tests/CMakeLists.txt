cmake_minimum_required(VERSION 2.8)

include(test)

set(MT_TEST_SOURCES
	ThreadCallbacksTests.cpp
)

if(WIN32)
	set(MT_TEST_SOURCES ${MT_TEST_SOURCES}
		ThreadTests.cpp
	)
endif()

add_library(mt.tests SHARED ${MT_TEST_SOURCES})

target_link_libraries(mt.tests mt $<TARGET_OBJECTS:mt.thread_callbacks> test-helpers)
