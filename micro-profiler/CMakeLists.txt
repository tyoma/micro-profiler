cmake_minimum_required(VERSION 2.8)

set(VSSDKROOT $ENV{VSSDK100Install}/VisualStudioIntegration)
set(VSSDKINCLUDE ${VSSDKROOT}/Common/Inc)
set(VSSDKLIB ${VSSDKROOT}/Common/Lib)
set(VSSDKBIN ${VSSDKROOT}/Tools/Bin)
set(MP_INTDIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR})
set(MP_OUTDIR $<TARGET_FILE_DIR:micro-profiler_frontend>)

include_directories(.)
include_directories(compat)
include_directories(${VSSDKINCLUDE})
include_directories(${MP_INTDIR})

add_custom_command(OUTPUT ${MP_INTDIR}/commands.cto
	COMMAND "${VSSDKBIN}/vsct.exe" "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/commands.vsct" "${MP_INTDIR}/commands.cto" -I"${VSSDKINCLUDE}"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/commands.vsct"
	COMMENT "Compiling vsct..."
)

add_custom_command(OUTPUT micro-profiler.pkgdef
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/..
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/make-version.cmd VERSION version.h
	COMMAND cd ${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/replace.cmd micro-profiler.pkgdef > ${MP_OUTDIR}/micro-profiler.pkgdef
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/replace.cmd extension.vsixmanifest > ${MP_OUTDIR}/extension.vsixmanifest
	COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../scripts/replace.cmd extension.vs10.vsixmanifest > ${MP_OUTDIR}/extension.vs10.vsixmanifest
	COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/logo.ico ${MP_OUTDIR}/.
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/micro-profiler.pkgdef"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/extension.vsixmanifest"
	DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/visualstudio/vs-extension/extension.vs10.vsixmanifest"
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

	../frontend/src/untested/resources/frontend.rc
	
	micro-profiler.pkgdef
	${MP_INTDIR}/commands.cto
)

set(MICROPROFILER_UI_SOURCES
	resources/micro-profiler_ui.rc

	${MP_INTDIR}/commands.cto
)

add_library(micro-profiler_frontend SHARED ${MICROPROFILER_SOURCES})
add_library(micro-profiler.ui SHARED ${MICROPROFILER_UI_SOURCES})

target_link_libraries(micro-profiler_frontend frontend.lib ipc injector common wpl.ui)

set_target_properties(micro-profiler.ui PROPERTIES
	LINK_OPTIONS /NOENTRY
	RUNTIME_OUTPUT_DIRECTORY ${MP_OUTDIR}/1033
)