cmake_minimum_required(VERSION 2.8)

set(COMMON_TEST_SOURCES
	AllocatorTests.cpp
#	ImageInfoTests.cpp
	ImageUtilitiesTests.cpp
	MiscTests.cpp
	PrimitivesTests.cpp
	SerializationTests.cpp
	TextFormattingServicesTests.cpp
)

if (WIN32)
	set(COMMON_TEST_SOURCES ${COMMON_TEST_SOURCES}
		ImageInfoTests.cpp
	)
endif()

add_library(common.tests SHARED ${COMMON_TEST_SOURCES})

target_link_libraries(common.tests common test-helpers utee)
