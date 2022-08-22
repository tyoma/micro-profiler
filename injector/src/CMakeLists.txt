cmake_minimum_required(VERSION 2.8)

add_definitions(-DMP_EXPORT_PREFIX=__declspec\(dllexport\))

set(INJECTOR_LIB_SOURCES
	process_win32.cpp
)

add_library(injector STATIC ${INJECTOR_LIB_SOURCES})

target_link_libraries(injector psapi)
