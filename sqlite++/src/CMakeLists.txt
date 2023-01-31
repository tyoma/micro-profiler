cmake_minimum_required(VERSION 3.13)

include(sqlite3.include)

add_library(sqlite++ INTERFACE)
target_link_libraries(sqlite++ INTERFACE CONAN_PKG::sqlite3)
