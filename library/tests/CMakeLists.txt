add_compile_options(-Wno-write-strings -Wno-format-security)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/tests)
set (MOCK_SRCS 
	${CMAKE_CURRENT_SOURCE_DIR}/fixture/restapi_server.c
	${CMAKE_CURRENT_SOURCE_DIR}/mock_mosquitto.c 
	${CMAKE_CURRENT_SOURCE_DIR}/mock_curl.c 
	${CMAKE_CURRENT_SOURCE_DIR}/mock_expect_thingplus.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/mock_expect_tcurl.cpp)
set (MOCK_INCLUDE_DIRECTORIES 
	${CMAKE_CURRENT_SOURCE_DIR} 
	${CMAKE_CURRENT_SOURCE_DIR}/fixture
	${THINGPLUS_LIBRARY_SOURCE_DIRECTORY}
	${THINGPLUS_LIBRARY_INCLUDE_DIRECTORY})

add_subdirectory(mock)
add_subdirectory(fixture)
