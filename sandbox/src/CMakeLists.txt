cmake_minimum_required(VERSION 2.8)

add_executable(${sandbox} loader_win32.cpp)

target_link_libraries(${sandbox} ipc.spawn common)
