cmake_minimum_required(VERSION 3.13)

set(COLLECTOR_TESTS_SOURCES
	ActiveServerAppTests.cpp
	AnalyzerTests.cpp
	BuffersQueueTests.cpp
	CallsCollectorTests.cpp
	CallsCollectorThreadTests.cpp
	CollectorAppPatcherTests.cpp
	CollectorAppTests.cpp
	helpers.cpp
	mocks.cpp
	ModuleTrackerTests.cpp
	SerializationTests.cpp
	ShadowStackTests.cpp
	ThreadAnalyzerTests.cpp
	ThreadMonitorTests.cpp
	ThreadQueueManagerTests.cpp
)

add_library(collector.tests SHARED ${COLLECTOR_TESTS_SOURCES} $<TARGET_OBJECTS:mt.thread_callbacks>)
target_link_libraries(collector.tests collector.lib common ipc patcher scheduler mt logger test-helpers)
