cmake_minimum_required(VERSION 3.13)

set(MATH_TEST_SOURCES
	DeSerializationTests.cpp
	DisplayScaleTests.cpp
	HistogramTests.cpp
	LinearScaleRulerTests.cpp
	LinearScaleTests.cpp
	LogScaleRulerTests.cpp
	LogScaleTests.cpp
	SerializationTests.cpp
	VariantScaleTests.cpp
	VariantSerializationTests.cpp
	VariantTests.cpp
)

add_library(math.tests SHARED ${MATH_TEST_SOURCES})
target_link_libraries(math.tests math test-helpers common)
