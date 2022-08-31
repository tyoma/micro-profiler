cmake_minimum_required(VERSION 3.13)

link_directories($<TARGET_FILE_DIR:playground>)

add_executable(playground main.cpp)

target_link_libraries(playground micro-profiler)

set_target_properties(playground PROPERTIES
	COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-Ob0;-GH;-Gh;-GL->"
	COMPILE_OPTIONS "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-finstrument-functions>"
)
