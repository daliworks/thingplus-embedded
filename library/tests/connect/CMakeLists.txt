### connect_mock
set (MOCK connect_mock)
set (MOCK_SRC 
	test_mock.cpp 
	main.cpp 
	${MOCK_SRCS} 
	${THINGPLUS_LIBRARY_SOURCE})
add_executable(${MOCK} ${MOCK_SRC})
target_link_libraries(${MOCK}
	json-c
	curl
	CppUTest 
	CppUTestExt)
include_directories(${MOCK_INCLUDE_DIRECTORIES})

### connect
set (TEST connect)
set (TEST_SRC test.cpp main.cpp)
add_executable(${TEST} ${TEST_SRC})
target_link_libraries(${TEST}
	thingplus
	CppUTest 
	CppUTestExt)
