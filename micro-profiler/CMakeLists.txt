cmake_minimum_required(VERSION 2.8)

set(VSSDKROOT $ENV{VSSDK100Install}/VisualStudioIntegration)
set(VSSDKINCLUDE ${VSSDKROOT}/Common/Inc)
set(VSSDKLIB ${VSSDKROOT}/Common/Lib)
set(VSSDKBIN ${VSSDKROOT}/Tools/Bin)
set(MP_INT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

include_directories(.)
include_directories(compat)
include_directories(${VSSDKINCLUDE})
include_directories(${MP_INT_DIRECTORY})

add_custom_command(OUTPUT ${MP_INT_DIRECTORY}/commands.cto
	COMMAND "${VSSDKBIN}/vsct.exe" "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/commands.vsct" "${MP_INT_DIRECTORY}/commands.cto" -I"${VSSDKINCLUDE}"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/commands.vsct"
	COMMENT "Compiling vsct..."
)

add_custom_command(OUTPUT micro-profiler.pkgdef extension.vsixmanifest
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/..
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/make-version.cmd VERSION version.h
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/replace.cmd micro-profiler.pkgdef > $<TARGET_FILE_DIR:micro-profiler_frontend>/micro-profiler.pkgdef
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/replace.cmd extension.vsixmanifest > $<TARGET_FILE_DIR:micro-profiler_frontend>/extension.vsixmanifest
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/micro-profiler.pkgdef"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/extension.vsixmanifest"
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
	COMMENT "Transforming metadata..."
)


set(MICROPROFILER_SOURCES
	frontend/AttachToProcessDialog.cpp
	frontend/inject_profiler.cpp
	module/main.cpp
	module/micro-profiler.def
	resources/micro-profiler_ui.rc
	setup/environment.cpp
	visualstudio/dispatch.cpp
	visualstudio/vs-extension/async.cpp
	visualstudio/vs-extension/commands-global.cpp
	visualstudio/vs-extension/commands-instance.cpp
	visualstudio/vs-extension/helpers.cpp
	visualstudio/vs-extension/vs-package.cpp
	visualstudio/vs-extension/vs-pane.cpp

	commands.cto
	extension.vsixmanifest
	micro-profiler.pkgdef
)

set(MICROPROFILER_UI_SOURCES
	resources/micro-profiler_ui.rc

	commands.cto
)

add_library(micro-profiler_frontend SHARED ${MICROPROFILER_SOURCES} commands.cto)
target_link_libraries(micro-profiler_frontend frontend.lib ipc injector common wpl.ui)

add_library(micro-profiler.ui SHARED ${MICROPROFILER_UI_SOURCES} commands.cto)
set_target_properties(micro-profiler.ui PROPERTIES LINK_FLAGS "/NOENTRY")
set_target_properties(micro-profiler.ui PROPERTIES RUNTIME_OUTPUT_DIRECTORY $<TARGET_FILE_DIR:micro-profiler_frontend>/1033)
