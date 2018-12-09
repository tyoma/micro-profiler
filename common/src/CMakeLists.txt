cmake_minimum_required(VERSION 2.8)

set(COMMON_SOURCES
	constants.cpp
	formatting.cpp
	memory.cpp
	string.cpp
	symbol_resolver.cpp
	time.cpp
	time_generic.cpp
)

if (WIN32)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		memory_win32.cpp
		module_win32.cpp
		symbol_resolver_win32.cpp
	)	
elseif (UNIX)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		elf/filemapping_unix.cpp
		elf/sym-elf.cpp
		memory_unix.cpp
		module_unix.cpp
		symbol_resolver_unix.cpp
	)		
endif()

add_library(common STATIC ${COMMON_SOURCES})

if (WIN32)
	target_link_libraries(common dbghelp)
elseif (UNIX)
	target_link_libraries(common dl)
endif()
