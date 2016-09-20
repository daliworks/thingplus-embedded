### test mock
file (GLOB TEST_SRCS *.cpp)
set (MOCK_SRC 
	${TEST_SRCS}
	${MOCK_SRCS} 
	${THINGPLUS_LIBRARY_SOURCE})
add_executable (mock ${MOCK_SRC})
target_link_libraries(mock
	json-c
	gcov
	CppUTest 
	CppUTestExt)
include_directories(${MOCK_INCLUDE_DIRECTORIES})
