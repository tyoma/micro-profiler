cmake_minimum_required(VERSION 2.8)

set(COMMON_SOURCES
	constants.cpp
	formatting.cpp
	memory.cpp
	string.cpp
)

if (WIN32)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		memory_win32.cpp
		time_win32.cpp
	)	
elseif (UNIX)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		memory_unix.cpp
		time_unix.cpp
	)		
endif()

add_library(common STATIC ${COMMON_SOURCES})
