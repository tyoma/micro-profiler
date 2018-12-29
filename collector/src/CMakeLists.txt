cmake_minimum_required(VERSION 2.8)

if(MSVC)
	project(micro-profiler C CXX ASM_MASM)
	enable_language(ASM_MASM)

	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_definitions(-D_M_IX86=1)
	else()
		add_definitions(-D_M_X64=1)
	endif()
endif()

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

add_library(calibration STATIC ${CALIBRATION_SOURCES})
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:GNU>:-finstrument-functions>)
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:Clang>:-finstrument-functions>)
target_compile_options(calibration PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/GH /Gh>)

add_library(micro-profiler SHARED ${COLLECTOR_SOURCES})
target_link_libraries(micro-profiler calibration collector.lib patcher ipc common)

if(UNIX)
	target_compile_options(micro-profiler PRIVATE -fvisibility=default)
	set_target_properties(micro-profiler PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/collector.expmap") 
endif()
