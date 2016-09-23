file (GLOB_RECURSE THINGPLUS_LIBRARY_SOURCE *.c)
set (THINGPLUS_LIBRARY_SOURCE ${THINGPLUS_LIBRARY_SOURCE} PARENT_SCOPE)

add_library(thingplus SHARED ${THINGPLUS_LIBRARY_SOURCE})
target_link_libraries(thingplus json-c curl mosquitto)

target_include_directories(thingplus PUBLIC ${PROJECT_BINARY_DIR}/include)

#install(TARGETS thingplus LIBRARY DESTINATION lib)
