cmake_minimum_required(VERSION 2.8)

include(test)

add_executable(patcher.benchmark benchmark.cpp)

target_link_libraries(patcher.benchmark patcher common)
