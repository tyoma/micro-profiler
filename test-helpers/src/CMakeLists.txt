cmake_minimum_required(VERSION 2.8)

include(test)

set(TEST_HELPERS_SOURCES
	helpers.cpp
	mock_frontend.cpp
	test_constants.cpp
	thread.cpp
)

if (WIN32)
	set(TEST_HELPERS_SOURCES ${TEST_HELPERS_SOURCES}
		com.cpp
		helpers_win32.cpp
		process_win32.cpp
		thread_win32.cpp
	)
elseif(UNIX)
	set(TEST_HELPERS_SOURCES ${TEST_HELPERS_SOURCES}
		helpers_unix.cpp
		thread_unix.cpp
	)
endif()

add_library(test-helpers STATIC ${TEST_HELPERS_SOURCES})

target_link_libraries(test-helpers common)
