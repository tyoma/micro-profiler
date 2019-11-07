cmake_minimum_required(VERSION 2.8)

set(INJECTOR_SOURCES
	process_win32.cpp
)

add_library(injector STATIC ${INJECTOR_SOURCES})
