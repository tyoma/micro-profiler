cmake_minimum_required(VERSION 3.13)

include(test)

add_executable(patcher.benchmark benchmark.cpp)

target_link_libraries(patcher.benchmark patcher common)
