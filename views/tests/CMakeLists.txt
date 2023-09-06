cmake_minimum_required(VERSION 3.13)

set(FRONTEND_TESTS_SOURCES
	AnyKeyTests.cpp
	FilterViewTests.cpp
	OrderedViewTests.cpp
)

add_library(views.tests SHARED ${FRONTEND_TESTS_SOURCES})
target_link_libraries(views.tests test-helpers)
