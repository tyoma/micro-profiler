cmake_minimum_required(VERSION 2.8)

include(test)

set(IPC_TESTS_SOURCES
	ClientSessionTests.cpp
	EndpointSelectorTests.cpp
	helpers_sockets.cpp
	MarshalledActiveSessionTests.cpp
	MarshalledServerTests.cpp
	ServerSessionContinuationTests.cpp
	ServerSessionTests.cpp
	SocketsEndpointClientTests.cpp
	SocketsEndpointServerTests.cpp
)

if (WIN32)
	set(IPC_TESTS_SOURCES ${IPC_TESTS_SOURCES}
		COMEndpointClientTests.cpp
		COMEndpointServerTests.cpp
		helpers_com.cpp
		SpawnEndpointClientTests.cpp
	)

	add_executable(guinea_ipc_spawn guinea_ipc_spawn.cpp)

	target_link_libraries(guinea_ipc_spawn mt)
	
endif()

add_library(ipc.tests SHARED ${IPC_TESTS_SOURCES})

target_link_libraries(ipc.tests ipc logger common test-helpers scheduler)
