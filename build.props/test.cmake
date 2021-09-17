include_directories(${CMAKE_CURRENT_LIST_DIR}/../libraries/utee)

set(GCC_CLANG_COMPILER_OPTIONS_C
	-Wno-pedantic -Wno-all -Wno-extra
)
set(GCC_CLANG_COMPILER_OPTIONS_CXX ${GCC_CLANG_COMPILER_OPTIONS_C}
)

add_compile_options(
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:C>:${GCC_CLANG_COMPILER_OPTIONS_C}>>"
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:CXX>:${GCC_CLANG_COMPILER_OPTIONS_CXX}>>"
)

link_libraries(utee)
