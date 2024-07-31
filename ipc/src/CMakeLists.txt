cmake_minimum_required(VERSION 3.13)

set(IPC_SOURCES
	client_endpoint.cpp
	client_endpoint_sockets.cpp
	client_endpoint_spawn.cpp
	client_session.cpp
	marshalled_server.cpp
	marshalled_session.cpp
	misc.cpp
	server_endpoint.cpp
	server_endpoint_sockets.cpp
	server_session.cpp
)

if (WIN32)
	set(IPC_SOURCES ${IPC_SOURCES}
		client_endpoint_com.cpp
		client_endpoint_spawn_win32.cpp
		com_init.cpp
		server_endpoint_com.cpp
		socket_helpers_win32.cpp
	)
elseif(UNIX)
	set(IPC_SOURCES ${IPC_SOURCES}
		client_endpoint_spawn_unix.cpp
		socket_helpers_unix.cpp
	)
endif()

add_library(ipc STATIC ${IPC_SOURCES})
target_link_libraries(ipc tasker strmd $<$<PLATFORM_ID:Windows>:ws2_32>)

add_library(ipc.spawn STATIC server_endpoint_spawn.cpp)
target_link_libraries(ipc.spawn mt)
