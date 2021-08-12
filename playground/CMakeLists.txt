cmake_minimum_required(VERSION 2.8)

include_directories(../collector)
link_directories($<TARGET_FILE_DIR:playground>)


add_executable(playground main.cpp)
set_property(TARGET playground APPEND PROPERTY COMPILE_OPTIONS -O0 -fpatchable-function-entry=24,22)

# target_link_libraries(playground micro-profiler)

# set_property(TARGET playground APPEND PROPERTY COMPILE_OPTIONS $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fno-inline -finstrument-functions>)
set_property(TARGET playground APPEND PROPERTY COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/Ob0 /GH /Gh>)
