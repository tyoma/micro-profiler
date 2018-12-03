cmake_minimum_required(VERSION 2.8)

set(COLLECTOR_TESTS_SOURCES
	AnalyzerTests.cpp
#	BinaryImageTests.cpp
#	COMChannelTests.cpp
	CallsCollectorTests.cpp
	CallsCollectorThreadTests.cpp
#	FrontendControllerTests.cpp
#	ImageLoadQueueTests.cpp
#	ImagePatchTests.cpp
	SerializationTests.cpp
	ShadowStackTests.cpp
#	StatisticsBridgeTests.cpp
	mocks.cpp
#	mocks_com.cpp
	helpers.h
	mocks.h
#	mocks_com.h
)

add_library(collector.tests SHARED ${COLLECTOR_TESTS_SOURCES})
target_link_libraries(collector.tests collector.lib common patcher mt test-helpers utee)
