set (TEST values)
set (TEST_SRC 
	test.cpp 
	main.cpp 
	${THINGPLUS_LIBRARY_SOURCE_DIRECTORY}/mqtt/values.c)
add_executable(${TEST} ${TEST_SRC})
include_directories(${THINGPLUS_LIBRARY_SOURCE_DIRECTORY} ${THINGPLUS_LIBRARY_INCLUDE_DIRECTORY})
target_link_libraries(${TEST}
	CppUTest 
	CppUTestExt)
