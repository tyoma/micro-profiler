cmake_minimum_required(VERSION 2.8)

if(NOT ANDROID_ABI OR ANDROID_ABI MATCHES "x86.*")
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		if(WIN32)
			set(micro-profiler "micro-profiler_Win32")
		else()
			set(micro-profiler "micro-profiler_x86")
		endif()
	else()
		set(micro-profiler "micro-profiler_x64")
	endif()
elseif(ANDROID_ABI MATCHES "arm.*")
	set(micro-profiler "micro-profiler_arm")
endif()

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
		main_win32.cpp
	)
endif()

add_library(collector.lib STATIC ${COLLECTOR_LIB_SOURCES})
add_library(${micro-profiler} SHARED ${COLLECTOR_SOURCES} $<TARGET_OBJECTS:common.thread_callbacks>)

target_link_libraries(${micro-profiler} collector.lib patcher ipc common)

if(MSVC)
	set_property(SOURCE ${CALIBRATION_SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS " /Ob0 /GH /Gh /GL- ")
	set_property(TARGET ${micro-profiler} APPEND PROPERTY LINK_OPTIONS /DEF:${CMAKE_CURRENT_SOURCE_DIR}/collector.def /IMPLIB:${MP_OUTDIR}/${micro-profiler}.lib)
	
	add_library(micro-profiler SHARED IMPORTED GLOBAL)
	add_dependencies(micro-profiler ${micro-profiler})
	if(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set_property(SOURCE calls_collector.cpp calls_collector_thread.cpp APPEND PROPERTY COMPILE_OPTIONS /arch:SSE)
	endif()
	set_property(TARGET ${micro-profiler} APPEND PROPERTY LINK_OPTIONS /MANIFEST:NO)
	set_property(TARGET micro-profiler PROPERTY IMPORTED_IMPLIB ${micro-profiler}.lib)

	add_custom_command(TARGET ${micro-profiler} POST_BUILD
		COMMAND "mt.exe" -manifest \"${CMAKE_CURRENT_SOURCE_DIR}\\com.manifest\" -outputresource:${micro-profiler}.dll\;\#2
		WORKING_DIRECTORY ${MP_OUTDIR}
		COMMENT "Adding custom manifest containing default COM proxystubs..." 
	)
else()
	set_property(SOURCE ${CALIBRATION_SOURCES} APPEND_STRING PROPERTY COMPILE_FLAGS " -fno-inline -finstrument-functions ")
	set_property(TARGET ${micro-profiler} APPEND_STRING PROPERTY LINK_FLAGS " -Wl,-znodelete -Wl,-zinterpose ")

	add_library(micro-profiler ALIAS ${micro-profiler})
endif()
