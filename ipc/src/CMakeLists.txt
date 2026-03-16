cmake_minimum_required(VERSION 3.13)

set(IPC_SOURCES
	marshalled_server.cpp
	marshalled_session.cpp
)

add_library(ipc STATIC ${IPC_SOURCES})
target_link_libraries(ipc cohesion-ipc)
