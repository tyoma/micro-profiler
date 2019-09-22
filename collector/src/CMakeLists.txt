cmake_minimum_required(VERSION 2.8)

if(MSVC)
	project(micro-profiler C CXX ASM_MASM)
	enable_language(ASM_MASM)

	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_definitions(-D_M_IX86)
	else()
		add_definitions(-D_M_X64)
	endif()
endif()

set(COLLECTOR_LIB_SOURCES
	analyzer.cpp
	calls_collector.cpp
	calls_collector_thread.cpp
	collector_app.cpp
	image_patch.cpp
	module_tracker.cpp
	statistics_bridge.cpp
)

set(CALIBRATION_SOURCES
	calibration_functions_inner_instrumented.cpp
	calibration_functions_outer_instrumented.cpp
)

set(COLLECTOR_SOURCES ${CALIBRATION_SOURCES}
	calibration.cpp
	main.cpp
)

if(UNIX)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		main_unix.cpp
	)
elseif(WIN32)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		collector.def
		hooks.asm
		main_win32.cpp
	)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
endif()

add_library(collector.lib STATIC ${COLLECTOR_LIB_SOURCES})

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set_property(SOURCE ${CALIBRATION_SOURCES} PREPEND_STRING PROPERTY COMPILE_FLAGS " -finstrument-functions ")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
	set_property(SOURCE ${CALIBRATION_SOURCES} PREPEND_STRING PROPERTY COMPILE_FLAGS " /GH /Gh ")
endif()

add_library(${micro-profiler} SHARED ${COLLECTOR_SOURCES})
target_link_libraries(${micro-profiler} collector.lib patcher ipc common)

if(UNIX)
	target_compile_options(${micro-profiler} PRIVATE -fvisibility=default)
	set_target_properties(${micro-profiler} PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/collector.expmap ") 
endif()
