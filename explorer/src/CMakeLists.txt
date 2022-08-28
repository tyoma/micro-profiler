cmake_minimum_required(VERSION 3.13)

set(EXPLORER_SOURCES
	process_win32.cpp
)

add_library(explorer STATIC ${EXPLORER_SOURCES})
