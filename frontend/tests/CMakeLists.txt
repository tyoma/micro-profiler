cmake_minimum_required(VERSION 3.13)

set(FRONTEND_TESTS_SOURCES
	AsyncFileTests.cpp
	ColumnsDefinitionTests.cpp
	DataSchemeTests.cpp
	DerivedStatisticsTests.cpp
	DeSerializationTests.cpp
	FilePersistenceTests.cpp
	FrontendManagerTests.cpp
	FrontendMetadataTests.cpp
	FrontendPatcherTests.cpp
	FrontendStatisticsTests.cpp
	FrontendTests.cpp
	FunctionListPiechartTests.cpp
	FunctionListTests.cpp
	HeadersModelTests.cpp
	HierarchicalTableModelImplTests.cpp
	HierarchyAlgorithmsTests.cpp
	ImagePatchModelTests.cpp
	legacy_serialization.cpp
	mock_channel.cpp
	mocks.cpp
	PrimitivesTests.cpp
	ProcessListTests.cpp
	ProfilingPreferencesTests.cpp
	ProjectionViewTests.cpp
	RepresentationTests.cpp
	SelectionModelTests.cpp
	StatisticsHierarchyAccessTests.cpp
	SymbolResolverTests.cpp
	TableModelImplTests.cpp
	ThreadsModelTests.cpp
	TrackablesProviderTests.cpp
)

add_library(frontend.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(frontend.tests frontend test-helpers ipc logger common scheduler mt polyq)
