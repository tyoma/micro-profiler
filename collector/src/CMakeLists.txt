cmake_minimum_required(VERSION 2.8)

set(COLLECTOR_LIB_SOURCES
	analyzer.cpp
#	binary_image.cpp
	calls_collector.cpp
	calls_collector_thread.cpp 
#	channel_client.cpp
#	frontend_controller.cpp
#	image_patch.cpp
#	main.cpp
#	patched_image.cpp
	statistics_bridge.cpp
)

add_library(collector.lib STATIC ${COLLECTOR_LIB_SOURCES})
target_link_libraries(collector.lib mt)
