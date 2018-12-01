cmake_minimum_required(VERSION 2.8)

set(PATCHER_TEST_SOURCES
	BinaryTranslationTestsX86.cpp
	DynamicHookingTests.cpp
#	FunctionPatchTests.cpp
	guineapigs.cpp
#	helpers.cpp
)

add_library(patcher.tests SHARED ${PATCHER_TEST_SOURCES})

target_link_libraries(patcher.tests patcher common test-helpers utee)
