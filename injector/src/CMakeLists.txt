cmake_minimum_required(VERSION 3.13)

set(INJECTOR_LIB_SOURCES
	process_win32.cpp
)

add_library(injector STATIC ${INJECTOR_LIB_SOURCES})
target_link_libraries(injector mt)
