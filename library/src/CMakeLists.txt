file (GLOB_RECURSE THINGPLUS_LIBRARY_SOURCE *.c)
set (THINGPLUS_LIBRARY_SOURCE ${THINGPLUS_LIBRARY_SOURCE} PARENT_SCOPE)

#shared library
add_library(thingplus SHARED ${THINGPLUS_LIBRARY_SOURCE})
target_link_libraries(thingplus json-c curl mosquitto)
target_include_directories(thingplus PUBLIC ${PROJECT_BINARY_DIR}/include)

#static library
add_library(thingplus_static STATIC ${THINGPLUS_LIBRARY_SOURCE})
target_link_libraries(thingplus_static json-c curl mosquitto)
set_target_properties(thingplus_static PROPERTIES OUTPUT_NAME thingplus)
target_include_directories(thingplus_static PUBLIC ${PROJECT_BINARY_DIR}/include)


install(TARGETS thingplus LIBRARY DESTINATION lib)
install(TARGETS thingplus_static ARCHIVE DESTINATION lib)
