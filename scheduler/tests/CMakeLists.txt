cmake_minimum_required(VERSION 2.8)

include(test)

set(SCHEDULER_TEST_SOURCES
	TaskQueueTests.cpp
)

if (WIN32)
	set(SCHEDULER_TEST_SOURCES ${SCHEDULER_TEST_SOURCES}
		helpers_win32.cpp
		UIQueueTests.cpp
	)
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(SCHEDULER_TEST_SOURCES ${SCHEDULER_TEST_SOURCES}
		helpers_macos.mm
		UIQueueTests.cpp
	)
endif()

add_library(scheduler.tests SHARED ${SCHEDULER_TEST_SOURCES})

target_link_libraries(scheduler.tests scheduler common logger mt)
