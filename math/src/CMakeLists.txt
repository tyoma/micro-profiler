cmake_minimum_required(VERSION 3.13)

set(MATH_SOURCES
	linear_scale_ruler.cpp
	log_scale_ruler.cpp
)

add_library(math STATIC ${MATH_SOURCES})
