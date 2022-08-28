cmake_minimum_required(VERSION 3.13)

include(asm)

set(PATCHER_SOURCES
	dynamic_hooking.cpp
	exceptions.cpp
	image_patch_manager.cpp
	intel/binary_translation_x86.cpp
	intel/jump.cpp
	intel/trampoline${ASMEXT}
	jumper.cpp
)

add_library(patcher STATIC ${PATCHER_SOURCES})
