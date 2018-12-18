cmake_minimum_required(VERSION 2.8)

set(FRONTEND_TESTS_SOURCES
	ColumnsModelTests.cpp
#	CommandTargetTests.cpp
#	FrontendManagerTests.cpp
	FunctionListTests.cpp
#	main.cpp
	OrderedViewTests.cpp
)

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(frontend.tests frontend.lib test-helpers common utee)
