cmake_minimum_required(VERSION 2.8)

include(test)

set(COLLECTOR_TESTS_SOURCES
	AnalyzerTests.cpp
	CallsCollectorTests.cpp
	CallsCollectorThreadTests.cpp
	CollectorAppTests.cpp
	ImagePatchTests.cpp
	ModuleTrackerTests.cpp
	SerializationTests.cpp
	ShadowStackTests.cpp
	StatisticsBridgeTests.cpp
	mocks.cpp
	mocks_image_info.cpp
)

set_property(SOURCE mocks.cpp PREPEND_STRING PROPERTY COMPILE_FLAGS " -fno-rtti ")

add_library(collector.tests SHARED ${COLLECTOR_TESTS_SOURCES})
target_link_libraries(collector.tests collector.lib common patcher mt test-helpers)
