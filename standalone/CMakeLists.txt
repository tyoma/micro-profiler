cmake_minimum_required(VERSION 3.4)

add_definitions(-D_ATL_SINGLE_THREADED)

set(MICROPROFILER_SOURCES
	main.cpp
	ProfilerMainDialog.cpp
	resources/micro-profiler_standalone.rc
)

if (WIN32)
	set(MICROPROFILER_SOURCES ${MICROPROFILER_SOURCES}
		standalone.manifest
	)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
endif()

add_executable(micro-profiler_standalone ${MICROPROFILER_SOURCES})

target_link_libraries(micro-profiler_standalone frontend.lib ipc logger common wpl)
