include_directories(${PROJECT_SOURCE_DIR}/libraries/utee)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti")
endif()

link_libraries(utee)
