cmake_minimum_required(VERSION 2.8)

set(COLLECTOR_LIB_SOURCES
	analyzer.cpp
	calls_collector.cpp
	calls_collector_thread.cpp 
	frontend_controller.cpp
	image_patch.cpp
	statistics_bridge.cpp
)

set(CALIBRATION_SOURCES
	calibration_functions_inner_instrumented.cpp
	calibration_functions_outer_instrumented.cpp
)

set(COLLECTOR_SOURCES
	calibration.cpp
)

if(UNIX)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		main_unix.cpp
	)
elseif(WIN32)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		collector.def
		hooks.asm
		main.cpp
	)
endif()

add_library(collector.lib STATIC ${COLLECTOR_LIB_SOURCES})

add_library(calibration STATIC ${CALIBRATION_SOURCES})
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:GNU>:-finstrument-functions>)
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:Clang>:-finstrument-functions>)
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/GH /Gh>)

add_library(collector SHARED ${COLLECTOR_SOURCES})
target_compile_options(collector PRIVATE $<$<CXX_COMPILER_ID:GNU>:-fvisibility=default>)
target_link_libraries(collector calibration collector.lib patcher ipc common)

if(UNIX)
	set_target_properties(collector PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/collector.expmap") 
endif()
