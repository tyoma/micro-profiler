cmake_minimum_required(VERSION 3.13)

include(asm)

find_package(capstone REQUIRED)

set(PATCHER_SOURCES
	dynamic_hooking.cpp
	exceptions.cpp
	image_patch_manager.cpp
	instruction_iterator.cpp
	intel/binary_translation_x86.cpp
	intel/jump.cpp
	intel/trampoline${ASMEXT}
	jumper.cpp
	translated_function_patch.cpp
)

add_library(patcher STATIC ${PATCHER_SOURCES})
target_link_libraries(patcher capstone::capstone mt)
target_compile_definitions(patcher PUBLIC SDB_NO_SIGNALS)
