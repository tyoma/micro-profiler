cmake_minimum_required(VERSION 2.8)

if (MSVC)
	enable_language(ASM_MASM)
else()
	enable_language(ASM-ATT)
endif()

include_directories(../../libraries/lend)

set(PATCHER_SOURCES
	dynamic_hooking.cpp
	function_patch.cpp
	image_patch_manager.cpp
	intel/binary_translation_x86.cpp
	intel/jump.cpp
	../../libraries/lend/ld32.c
)

if (MSVC)
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(PATCHER_SOURCES ${PATCHER_SOURCES}
			intel/trampoline_x86.asm
		)
	else()
		set(PATCHER_SOURCES ${PATCHER_SOURCES}
			intel/trampoline_x64.asm
		)
	endif()
else()
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(PATCHER_SOURCES ${PATCHER_SOURCES}
			intel/trampoline_x86.s
		)
		set_property(SOURCE intel/trampoline_x86.s APPEND_STRING PROPERTY COMPILE_FLAGS " --32 ")
	else()
		set(PATCHER_SOURCES ${PATCHER_SOURCES}
			intel/trampoline_x64.s
		)
		set_property(SOURCE intel/trampoline_x64.s APPEND_STRING PROPERTY COMPILE_FLAGS " --64 ")
	endif()
endif()

add_library(patcher STATIC ${PATCHER_SOURCES})
