### test init_mock
set (INIT_MOCK_SRC 
	test_mock.cpp 
	main.cpp 
	${MOCK_SRCS} 
	${THINGPLUS_LIBRARY_SOURCE})
add_executable (init_mock ${INIT_MOCK_SRC})
target_link_libraries(init_mock
	json-c
	curl
	CppUTest 
	CppUTestExt)
include_directories(${MOCK_INCLUDE_DIRECTORIES})
