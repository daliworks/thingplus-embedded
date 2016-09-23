set (RESTAPI_SRC
	main.cpp
	testcase.cpp
	../restapi_server.c)

add_executable (restapi_server_test ${RESTAPI_SRC})

target_link_libraries(restapi_server_test 
	CppUTest
	CppUTestExt
	gcov)
include_directories(../)
