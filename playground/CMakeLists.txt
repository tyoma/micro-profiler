cmake_minimum_required(VERSION 2.8)

include_directories(../collector)

add_executable(playground main.cpp ../collector/src/micro-profiler.initializer.cpp)
target_link_libraries(playground micro-profiler)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	target_compile_options(playground PRIVATE -fno-inline -finstrument-functions)
elseif(MSVC)
	target_compile_options(playground PRIVATE /Ob0 /GH /Gh)
endif()
