cmake_minimum_required(VERSION 2.8)

include_directories(../../collector)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	add_compile_options(-O0)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default")

	add_library(symbol_container_1 SHARED symbol_container_1.cpp)
	add_library(symbol_container_2 SHARED symbol_container_2.cpp symbol_container_2_internal.cpp)
	add_library(symbol_container_2_instrumented SHARED symbol_container_2.cpp symbol_container_2_internal.cpp ../../collector/src/micro-profiler.initializer.cpp)
	add_library(symbol_container_3_nosymbols SHARED symbol_container_3.cpp)

	target_compile_options(symbol_container_2_instrumented PRIVATE -finstrument-functions)
	
	set_target_properties(symbol_container_1 PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbol_container_1.expmap") 
	set_target_properties(symbol_container_2 symbol_container_2_instrumented PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbol_container_2.expmap") 
	set_target_properties(symbol_container_3_nosymbols PROPERTIES LINK_FLAGS " -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/symbol_container_3.expmap") 
elseif(MSVC)
	add_compile_options(/Od)
	
	add_library(symbol_container_1 SHARED symbol_container_1.cpp symbol_container_1.def)
	add_library(symbol_container_2 SHARED symbol_container_2.cpp symbol_container_2_internal.cpp symbol_container_2.def)
	add_library(symbol_container_2_instrumented SHARED symbol_container_2.cpp symbol_container_2_internal.cpp ../../collector/src/micro-profiler.initializer.cpp symbol_container_2.def)
	add_library(symbol_container_3_nosymbols SHARED symbol_container_3.cpp symbol_container_3.def)

	target_compile_options(symbol_container_2_instrumented PRIVATE /GH /Gh)
	
	set_target_properties(symbol_container_1 symbol_container_2 symbol_container_2_instrumented symbol_container_3_nosymbols PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
	set_target_properties(symbol_container_1 symbol_container_2 symbol_container_2_instrumented PROPERTIES LINK_FLAGS "/DEBUG")
	set_target_properties(symbol_container_3_nosymbols PROPERTIES LINK_FLAGS "/DEBUG:NONE")
endif()

target_link_libraries(symbol_container_2_instrumented micro-profiler)

add_executable(guinea_runner guinea_runner.cpp)
target_link_libraries(guinea_runner ipc test-helpers common utee)
