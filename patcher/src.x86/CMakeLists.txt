cmake_minimum_required(VERSION 2.8)

include_directories(../../libraries/lend)

set(PATCHER_X86_SOURCES
	assembler_intel.cpp
	binary_translation_x86.cpp
	dynamic_hooking_x86.cpp
	function_patch.cpp
	../../libraries/lend/ld32.c
)

add_library(patcher STATIC ${PATCHER_X86_SOURCES})
