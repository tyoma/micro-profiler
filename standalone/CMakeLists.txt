cmake_minimum_required(VERSION 2.8)

add_definitions(-D_ATL_SINGLE_THREADED)

set(MICROPROFILER_SOURCES
	main.cpp
	ProfilerMainDialog.cpp
	resources/micro-profiler_standalone.rc

	../frontend/src/untested/resources/frontend.rc
)

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS /MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

add_executable(micro-profiler_standalone ${MICROPROFILER_SOURCES})

target_link_libraries(micro-profiler_standalone frontend.lib ipc logger common wpl.ui)
