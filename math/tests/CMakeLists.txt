cmake_minimum_required(VERSION 2.8)

include(test)

set(MATH_TEST_SOURCES
	DeSerializationTests.cpp
	DisplayScaleTests.cpp
	HistogramTests.cpp
	ScaleTests.cpp
	SerializationTests.cpp
)

add_library(math.tests SHARED ${MATH_TEST_SOURCES})

target_link_libraries(math.tests test-helpers common)
