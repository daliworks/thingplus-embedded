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

# set global cflags, cxxflags
add_compile_options(-Wall -g)

add_subdirectory(src)
add_subdirectory(tests)

#TESTING
enable_testing()
add_test(NAME init_mock COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/init_mock)
add_test(NAME connect_mock COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/connect_mock)
add_test(NAME pubsub_mock COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/pubsub_mock)
add_test(NAME values COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/tests/values)
