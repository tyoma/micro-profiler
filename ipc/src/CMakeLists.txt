cmake_minimum_required(VERSION 2.8)

if (WIN32)
	include_directories(../../compat)
endif()

set(IPC_SOURCES
	endpoint.cpp
	endpoint_sockets.cpp
)

if (WIN32)
	set(IPC_SOURCES ${IPC_SOURCES}
		channel_client.cpp
		endpoint_com.cpp
		ipc_client.cpp
		ipc_server.cpp
	)	
endif()

add_library(ipc STATIC ${IPC_SOURCES})

target_link_libraries(ipc mt)

if (WIN32)
	target_link_libraries(ipc ws2_32)
endif()
