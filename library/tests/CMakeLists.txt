ADD_COMPILE_OPTIONS (-Wno-write-strings -Wno-format-security)

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/tests)

ADD_SUBDIRECTORY (fixture)
ADD_SUBDIRECTORY (mqtt)
ADD_SUBDIRECTORY(rest)

SET (TEST_SOURCES
	${MQTT_TEST_SOURCES}
	${REST_TEST_SOURCES}
	thingplus.cpp
	test_main.cpp
	)

ADD_EXECUTABLE(test_thingplus 
	${TEST_SOURCES}
	${FIXTURES}
	${THINGPLUS_LIBRARY_SOURCE})

target_link_libraries(test_thingplus
	json-c
	gcov
	CppUTest 
	CppUTestExt)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/fixture
	${THINGPLUS_LIBRARY_SOURCE_DIRECTORY}
	${THINGPLUS_LIBRARY_INCLUDE_DIRECTORY})
