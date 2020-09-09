cmake_minimum_required(VERSION 2.8)

set(FRONTEND_LIB_SOURCES
	columns_model.cpp
	frontend.cpp
	frontend_manager.cpp
	function_list.cpp
	process_list.cpp
	symbol_resolver.cpp
	threads_model.cpp
)

set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
	untested/factory.cpp
	untested/ipc_manager.cpp
	untested/listview.cpp
	untested/piechart.cpp
	untested/tables_ui.cpp
)

if(WIN32)
	set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
		untested/about_ui.cpp
		untested/file.cpp
		untested/marshalling_server.cpp
		untested/stream.cpp
	)
endif()

add_library(frontend.lib STATIC ${FRONTEND_LIB_SOURCES})
