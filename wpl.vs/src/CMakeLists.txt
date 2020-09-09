cmake_minimum_required(VERSION 2.8)

include(vssdk)

add_definitions(-D_ATL_FREE_THREADED)

set(WPL_VS_SOURCES
	async.cpp
	frame.cpp
	pane.cpp
	vspackage.cpp
)

add_library(wpl.vs STATIC ${WPL_VS_SOURCES})

target_link_libraries(wpl.vs common wpl agge agge.text)
