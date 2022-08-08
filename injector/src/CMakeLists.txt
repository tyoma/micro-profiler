cmake_minimum_required(VERSION 2.8)

add_definitions(-DMP_EXPORT_PREFIX=__declspec\(dllexport\))

set(INJECTOR_LIB_SOURCES
	process_win32.cpp
)
set(INJECTOR_SOURCES
	injector.cpp
)

add_library(injector.lib STATIC ${INJECTOR_LIB_SOURCES})
add_library(${injector} SHARED ${INJECTOR_SOURCES})

target_link_libraries(injector.lib psapi.lib)
target_link_libraries(${injector} injector.lib ipc common logger scheduler)
