cmake_minimum_required(VERSION 2.8)

set(EXPLORER_SOURCES
	process_win32.cpp
)

add_library(explorer STATIC ${EXPLORER_SOURCES})
