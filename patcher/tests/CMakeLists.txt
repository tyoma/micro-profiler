cmake_minimum_required(VERSION 3.13)

include(test)

set(PATCHER_TEST_SOURCES
	BinaryTranslationTestsX86.cpp
	DynamicHookingTests.cpp
	FunctionPatchTests.cpp
	guineapigs.cpp
	ImagePatchManagerTests.cpp
	JumperIntelTests.cpp
	OffsetDisplacedReferencesTestsX86.cpp
)

add_compile_options(
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:C>:-fpatchable-function-entry=14,12>>"
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:CXX>:-fpatchable-function-entry=14,12>>"
	"$<$<CXX_COMPILER_ID:MSVC>:-Gy>"
)
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:-hotpatch>")
endif()
add_link_options("$<$<CXX_COMPILER_ID:MSVC>:-functionpadmin:24>")

set_property(SOURCE guineapigs.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O0>")
set_property(SOURCE guineapigs.cpp APPEND_STRING PROPERTY COMPILE_FLAGS "$<$<CXX_COMPILER_ID:MSVC>:/Od>")

add_library(patcher.tests SHARED ${PATCHER_TEST_SOURCES})

target_link_libraries(patcher.tests patcher common test-helpers)
