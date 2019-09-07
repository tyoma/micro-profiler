cmake_minimum_required(VERSION 2.8)

include(test)

set(IPC_TESTS_SOURCES
	helpers_sockets.cpp
	EndpointSelectorTests.cpp
	SocketsEndpointClientTests.cpp
	SocketsEndpointServerTests.cpp
)

if (WIN32)
	set(IPC_TESTS_SOURCES ${IPC_TESTS_SOURCES}
		COMEndpointClientTests.cpp
		COMEndpointServerTests.cpp
		helpers_com.cpp
	)	
endif()

add_library(ipc.tests SHARED ${IPC_TESTS_SOURCES})

target_link_libraries(ipc.tests ipc common test-helpers)
