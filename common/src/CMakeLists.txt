cmake_minimum_required(VERSION 2.8)

set(COMMON_SOURCES
	constants.cpp
	formatting.cpp
	image_info.cpp
	memory.cpp
	serialization.cpp
	string.cpp
	time.cpp
)

if (WIN32)
	set(COMMON_SOURCES ${COMMON_SOURCES}
		file_id_win32.cpp
		image_info_win32.cpp
		memory_win32.cpp
		module_win32.cpp
		registry_configuration.cpp
		thread_monitor_win32.cpp
		time_win32.cpp
	)	
elseif (UNIX)	
	set(COMMON_SOURCES ${COMMON_SOURCES}
		elf/filemapping_unix.cpp
		elf/sym-elf.cpp
		file_id_unix.cpp
		image_info_unix.cpp
		memory_unix.cpp
		module_linux.cpp
		thread_monitor_linux.cpp
		time_generic.cpp
	)		
endif()

if (ANDROID)
	if (${ANDROID_NATIVE_API_LEVEL} LESS 21)
		include_directories(BEFORE android-compat)
		
		set(COMMON_SOURCES ${COMMON_SOURCES}
			android-compat/src/dl_iterate_phdr.c
		)
	endif()
endif()

add_library(common STATIC ${COMMON_SOURCES})

if (WIN32)
	target_link_libraries(common dbghelp)
elseif (UNIX)
	target_link_libraries(common dl)
endif()
