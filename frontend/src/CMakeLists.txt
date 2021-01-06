cmake_minimum_required(VERSION 2.8)

set(FRONTEND_LIB_SOURCES
	columns_model.cpp
	frontend.cpp
	frontend_manager.cpp
	function_list.cpp
	process_list.cpp
	symbol_resolver.cpp
	threads_model.cpp

	untested/about_ui.cpp
	untested/factory.cpp
	untested/ipc_manager.cpp
	untested/marshalling_server.cpp
	untested/tables_ui.cpp
)

if(NOT ANDROID_ABI OR ANDROID_ABI MATCHES "x86")
	set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
		untested/piechart.cpp
	)
endif()

if(WIN32)
	set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
		untested/file.cpp
		untested/stream.cpp
		untested/system_stylesheet.cpp
		untested/ui_queue_win32.cpp
	)
endif()

add_library(frontend.lib STATIC ${FRONTEND_LIB_SOURCES})
