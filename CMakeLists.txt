
cmake_minimum_required(VERSION 2.8)

project(micro-profiler)


set(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/_bin)


# Setup include directories
include_directories(.)
include_directories(./libraries/agge)
include_directories(./libraries/strmd)
include_directories(./libraries/wpl)
if (WIN32)
	include_directories(./compat)
endif()


# Add definitions: global, platform-based, compiler-based
if(UNIX)
	set(MP_MT_GENERIC 1)
	add_definitions(-DMP_MT_GENERIC)
	add_definitions(-DMP_NO_EXCEPTIONS) # Before DWARF exception model is supported...
endif()
if (MSVC)
	if (MSVC_VERSION LESS 1900)
		add_definitions(-Dsnprintf=_snprintf)
	endif()
	add_definitions(-D_SCL_SECURE_NO_WARNINGS -D_CRT_SECURE_NO_WARNINGS -D_SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING -D_WINSOCK_DEPRECATED_NO_WARNINGS)
	add_definitions(-DUNICODE -D_UNICODE)
endif()

# Setup global compiler/linker settings
set(GCC_CLANG_COMPILER_OPTIONS_C
	-g -fPIC -fvisibility=hidden -ffunction-sections -fdata-sections
	-Wall -Wno-deprecated-declarations -Wno-unknown-pragmas
)
set(GCC_CLANG_COMPILER_OPTIONS_CXX ${GCC_CLANG_COMPILER_OPTIONS_C}
	-std=c++11 -fexceptions -fno-rtti
	-Wno-delete-non-virtual-dtor
)
set(MSVC_COMPILER_OPTIONS /W4 /wd4503)
set(MSVC_COMPILER_OPTIONS_DEBUG ${MSVC_COMPILER_OPTIONS} /MTd)
set(MSVC_COMPILER_OPTIONS_RELEASE ${MSVC_COMPILER_OPTIONS} /MT /GL)
set(MSVC_COMPILER_OPTIONS_RELEASE_MAX /Ox /Ob2 /Oi /Ot /Oy)

add_compile_options(
	"$<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:C>:${GCC_CLANG_COMPILER_OPTIONS_C}>>"
	"$<$<CXX_COMPILER_ID:GNU>:$<$<COMPILE_LANGUAGE:CXX>:${GCC_CLANG_COMPILER_OPTIONS_CXX}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:${MSVC_COMPILER_OPTIONS_DEBUG}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:${MSVC_COMPILER_OPTIONS_RELEASE}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<OR:$<CONFIG:Release>,$<CONFIG:RelWithDebInfo>>:${MSVC_COMPILER_OPTIONS_RELEASE_MAX}>>"
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--no-undefined")
elseif(MSVC)
	add_link_options(/INCREMENTAL:NO /SAFESEH:NO)
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/LTCG>)
	add_link_options($<$<NOT:$<CONFIG:Debug>>:/OPT:REF>)
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} /LTCG)
	set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} /LTCG)
	set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_STATIC_LINKER_FLAGS_MINSIZEREL} /LTCG)
endif()

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

# Add subdirectories
add_subdirectory(libraries/utee/src)
add_subdirectory(libraries/agge)
add_subdirectory(collector/src)
add_subdirectory(common/src)
add_subdirectory(frontend/src)
add_subdirectory(ipc/src)
add_subdirectory(mt/src)
add_subdirectory(patcher/src.x86)
add_subdirectory(playground)

add_subdirectory(collector/tests)
add_subdirectory(common/tests)
add_subdirectory(frontend/tests)
add_subdirectory(ipc/tests)
add_subdirectory(micro-profiler.tests/guineapigs)
add_subdirectory(micro-profiler.tests)
if((NOT ANDROID_ABI OR ANDROID_ABI MATCHES "x86") AND CMAKE_SIZEOF_VOID_P EQUAL 4)
	add_subdirectory(patcher/tests)
endif()
add_subdirectory(test-helpers/src)

if (WIN32)
	add_subdirectory(injector/src)
	add_subdirectory(libraries/wpl/src/wpl.ui)
	add_subdirectory(micro-profiler)
	add_subdirectory(standalone)

	add_subdirectory(mt/tests)
endif()
