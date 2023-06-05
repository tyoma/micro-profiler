cmake_minimum_required(VERSION 3.13)

set(COMMON_TEST_SOURCES
	AllocatorTests.cpp
	ExecutableAllocatorTests.cpp
	FileStreamTests.cpp
	FileUtilitiesTests.cpp
	GapSearcherTests.cpp
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
	VirtualMemoryTests.cpp
)

if (NOT APPLE)
    set(COMMON_TESTS_SOURCES ${COMMON_TESTS_SOURCES}
        ImageInfoTests.cpp
    )
endif()

add_library(common.tests SHARED ${COMMON_TEST_SOURCES})
target_link_libraries(common.tests common mt test-helpers)
