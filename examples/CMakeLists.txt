add_executable(deserialize deserialize.cpp)
target_link_libraries(deserialize PRIVATE patrickroberts)

add_executable(serialize serialize.cpp)
target_link_libraries(serialize PRIVATE patrickroberts)
