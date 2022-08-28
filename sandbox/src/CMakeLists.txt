cmake_minimum_required(VERSION 3.13)

add_executable(${sandbox} loader.cpp)

target_link_libraries(${sandbox} ipc.spawn common)
