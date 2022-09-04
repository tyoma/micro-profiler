cmake_minimum_required(VERSION 3.13)

set(FRONTEND_LIB_SOURCES
	columns_layout.cpp
	derived_statistics.cpp
	frontend.cpp
	frontend_manager.cpp
	frontend_metadata.cpp
	frontend_patcher.cpp
	headers_model.cpp
	image_patch_model.cpp
	profiling_preferences.cpp
	representation.cpp
	symbol_resolver.cpp
	threads_model.cpp

	untested/about_ui.cpp
	untested/factory.cpp
	untested/image_patch_ui.cpp
	untested/ipc_manager.cpp
	untested/statistics_poll.cpp
	untested/tables_ui.cpp
)

if(NOT ANDROID_ABI OR ANDROID_ABI MATCHES "x86")
	set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
		untested/function_hint.cpp
		untested/piechart.cpp
	)
endif()

if(WIN32)
	set(FRONTEND_LIB_SOURCES ${FRONTEND_LIB_SOURCES}
		untested/file.cpp
		untested/system_stylesheet.cpp
	)
endif()

add_library(frontend STATIC ${FRONTEND_LIB_SOURCES})
target_link_libraries(frontend wpl agge.text strmd)
