cmake_minimum_required(VERSION 2.8)

set(SCHEDULER_SOURCES
	task_queue.cpp
)

add_library(scheduler STATIC ${SCHEDULER_SOURCES})
