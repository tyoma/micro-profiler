cmake_minimum_required(VERSION 2.8)

set(COMMON_SOURCES
	allocator.cpp
	constants.cpp
	formatting.cpp
	string.cpp
#	stopwatch.cpp
)

if (WIN32)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		module.cpp
	)	
elseif (UNIX)
	
endif()

add_library(common STATIC ${COMMON_SOURCES})
