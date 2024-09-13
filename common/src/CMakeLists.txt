cmake_minimum_required(VERSION 3.13)

set(COMMON_SOURCES
	allocator.cpp
	configuration_file.cpp
	constants.cpp
	file_stream.cpp
	formatting.cpp
	memory.cpp
	memory_manager.cpp
	path.cpp
	pool_allocator.cpp
	stream.cpp
	string.cpp
	time.cpp
)

if (WIN32)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		configuration_registry.cpp
		file_id_win32.cpp
		image_info_win32.cpp
		memory_win32.cpp
		module_win32.cpp
		time_win32.cpp
	)
elseif (UNIX)	
	set(COMMON_SOURCES ${COMMON_SOURCES}
		elf/filemapping_unix.cpp
		elf/sym-elf.cpp
		file_id_unix.cpp
		image_info_unix.cpp
		memory_unix.cpp
		time_generic.cpp
	)
	if (APPLE)
		set(COMMON_SOURCES ${COMMON_SOURCES}
			memory_macos.cpp
			module_macos.cpp
		)
	else()
		set(COMMON_SOURCES ${COMMON_SOURCES}
			memory_linux.cpp
			module_linux.cpp
		)
	endif()		
endif()

if (ANDROID)
	if (${ANDROID_NATIVE_API_LEVEL} LESS 21)
		include_directories(BEFORE android-compat)
		
		set(COMMON_SOURCES ${COMMON_SOURCES}
			android-compat/src/dl_iterate_phdr.c
		)
	endif()
elseif (APPLE)
	include_directories(BEFORE macos-compat)
endif()

add_library(common STATIC ${COMMON_SOURCES})
target_link_libraries(common PUBLIC strmd PUBLIC $<$<NOT:$<PLATFORM_ID:Windows>>:dl> mt PRIVATE utfia)
