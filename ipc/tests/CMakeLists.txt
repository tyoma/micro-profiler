cmake_minimum_required(VERSION 3.13)

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
	SpawnEndpointClientTests.cpp
	SpawnEndpointServerTests.cpp
)

if (WIN32)
	set(IPC_TESTS_SOURCES ${IPC_TESTS_SOURCES}
		COMEndpointClientTests.cpp
		COMEndpointServerTests.cpp
		helpers_com.cpp
	)
endif()

add_library(ipc.tests SHARED ${IPC_TESTS_SOURCES})
add_executable(guinea_ipc_spawn guinea_ipc_spawn.cpp)
add_executable(guinea_ipc_spawn_server guinea_ipc_spawn_server.cpp)

target_link_libraries(ipc.tests ipc logger common test-helpers scheduler)
target_link_libraries(guinea_ipc_spawn mt)
target_link_libraries(guinea_ipc_spawn_server ipc.spawn ipc logger common)
