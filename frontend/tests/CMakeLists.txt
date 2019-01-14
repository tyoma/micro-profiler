cmake_minimum_required(VERSION 2.8)

set(FRONTEND_TESTS_SOURCES
	ColumnsModelTests.cpp
	FrontendManagerTests.cpp
	FunctionListTests.cpp
	OrderedViewTests.cpp
	ProcessListTests.cpp
)

if(WIN32)
	set(FRONTEND_TESTS_SOURCES ${FRONTEND_TESTS_SOURCES}
		CommandTargetTests.cpp
	)
endif()

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(frontend.tests frontend.lib test-helpers common utee)
