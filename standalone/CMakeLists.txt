cmake_minimum_required(VERSION 3.4)

add_definitions(-D_ATL_SINGLE_THREADED)

set(MICROPROFILER_SOURCES
	main.cpp
	ProfilerMainDialog.cpp
	static_stylesheet.cpp
)

if (WIN32)
	set(MICROPROFILER_SOURCES ${MICROPROFILER_SOURCES}
		application_win32.cpp
		resources/micro-profiler_standalone.rc
		standalone.manifest
	)
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
elseif (APPLE)
	enable_language(OBJCXX)

	set(CMAKE_OBJCXX_STANDARD 11)

	set(MICROPROFILER_SOURCES ${MICROPROFILER_SOURCES}
		application_macos.mm
	)
endif()

add_executable(micro-profiler_standalone ${MICROPROFILER_SOURCES})

target_link_libraries(micro-profiler_standalone frontend ipc logger common wpl)

if (APPLE)
	set_target_properties(micro-profiler_standalone PROPERTIES
		MACOSX_BUNDLE YES
		MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/info.plist
		XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.gevorkyan.org.micro-profiler"
		XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
		XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT "dwarf-with-dsym"
		XCODE_ATTRIBUTE_INFOPLIST_PREPROCESS YES
	)
endif()
