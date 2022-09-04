cmake_minimum_required(VERSION 3.13)

set(LOGGER_TEST_SOURCES
	LogTests.cpp
	MultiThreadedLoggerTests.cpp
	WriterTests.cpp
)

add_library(logger.tests SHARED ${LOGGER_TEST_SOURCES})
target_link_libraries(logger.tests logger common mt test-helpers)
