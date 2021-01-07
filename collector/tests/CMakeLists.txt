cmake_minimum_required(VERSION 2.8)

include(test)

set(COLLECTOR_TESTS_SOURCES
	AnalyzerTests.cpp
	CallsCollectorTests.cpp
	CallsCollectorThreadTests.cpp
	CollectorAppTests.cpp
	helpers.cpp
	ImagePatchTests.cpp
	ModuleTrackerTests.cpp
	SerializationTests.cpp
	ShadowStackTests.cpp
	StatisticsBridgeTests.cpp
	ThreadAnalyzerTests.cpp
	ThreadMonitorTests.cpp
	mocks.cpp
	mocks_image_info.cpp
)

add_library(collector.tests SHARED ${COLLECTOR_TESTS_SOURCES} $<TARGET_OBJECTS:mt.thread_callbacks>)
target_link_libraries(collector.tests collector.lib common patcher scheduler mt logger test-helpers)
