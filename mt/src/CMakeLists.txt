cmake_minimum_required(VERSION 2.8)

set(MT_SOURCES
	generic/event.cpp
)

if (WIN32)
	set(MT_SOURCES ${MT_SOURCES}
		win32/tls.cpp
	)
elseif (UNIX)
	set(MT_SOURCES ${MT_SOURCES}
		pthreads/tls.cpp
	)
endif()

add_library(mt STATIC ${MT_SOURCES})
