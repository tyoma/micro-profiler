cmake_minimum_required(VERSION 2.8)

include(test)

set(PATCHER_TEST_SOURCES
	BinaryTranslationTestsX86.cpp
	DynamicHookingTests.cpp
	FunctionPatchTests.cpp
	FunctionPatchTestsX86.cpp
	guineapigs.cpp
	helpers.cpp
	OffsetDisplacedReferencesTestsX86.cpp
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set_property(SOURCE guineapigs.cpp PREPEND_STRING PROPERTY COMPILE_FLAGS " -O0 ")
endif()

add_library(patcher.tests SHARED ${PATCHER_TEST_SOURCES})

target_link_libraries(patcher.tests patcher common test-helpers)
