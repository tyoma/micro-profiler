cmake_minimum_required(VERSION 3.13)

include(asm)

include(conan)

conan_cmake_run(REQUIRES capstone/4.0.2
	BUILD missing
	BASIC_SETUP
	CMAKE_TARGETS
	OPTIONS capstone:arm=False;capstone:evm=False;capstone:m680x=False;capstone:m68k=False;capstone:mips=False;capstone:ppc=False;capstone:sparc=False;capstone:sysz=False;capstone:tms320c64x=False;capstone:xcore=False;)

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
target_link_libraries(patcher CONAN_PKG::capstone)
target_compile_definitions(patcher PUBLIC SDB_NO_SIGNALS)
