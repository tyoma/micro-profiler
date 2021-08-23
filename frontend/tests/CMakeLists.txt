cmake_minimum_required(VERSION 2.8)

include(test)

set(FRONTEND_TESTS_SOURCES
	FilterViewTests.cpp
	FlattenViewTests.cpp
	FrontendManagerTests.cpp
	FrontendPatcherTests.cpp
	FrontendTests.cpp
	FunctionListPersistence.cpp
	FunctionListPiechartTests.cpp
	FunctionListTests.cpp
	HeadersModelTests.cpp
#	ImagePatchModelTests.cpp
	mocks.cpp
	OrderedViewTests.cpp
	ProcessListTests.cpp
	ProjectionViewTests.cpp
	SerializationTests.cpp
	SymbolResolverTests.cpp
	ThreadsModelTests.cpp
	TrackablesProviderTests.cpp
)

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})

target_link_libraries(frontend.tests frontend.lib test-helpers ipc logger common scheduler mt agge.text)
