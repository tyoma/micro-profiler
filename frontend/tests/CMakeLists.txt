cmake_minimum_required(VERSION 2.8)

include(test)

set(FRONTEND_TESTS_SOURCES
	ColumnsModelTests.cpp
	FrontendManagerTests.cpp
	FunctionListTests.cpp
	mocks.cpp
	OrderedViewTests.cpp
	ProcessListTests.cpp
	SymbolResolverTests.cpp
)

if(WIN32)
	set(FRONTEND_TESTS_SOURCES ${FRONTEND_TESTS_SOURCES}
		CommandTargetTests.cpp
	)
endif()

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(frontend.tests frontend.lib test-helpers logger common)
