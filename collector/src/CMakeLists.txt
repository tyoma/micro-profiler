cmake_minimum_required(VERSION 2.8)

set(COLLECTOR_LIB_SOURCES
	analyzer.cpp
	calibration.cpp
	calls_collector.cpp
	calls_collector_thread.cpp 
	frontend_controller.cpp
	image_patch.cpp
	statistics_bridge.cpp
)

set(COLLECTOR_LIB_SOURCES ${COLLECTOR_LIB_SOURCES}
	empty_function.intel.c
)

add_library(collector.lib STATIC ${COLLECTOR_LIB_SOURCES})
