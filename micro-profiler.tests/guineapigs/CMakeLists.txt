cmake_minimum_required(VERSION 2.8)

include(test)

include_directories(../../collector)
link_directories($<TARGET_FILE_DIR:guinea_runner>)

add_compile_options(
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O0>"
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fno-inline>"
	"$<$<CXX_COMPILER_ID:MSVC>:/Od>"
	"$<$<CXX_COMPILER_ID:MSVC>:/Ob0>"
	"$<$<CXX_COMPILER_ID:MSVC>:/GL->"
)

add_library(symbol_container_1 SHARED symbol_container_1.cpp)
add_library(symbol_container_2 SHARED symbol_container_2.cpp symbol_container_2_internal.cpp)
add_library(symbol_container_2_instrumented SHARED symbol_container_2.cpp symbol_container_2_internal.cpp ../../collector/src/micro-profiler.initializer.cpp)
add_library(symbol_container_3_nosymbols SHARED symbol_container_3.cpp)
add_executable(guinea_runner guinea_runner.cpp)

target_compile_options(symbol_container_2_instrumented PUBLIC $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-finstrument-functions>)
target_compile_options(symbol_container_2_instrumented PUBLIC $<$<CXX_COMPILER_ID:MSVC>:/GH /Gh>)
#set_target_properties(symbol_container_1 symbol_container_2 symbol_container_2_instrumented symbol_container_3_nosymbols PROPERTIES LINK_FLAGS "/INCREMENTAL:NO")
#set_target_properties(symbol_container_1 symbol_container_2 symbol_container_2_instrumented PROPERTIES LINK_FLAGS "/DEBUG")
#set_target_properties(symbol_container_3_nosymbols PROPERTIES LINK_FLAGS "/DEBUG:NONE")

target_link_libraries(symbol_container_2_instrumented micro-profiler)
target_link_libraries(guinea_runner ipc test-helpers common utee)
