cmake_minimum_required(VERSION 2.8)

include(test)

set(SCHEDULER_TEST_SOURCES
	TaskQueueTests.cpp
)

add_library(scheduler.tests SHARED ${SCHEDULER_TEST_SOURCES})

target_link_libraries(scheduler.tests scheduler common mt)
