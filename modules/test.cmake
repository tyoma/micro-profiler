include_directories(${CMAKE_CURRENT_LIST_DIR}/../libraries/utee)

add_compile_options("$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-frtti>")

link_libraries(utee)
