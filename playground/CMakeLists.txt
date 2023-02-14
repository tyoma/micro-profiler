cmake_minimum_required(VERSION 3.13)

link_directories($<TARGET_FILE_DIR:playground>)

add_executable(playground main.cpp)
target_compile_options(playground
	PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GH;-Gh;-GL->"
	PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-finstrument-functions>"
)
target_link_libraries(playground $<$<PLATFORM_ID:Windows,Darwin>:micro-profiler>)

add_executable(playground-patchable main.cpp)
target_compile_options(playground
	PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GL->"
	PRIVATE "$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-hotpatch>"
	PRIVATE "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-fpatchable-function-entry=14,12>"
)
target_link_options(playground-patchable PRIVATE "$<$<CXX_COMPILER_ID:MSVC>:-functionpadmin:24>")
