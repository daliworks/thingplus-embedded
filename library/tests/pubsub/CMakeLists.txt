set (MOCK pubsub_mock)
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
