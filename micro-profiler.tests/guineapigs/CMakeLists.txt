cmake_minimum_required(VERSION 3.13)

include_directories(../../collector)
link_directories($<TARGET_FILE_DIR:guinea_runner>)

add_compile_options(
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>>:-O0;-fno-inline;-fpatchable-function-entry=14,12>"
	"$<$<CXX_COMPILER_ID:MSVC>:-Od;-Ob0>"
	"$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-hotpatch>"
)

add_library(symbol_container_1 SHARED symbol_container_1.cpp unload_tracker.cpp)
add_library(symbol_container_2 SHARED symbol_container_2.cpp symbol_container_2_internal.cpp unload_tracker.cpp)
if (MSVC)
	set_target_properties(symbol_container_1 symbol_container_2 PROPERTIES LINK_FLAGS "-functionpadmin:24")
endif()

add_library(symbol_container_2_instrumented SHARED symbol_container_2.cpp symbol_container_2_internal.cpp)
target_compile_options(symbol_container_2_instrumented PUBLIC $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-finstrument-functions>)
target_compile_options(symbol_container_2_instrumented PUBLIC $<$<CXX_COMPILER_ID:MSVC>:-GH;-Gh>)
target_link_libraries(symbol_container_2_instrumented micro-profiler)

add_library(symbol_container_3_nosymbols SHARED symbol_container_3.cpp)
target_link_options(symbol_container_3_nosymbols PRIVATE $<$<CXX_COMPILER_ID:MSVC>:-DEBUG:NONE>)

add_executable(guinea_runner guinea_runner.cpp)
target_link_libraries(guinea_runner ipc ipc.spawn test-helpers common logger scheduler)
