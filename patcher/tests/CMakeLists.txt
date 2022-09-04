cmake_minimum_required(VERSION 3.13)

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
	"$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-hotpatch>"
)
add_link_options("$<$<CXX_COMPILER_ID:MSVC>:-functionpadmin:24>")

set_source_files_properties(guineapigs.cpp PROPERTIES
	COMPILE_FLAGS "$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:-O0>"
	COMPILE_OPTIONS "$<$<CXX_COMPILER_ID:MSVC>:-Od>"
)

add_library(patcher.tests SHARED ${PATCHER_TEST_SOURCES})
target_link_libraries(patcher.tests patcher common test-helpers)
