cmake_minimum_required(VERSION 3.13)

link_directories($<TARGET_FILE_DIR:playground>)

add_executable(playground main.cpp)
target_link_libraries(playground micro-profiler)

set_target_properties(playground PROPERTIES
	COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GH;-Gh;-GL->"
	COMPILE_OPTIONS "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-finstrument-functions>"
)

add_executable(playground-patchable main.cpp)
set_target_properties(playground-patchable PROPERTIES
	COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GL->"
	COMPILE_OPTIONS "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-hotpatch>"
	LINK_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-functionpadmin:24>"
	
	COMPILE_OPTIONS "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-fpatchable-function-entry=14,12>"
)
