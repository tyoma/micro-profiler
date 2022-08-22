cmake_minimum_required(VERSION 2.8)

include(test)

add_compile_options(
	"$<$<CXX_COMPILER_ID:MSVC>:/bigobj>"
)

set(SAVANT_DB_TESTS_SOURCES
	AggregationTests.cpp
	ImmutableIndexTests.cpp
	IndexedSerializationTests.cpp
	IntegratedIndexTests.cpp
	JoiningTests.cpp
	OrderedIndexTests.cpp
	TableSerializationTests.cpp
	TableTests.cpp
)

add_library(savant_db.tests SHARED ${SAVANT_DB_TESTS_SOURCES})

target_link_libraries(savant_db.tests test-helpers)
