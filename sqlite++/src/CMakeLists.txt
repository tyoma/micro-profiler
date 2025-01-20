cmake_minimum_required(VERSION 3.13)

find_package(SQLite3 REQUIRED)

add_library(sqlite++ INTERFACE)
target_link_libraries(sqlite++ INTERFACE SQLite::SQLite3)
