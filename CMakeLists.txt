cmake_minimum_required(VERSION 3.13)

project(micro-profiler)

option(MP_NO_TESTS "Do not build test modules." OFF)
option(MP_ENABLE_PATCHABLE "Make all functions patchable." OFF)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/build.props ${PROJECT_SOURCE_DIR}/libraries/wpl.vs/build.props)

include(architecture)
include(unipath)

set(injector "micro-profiler_injector_${archid}")
set(micro-profiler "micro-profiler_${archid}")
set(sandbox "micro-profiler_sandbox_${archid}")

enable_testing()

set(SPECIFIC_CONFIGS CMAKE_C_FLAGS_DEBUG CMAKE_CXX_FLAGS_DEBUG CMAKE_C_FLAGS_RELEASE CMAKE_CXX_FLAGS_RELEASE CMAKE_C_FLAGS_RELWITHDEBINFO CMAKE_CXX_FLAGS_RELWITHDEBINFO CMAKE_C_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_MINSIZEREL)
foreach(v ${SPECIFIC_CONFIGS})
   string(REPLACE /MDd /MTd ${v} ${${v}})
   string(REPLACE /MD /MT ${v} ${${v}})
endforeach()

# Setup include directories
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories($<$<PLATFORM_ID:Windows>:${CMAKE_CURRENT_LIST_DIR}/compat/win32>)


# Add definitions: global, platform-based, compiler-based
set(AGGE_NO_TESTS ON)
set(POLYQ_NO_TESTS ON)
set(STRMD_NO_TESTS ON)
set(UTEE_NO_TESTS ON)
set(UTFIA_NO_TESTS ON)
set(WPL_NO_TESTS ON)
set(WPL_VS_NO_TESTS ON)

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
set(MSVC_COMPILER_OPTIONS -wd4503;-wd4996;-wd4510;-wd4610;-wd4512;-wd4312;-wd4481;-wd4355)
set(MSVC_COMPILER_OPTIONS ${MSVC_COMPILER_OPTIONS} -MP;-W4;-GR-;-GS-;-Gy;-Zi)
set(MSVC_COMPILER_OPTIONS_DEBUG ${MSVC_COMPILER_OPTIONS})
set(MSVC_COMPILER_OPTIONS_RELEASE ${MSVC_COMPILER_OPTIONS} -GL)
set(MSVC_COMPILER_OPTIONS_RELEASE_MAX -Ox;-Ob2;-Oi;-Ot;-Oy)

add_compile_options(
	"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:$<$<COMPILE_LANGUAGE:C>:${GCC_CLANG_COMPILER_OPTIONS_C}>>"
	"$<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:$<$<COMPILE_LANGUAGE:CXX>:${GCC_CLANG_COMPILER_OPTIONS_CXX}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Debug>:${MSVC_COMPILER_OPTIONS_DEBUG}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:${MSVC_COMPILER_OPTIONS_RELEASE}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:Release,RelWithDebInfo>:${MSVC_COMPILER_OPTIONS_RELEASE_MAX}>>"
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>:-arch:SSE2>>"
)

add_link_options(
	"$<$<CXX_COMPILER_ID:GNU>:-Wl,--no-undefined;-Wl,--gc-sections>"
	"$<$<CXX_COMPILER_ID:GNU>:$<$<CONFIG:Release>:-Wl,--strip-debug>>"
	"$<$<CXX_COMPILER_ID:Clang>:-Wl,-undefined,error>"
	"$<$<CXX_COMPILER_ID:MSVC>:-INCREMENTAL:NO;-SAFESEH:NO>"
	"$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_LESS:${MSVC_VERSION},1910>>:-DEBUG>" # Less than Visual Studio 2017 (15.*)
	"$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<VERSION_GREATER_EQUAL:${MSVC_VERSION},1910>>:-DEBUG:FULL>" # Visual Studio 2017 (15.*) and up
	"$<$<CXX_COMPILER_ID:MSVC>:$<$<NOT:$<CONFIG:Debug>>:-LTCG;-OPT:REF>>"
)

if (MP_ENABLE_PATCHABLE)
	add_compile_options(
		"$<$<AND:$<CXX_COMPILER_ID:MSVC>,$<EQUAL:4,${CMAKE_SIZEOF_VOID_P}>>:-hotpatch>"
		"$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fno-inline;-fpatchable-function-entry=14,12>"
	)
	add_link_options("$<$<CXX_COMPILER_ID:MSVC>:-functionpadmin:24>")
endif()

if (MSVC)
	set(CMAKE_STATIC_LINKER_FLAGS_RELEASE ${CMAKE_STATIC_LINKER_FLAGS_RELEASE} -LTCG)
	set(CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO ${CMAKE_STATIC_LINKER_FLAGS_RELWITHDEBINFO} -LTCG)
endif ()

if (UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif ()


# Add subdirectories
add_subdirectory(libraries/utee)

add_subdirectory(libraries/polyq)
add_subdirectory(libraries/strmd)
add_subdirectory(libraries/utfia)

add_subdirectory(libraries/agge)

add_subdirectory(libraries/wpl)

add_subdirectory(collector/src)
add_subdirectory(common/src)
add_subdirectory(frontend/src)
add_subdirectory(ipc/src)
add_subdirectory(logger/src)
add_subdirectory(math/src)
add_subdirectory(mt/src)
add_subdirectory(patcher/src)
add_subdirectory(playground)
add_subdirectory(sandbox/src)
add_subdirectory(scheduler/src)
add_subdirectory(sqlite++/src)

if (NOT MP_NO_TESTS)
	add_subdirectory(collector/tests)
	add_subdirectory(common/tests)
	add_subdirectory(frontend/tests)
	add_subdirectory(ipc/tests)
	add_subdirectory(logger/tests)
	add_subdirectory(math/tests)
	add_subdirectory(micro-profiler.tests/guineapigs)
	add_subdirectory(micro-profiler.tests)
	add_subdirectory(mt/tests)
	add_subdirectory(patcher/benchmark)
	add_subdirectory(patcher/tests)
	add_subdirectory(reqm/tests)
	add_subdirectory(scheduler/tests)
	add_subdirectory(sdb/tests)
	add_subdirectory(sqlite++/tests)
	add_subdirectory(test-helpers/src)
	add_subdirectory(views/tests)
endif()

if (WIN32 OR APPLE)
	add_subdirectory(standalone)

	if (WIN32)
		add_subdirectory(libraries/wpl.vs)
		
		add_subdirectory(explorer/src)
		add_subdirectory(injector/src)
		
		add_subdirectory(micro-profiler)

		if (NOT MP_NO_TESTS)
			add_subdirectory(explorer/tests)
			add_subdirectory(injector/tests)
		endif()
	endif()
endif()

if (NOT MP_NO_TESTS)
	# Declare tests/benchmarks
	set(MP_TEST_SUITES collector common frontend ipc logger math micro-profiler mt patcher reqm savant_db scheduler sqlite++ views)
	if (WIN32)
		set(MP_TEST_SUITES ${MP_TEST_SUITES} explorer injector)
	endif()

	foreach(t ${MP_TEST_SUITES})
		set(x "${t}.tests")
		add_utee_test(${x})
	endforeach()
	add_test(NAME patcher.benchmark COMMAND $<TARGET_FILE:patcher.benchmark>)
endif()
