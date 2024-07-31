cmake_minimum_required(VERSION 3.13)

set(LOGGER_SOURCES
	log.cpp
	multithreaded_logger.cpp
	writer.cpp
)

if (WIN32)
	set(LOGGER_SOURCES ${LOGGER_SOURCES}
		file_win32.cpp
	)	
elseif (UNIX)	
	set(LOGGER_SOURCES ${LOGGER_SOURCES}
		file_unix.cpp
	)		
endif()


add_library(logger STATIC ${LOGGER_SOURCES})
target_link_libraries(logger mt)
