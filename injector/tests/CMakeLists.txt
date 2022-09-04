cmake_minimum_required(VERSION 3.13)

set(MP_OUTDIR $<TARGET_FILE_DIR:injector.tests>)

set(INJECTOR_TEST_SOURCES
	ProcessDiscoverTests.cpp
)

add_custom_command(OUTPUT copy-guineas.x
	COMMAND ${CMAKE_COMMAND} -E copy ${MP_OUTDIR}/guinea_runner.exe ${MP_OUTDIR}/guinea_runner2.exe
	COMMAND ${CMAKE_COMMAND} -E copy ${MP_OUTDIR}/guinea_runner.exe ${MP_OUTDIR}/guinea_runner3.exe
	COMMENT "Cloning guinea runners..."
)

add_library(injector.tests SHARED ${INJECTOR_TEST_SOURCES} copy-guineas.x)
target_link_libraries(injector.tests injector ipc logger common mt test-helpers)
add_dependencies(injector.tests guinea_runner)
