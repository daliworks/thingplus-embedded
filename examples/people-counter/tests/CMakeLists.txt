SET (SRCS main.cpp
	apc100.cpp
	../src/apc100.c
)

ADD_COMPILE_OPTIONS(-Wall -g -Wno-write-strings)

ADD_EXECUTABLE(unittest ${SRCS})

TARGET_LINK_LIBRARIES(unittest
	${PEOPLE_COUNTER_LIB}
	libCppUTest.a
	libCppUTestExt.a
)

