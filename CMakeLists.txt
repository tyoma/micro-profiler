cmake_minimum_required(VERSION 2.8)

project(micro-profiler)

option(MP_BUILD_VSPACKAGE "Build Visual Studio Package." ON)


set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/build.props)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)


# Setup include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${CMAKE_CURRENT_LIST_DIR}/libraries/agge)
include_directories(${CMAKE_CURRENT_LIST_DIR}/libraries/polyq)
include_directories(${CMAKE_CURRENT_LIST_DIR}/libraries/strmd)
include_directories(${CMAKE_CURRENT_LIST_DIR}/libraries/wpl)
include_directories(${CMAKE_CURRENT_LIST_DIR}/libraries/wpl.vs)
include_directories($<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_LIST_DIR}/compat/win32>)


# Add definitions: global, platform-based, compiler-based
set(AGGE_NO_TESTS 1)
set(UTEE_NO_TESTS 1)
set(WPL_NO_TESTS 1)

if (UNIX)
	set(MP_MT_GENERIC 1)
	add_definitions(-DMP_MT_GENERIC)
	add_definitions(-D_FILE_OFFSET_BITS=64) # Fixes problems 32-bit app on 64-bit Ubuntu
	if (APPLE)
		add_definitions(-DMSG_NOSIGNAL=0)
	endif ()
endif ()

if (UNIX OR (MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8))
	add_definitions(-DMP_NO_EXCEPTIONS) # Before DWARF exception model is supported...
endif()

if (MSVC)
	include_directories($<$<VERSION_LESS_EQUAL:${MSVC_VERSION},1600>:${CMAKE_CURRENT_LIST_DIR}/compat/msvc10->)
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	add_definitions(-DUNICODE -D_UNICODE)
endif ()

if (CMAKE_SIZEOF_VOID_P EQUAL 4)
	set(VSIXID "A42F6081-855F-41CB-8C89-9488AA847290")
else()
	set(VSIXID "24EB3F53-8909-4D6E-859B-58837349F59F")
endif ()


# Setup global compiler/linker settings
set(GCC_CLANG_COMPILER_OPTIONS_C
	-ggdb -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections
	-Wpedantic -Wall -Wextra
	-Wno-error
	-Wno-deprecated-declarations
	-Wno-unknown-pragmas
	-Wno-missing-field-initializers
	-Wno-implicit-fallthrough
)
if (NOT ANDROID_ABI OR ANDROID_ABI MATCHES "x86")
	set(GCC_CLANG_COMPILER_OPTIONS_C ${GCC_CLANG_COMPILER_OPTIONS_C} -msse2)
endif ()
set(GCC_CLANG_COMPILER_OPTIONS_CXX ${GCC_CLANG_COMPILER_OPTIONS_C}
	-std=c++11 -fexceptions -fno-rtti
	-Wno-delete-non-virtual-dtor
)
set(MSVC_COMPILER_OPTIONS /wd4503 /wd4996 /wd4510 /wd4610 /wd4512 /wd4312 /wd4481)
set(MSVC_COMPILER_OPTIONS ${MSVC_COMPILER_OPTIONS} /MP /W4 /GR- /GS- /Gy-)
set(MSVC_COMPILER_OPTIONS_DEBUG ${MSVC_COMPILER_OPTIONS} /MTd)
set(MSVC_COMPILER_OPTIONS_RELEASE ${MSVC_COMPILER_OPTIONS} /MT /GL)
set(MSVC_COMPILER_OPTIONS_RELEASE_MAX /Ox /Ob2 /Oi /Ot /Oy)

add_compile_options(
	"$<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:C>:${GCC_CLANG_COMPILER_OPTIONS_C}>>"
	"$<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:${GCC_CLANG_COMPILER_OPTIONS_CXX}>>"
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:C>:${GCC_CLANG_COMPILER_OPTIONS_C}>>"
	"$<$<OR:$<CXX_COMPILER_ID:GNU>,$<CXX_COMPILER_ID:Clang>>:$<$<COMPILE_LANGUAGE:CXX>:${GCC_CLANG_COMPILER_OPTIONS_CXX}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:${MSVC_COMPILER_OPTIONS_DEBUG}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:${MSVC_COMPILER_OPTIONS_RELEASE}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:${MSVC_COMPILER_OPTIONS_RELEASE_MAX}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>:/arch:SSE2>>"
)

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.13.5)
	add_link_options(
		"$<$<CXX_COMPILER_ID:GNU>:-Wl,--no-undefined>"
		"$<$<CXX_COMPILER_ID:Clang>:-Wl,-undefined,error>"
		"$<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:Release>:-Wl,--strip-debug>>"
		"$<$<CXX_COMPILER_ID:Clang>:$<$<OR:$<PLATFORM_ID:Apple>,$<PLATFORM_ID:Android>>:$<$<CONFIG:Release>:-Wl,-strip-debug>>>"
		"$<$<CXX_COMPILER_ID:GNU>:-Wl,--gc-sections>"
	)
endif ()

if (MSVC)
	add_link_options(/INCREMENTAL:NO /SAFESEH:NO)
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/LTCG>)
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/OPT:REF>)
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG)
	set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG)
endif ()

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()


# Add subdirectories
add_subdirectory(libraries/agge)
add_subdirectory(libraries/freetype2)
add_subdirectory(libraries/utee)

add_subdirectory(collector/src)
add_subdirectory(common/src)
add_subdirectory(frontend/src)
add_subdirectory(ipc/src)
add_subdirectory(logger/src)
add_subdirectory(math/src)
add_subdirectory(mt/src)
add_subdirectory(patcher/src)
add_subdirectory(playground)
add_subdirectory(scheduler/src)

add_subdirectory(collector/tests)
add_subdirectory(common/tests)
add_subdirectory(frontend/tests)
add_subdirectory(ipc/tests)
add_subdirectory(logger/tests)
add_subdirectory(math/tests)
add_subdirectory(micro-profiler.tests/guineapigs)
add_subdirectory(micro-profiler.tests)
add_subdirectory(mt/tests)
add_subdirectory(patcher/tests)
add_subdirectory(reqm/tests)
add_subdirectory(scheduler/tests)
add_subdirectory(test-helpers/src)
add_subdirectory(views/tests)

if (WIN32 OR APPLE)
	add_subdirectory(libraries/wpl)

	add_subdirectory(standalone)

	if (WIN32)
		add_subdirectory(injector/src)
		
		if (MB_BUILD_VSPACKAGE)
			add_subdirectory(libraries/wpl.vs)
			add_subdirectory(micro-profiler)
		endif()

		add_subdirectory(injector/tests)
	endif ()
endif ()
