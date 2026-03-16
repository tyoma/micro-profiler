cmake_minimum_required(VERSION 3.13)

set(IPC_TESTS_SOURCES
	MarshalledActiveSessionTests.cpp
	MarshalledServerTests.cpp
)

add_library(ipc.tests SHARED ${IPC_TESTS_SOURCES})
target_link_libraries(ipc.tests ipc logger common test-helpers)
