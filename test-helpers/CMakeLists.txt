set(TEST_HELPERS_SOURCES
	helpers.cpp
)

if (WIN32)
	set(TEST_HELPERS_SOURCES ${TEST_HELPERS_SOURCES}
		com.cpp
		helpers_win32.cpp
		mocks_com.cpp
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
