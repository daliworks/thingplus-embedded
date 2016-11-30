SET (SRCS main.cpp
	apc100.cpp
	people-counter.cpp
	gateway.cpp
	../src/apc100.c 
	../src/people-counter.c
)

ADD_COMPILE_OPTIONS(-Wall -g -Wno-write-strings -fpermissive)

ADD_EXECUTABLE(unittest ${SRCS})

TARGET_LINK_LIBRARIES(unittest
	${PEOPLE_COUNTER_LIB}
	libCppUTest.a
	libCppUTestExt.a
)


#ADD_EXECUTABLE(unittest-fff main.cpp gateway-fff.cpp)
#TARGET_LINK_LIBRARIES(unittest-fff
#	pthread
#	libCppUTest.a
#	libCppUTestExt.a
#)
