cmake_minimum_required(VERSION 2.8)

set(COMMON_TEST_SOURCES
	AllocatorTests.cpp
	ImageInfoTests.cpp
	ImageUtilitiesTests.cpp
	MiscTests.cpp
	PrimitivesTests.cpp
	SerializationTests.cpp
	SymbolResolverTests.cpp
	TextFormattingServicesTests.cpp
)

add_library(common.tests SHARED ${COMMON_TEST_SOURCES})

target_link_libraries(common.tests common test-helpers utee)
