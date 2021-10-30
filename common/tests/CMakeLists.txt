cmake_minimum_required(VERSION 2.8)

include(test)

set(COMMON_TEST_SOURCES
	AllocatorTests.cpp
	FileStreamTests.cpp
	FileUtilitiesTests.cpp
	ImageInfoTests.cpp
	ImageUtilitiesTests.cpp
	MiscTests.cpp
	PoolAllocatorTests.cpp
	PrimitivesTests.cpp
	SerializationTests.cpp
	StreamingTests.cpp
	TextConversionTests.cpp
	TextFormattingServicesTests.cpp
	TypesTests.cpp
#	UnorderedMapTests.cpp
)

add_library(common.tests SHARED ${COMMON_TEST_SOURCES})

target_link_libraries(common.tests common mt test-helpers)
