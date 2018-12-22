cmake_minimum_required(VERSION 2.8)

set(IPC_SOURCES
	client_endpoint.cpp
	client_endpoint_sockets.cpp
	server_endpoint.cpp
	server_endpoint_sockets.cpp
)

if (WIN32)
	set(IPC_SOURCES ${IPC_SOURCES}
		client_endpoint_com.cpp
		ipc_client.cpp
		ipc_server.cpp
		server_endpoint_com.cpp
	)	
endif()

add_library(ipc STATIC ${IPC_SOURCES})

target_link_libraries(ipc mt)

if (WIN32)
	target_link_libraries(ipc ws2_32)
endif()
