cmake_minimum_required(VERSION 2.8)

include(test)

set(FRONTEND_TESTS_SOURCES
	AnyKeyTests.cpp
	FilterViewTests.cpp
	FlattenViewTests.cpp
	OrderedViewTests.cpp
)

add_library(views.tests SHARED ${FRONTEND_TESTS_SOURCES})

target_link_libraries(views.tests test-helpers)
