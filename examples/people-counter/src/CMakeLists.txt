SET (SRCS 
	apc100.c 
	main.c
	gateway.c
)

FOREACH (SRC ${SRCS} )
	SET (PEOPLE_COUNTER_SRC ${PEOPLE_COUNTER_SRC} ${CMAKE_CURRENT_BINARY_DIR}/${SRC})
ENDFOREACH(SRC)
SET (PEOPLE_COUNTER_SRC ${PEOPLE_COUNTER_SRC} PARENT_SCOPE)

SET (PEOPLE_COUNTER_LIB 
	json-c
	mosquitto
	curl
	rt
	pthread 
	libthingplus.a
	PARENT_SCOPE)

ADD_EXECUTABLE(people-counter ${SRCS})
TARGET_LINK_LIBRARIES(people-counter 
	libthingplus.a
	json-c
	mosquitto
	curl
	rt
	pthread 
	)


