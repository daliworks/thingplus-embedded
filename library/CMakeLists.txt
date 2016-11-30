##	for change install dirctory 
#	cmake -DCMAKE_INSTALL_PREFIX:PATH=/h me/younghyun/tmp .
cmake_minimum_required(VERSION 3.0)

#project name is "Thinplus" and version is "1.0.0"
project(Thingplus VERSION 1.0.0)

#executable file ourput directory
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)
#static library output directory
set (CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
#shared library output directory
set (CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

set (THINGPLUS_LIBRARY_SOURCE_DIRECTORY ${PROJECT_BINARY_DIR}/src)
set (THINGPLUS_LIBRARY_INCLUDE_DIRECTORY ${PROJECT_BINARY_DIR}/include)

list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

MESSAGE(STATUS ${CURL_VERSION_STRING})

FIND_PACKAGE(CURL REQUIRED)
FIND_PACKAGE(JSON-C REQUIRED)
FIND_PACKAGE(MOSQUITTO)

# set global cflags, cxxflags
add_compile_options(-Wall -g)
add_subdirectory(src)
add_subdirectory(include)

IF(CMAKE_BUILD_TYPE STREQUAL "Debug") 
	#cmake -DCMAKE_BUILD_TYPE:STRING=Debug
	list (APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
	include (CodeCoverage)

	#configuration test coverage report
	add_compile_options(-O0 --coverage -fprofile-arcs -ftest-coverage)
	SETUP_TARGET_FOR_COVERAGE(coverage bin/tests/test_thingplus coverage)

	add_subdirectory(tests)

	#TESTING
	enable_testing()
	add_test(NAME mock COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/test_thingplus)
ENDIF()
