######## CPPUTEST START ########
SET (CPPUTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/fixture/cpputest)
SET (CPPUTEST_LIB ${CPPUTEST_DIR}/lib/libCppUTest.a ${CPPUTEST_DIR}/lib/libCppUTestExt.a)
SET (CPPUTEST_INCDIR ${CPPUTEST_DIR}/include)

include(ExternalProject)
ExternalProject_Add(cpputest 
	PREFIX ${CPPUTEST_DIR}
	GIT_REPOSITORY https://github.com/cpputest/cpputest.git
	GIT_TAG v3.8
	CMAKE_ARGS -DCMAKE_INSTALL_PREFIX:PATH=${CPPUTEST_DIR}
)
######## CPPUTEST END ########3

ADD_COMPILE_OPTIONS (-Wno-write-strings -Wno-format-security)

SET (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin/tests)

ADD_SUBDIRECTORY (fixture)
ADD_SUBDIRECTORY (mqtt)
ADD_SUBDIRECTORY (rest)

SET (TEST_SOURCES
	${MQTT_TEST_SOURCES}
	${REST_TEST_SOURCES}
	thingplus.cpp
	test_main.cpp)

ADD_EXECUTABLE(test_thingplus 
	${TEST_SOURCES}
	${FIXTURES}
	${THINGPLUS_LIBRARY_SOURCE})

target_link_libraries(test_thingplus
	json-c
	gcov
	${CPPUTEST_LIB}
	)

include_directories(
	${CMAKE_CURRENT_SOURCE_DIR}/fixture
	${CPPUTEST_INCDIR}
	${THINGPLUS_LIBRARY_SOURCE_DIRECTORY}
	${THINGPLUS_LIBRARY_INCLUDE_DIRECTORY})
