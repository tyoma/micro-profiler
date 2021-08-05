cmake_minimum_required(VERSION 2.8)

include(asm)

include_directories(../../libraries/lend)

set(PATCHER_SOURCES
	dynamic_hooking.cpp
	exceptions.cpp
	function_patch.cpp
	image_patch_manager.cpp
	intel/binary_translation_x86.cpp
	intel/jump.cpp
	intel/trampoline${ASMEXT}
	jumper.cpp
	../../libraries/lend/ld32.c
)

add_library(patcher STATIC ${PATCHER_SOURCES})
