cmake_minimum_required(VERSION 2.8)

include_directories(../collector)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -finstrument-functions -pie -fPIE")
	if(NOT ANDROID)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-inline")
	endif()
elseif(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Ob0 /GH /Gh")
endif()


add_executable(playground main.cpp ../collector/src/micro-profiler.initializer.cpp)
target_link_libraries(playground ${micro-profiler})
