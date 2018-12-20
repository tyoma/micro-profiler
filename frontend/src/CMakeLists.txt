cmake_minimum_required(VERSION 2.8)

set(FRONTEND_LIB_SOURCES
	columns_model.cpp
	frontend.cpp
	frontend_manager.cpp
	function_list.cpp
)

add_library(frontend.lib STATIC ${FRONTEND_LIB_SOURCES})
