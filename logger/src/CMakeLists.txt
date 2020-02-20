cmake_minimum_required(VERSION 2.8)

set(LOGGER_SOURCES
	log.cpp
	multithreaded_logger.cpp
)

add_library(logger STATIC ${LOGGER_SOURCES})
