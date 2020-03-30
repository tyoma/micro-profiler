cmake_minimum_required(VERSION 2.8)

include(test)

set(FRONTEND_TESTS_SOURCES
	ColumnsModelTests.cpp
	FilterViewTests.cpp
	FrontendManagerTests.cpp
	FrontendTests.cpp
	FunctionListPersistence.cpp
	FunctionListPiechartTests.cpp
	FunctionListTests.cpp
	mocks.cpp
	OrderedViewTests.cpp
	ProcessListTests.cpp
	SerializationTests.cpp
	SymbolResolverTests.cpp
	ThreadsModelTests.cpp
)

if(WIN32)
	set(FRONTEND_TESTS_SOURCES ${FRONTEND_TESTS_SOURCES}
		CommandTargetTests.cpp
	)
endif()

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(frontend.tests frontend.lib test-helpers logger common)
