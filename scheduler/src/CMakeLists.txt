cmake_minimum_required(VERSION 2.8)

set(SCHEDULER_SOURCES
	private_queue.cpp
	task_queue.cpp
	thread_queue.cpp
)

if (WIN32)
	set(SCHEDULER_SOURCES ${SCHEDULER_SOURCES}
		ui_queue_win32.cpp
	)
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(SCHEDULER_SOURCES ${SCHEDULER_SOURCES}
		ui_queue_macos.mm
	)
elseif (ANDROID)
	set(SCHEDULER_SOURCES ${SCHEDULER_SOURCES}
		ui_queue_android.cpp
	)
endif()

add_library(scheduler STATIC ${SCHEDULER_SOURCES})

if (APPLE)
	target_link_libraries(scheduler "-framework Foundation -framework Cocoa")
elseif (ANDROID) 
	target_link_libraries(scheduler log android)
endif()
