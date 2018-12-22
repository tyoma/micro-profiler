cmake_minimum_required(VERSION 2.8)

if (WIN32)
	include_directories(../../compat)
endif()

set(IPC_TESTS_SOURCES
	helpers_sockets.cpp
	EndpointSelectorTests.cpp
	SocketsEndpointServerTests.cpp
)

if (WIN32)
	set(IPC_TESTS_SOURCES ${IPC_TESTS_SOURCES}
		COMChannelTests.cpp
		COMEndpointClientTests.cpp
		COMEndpointServerTests.cpp
		helpers_com.cpp
		IPCClientServerTests.cpp
	)	
endif()

add_library(ipc.tests SHARED ${IPC_TESTS_SOURCES})

target_link_libraries(ipc.tests ipc common test-helpers utee)
