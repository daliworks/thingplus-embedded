cmake_minimum_required(VERSION 3.0)

#project name is "Thinplus" and version is "1.0.0"
project(Thingplus VERSION 1.0.0)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
include (CodeCoverage)

#executable file ourput directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#static library output directory
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
#shared library output directory
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set (THINGPLUS_LIBRARY_SOURCE_DIRECTORY ${PROJECT_BINARY_DIR}/src)
set (THINGPLUS_LIBRARY_INCLUDE_DIRECTORY ${PROJECT_BINARY_DIR}/include)

# set global cflags, cxxflags
add_compile_options(-Wall -g)

#configuration test coverage report
add_compile_options(-O0 --coverage -fprofile-arcs -ftest-coverage)
SETUP_TARGET_FOR_COVERAGE(
	mock_coverage
	bin/tests/mock
	coverage
)

add_subdirectory(src)
add_subdirectory(tests)

#TESTING
enable_testing()
add_test(NAME mock COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/mock)
