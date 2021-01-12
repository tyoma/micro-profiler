cmake_minimum_required(VERSION 2.8)

set(SCHEDULER_SOURCES
	task_queue.cpp
)

if (WIN32)
	set(SCHEDULER_SOURCES ${SCHEDULER_SOURCES}
		ui_queue_win32.cpp
	)
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(SCHEDULER_SOURCES ${SCHEDULER_SOURCES}
	)
endif()

add_library(scheduler STATIC ${SCHEDULER_SOURCES})
