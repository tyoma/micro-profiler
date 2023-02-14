cmake_minimum_required(VERSION 3.4)

set(MP_OUTDIR $<TARGET_FILE_DIR:${micro-profiler}>)

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
	active_server_app.cpp
	analyzer.cpp
	calls_collector.cpp
	calls_collector_thread.cpp
	collector_app.cpp
	module_tracker.cpp
	thread_monitor.cpp
)

if (WIN32)
	set(COLLECTOR_LIB_SOURCES ${COLLECTOR_LIB_SOURCES}
		calls_collector_msvc.asm
		process_explorer_win32.cpp
	)
elseif (APPLE)
	set(COLLECTOR_LIB_SOURCES ${COLLECTOR_LIB_SOURCES}
		process_explorer_macos.cpp
	)
elseif (UNIX)	
	set(COLLECTOR_LIB_SOURCES ${COLLECTOR_LIB_SOURCES}
		process_explorer_linux.cpp
	)
endif()

set(CALIBRATION_SOURCES
	calibration_functions_inner_instrumented.cpp
	calibration_functions_outer_instrumented.cpp
)

set(COLLECTOR_SOURCES ${CALIBRATION_SOURCES}
	calibration.cpp
	detour.cpp
	main.cpp
)

if(UNIX)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		main_unix.cpp
	)
elseif(WIN32)
	set(COLLECTOR_SOURCES ${COLLECTOR_SOURCES}
		collector.rc
		hooks.asm
		injector.cpp
		main_win32.cpp
		collector.manifest
	)
endif()

add_custom_target(dbghelp_redist
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/../../redist/dbghelp ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
	COMMENT "Copying dbghelp.dll redistributables..."
)

add_library(collector STATIC ${COLLECTOR_LIB_SOURCES})
target_compile_definitions(collector PUBLIC SDB_NO_SIGNALS)
target_link_libraries(collector polyq strmd)

add_library(${micro-profiler} SHARED ${COLLECTOR_SOURCES} $<TARGET_OBJECTS:mt.thread_callbacks>)
add_dependencies(${micro-profiler} dbghelp_redist)

target_link_libraries(${micro-profiler} collector patcher ipc common logger scheduler $<$<PLATFORM_ID:Windows>:injector>)
target_link_options(${micro-profiler} PRIVATE "-DEF:${CMAKE_CURRENT_SOURCE_DIR}/collector.def;-IMPLIB:${MP_OUTDIR}/${micro-profiler}.lib")

set_source_files_properties(${CALIBRATION_SOURCES} PROPERTIES
	COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GH;-Gh;-GL->"
	COMPILE_OPTIONS "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-finstrument-functions>"
)

set_source_files_properties(calls_collector.cpp calls_collector_thread.cpp PROPERTIES
	COMPILE_OPTIONS "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-arch:SSE>"
)

if(MSVC)
	add_library(micro-profiler SHARED IMPORTED GLOBAL)
	add_dependencies(micro-profiler ${micro-profiler})
	set_target_properties(micro-profiler PROPERTIES IMPORTED_IMPLIB ${micro-profiler}.lib)
else()
	add_library(micro-profiler ALIAS ${micro-profiler})
endif()
