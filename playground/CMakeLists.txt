cmake_minimum_required(VERSION 2.8)

include_directories(../collector)
link_directories($<TARGET_FILE_DIR:playground>)

add_executable(playground main.cpp ../collector/src/micro-profiler.initializer.cpp)

target_link_libraries(playground micro-profiler)

set_property(TARGET playground APPEND PROPERTY COMPILE_OPTIONS $<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-fno-inline -finstrument-functions -pie -fPIE>)
set_property(TARGET playground APPEND PROPERTY COMPILE_OPTIONS $<$<CXX_COMPILER_ID:MSVC>:/Ob0 /GH /Gh>)
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pie -fPIE")
