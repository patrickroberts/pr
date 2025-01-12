add_executable(any_iterator any_iterator.cpp)
target_link_libraries(any_iterator PRIVATE patrickroberts)
add_test(NAME test_any_iterator COMMAND any_iterator)

add_executable(any_view any_view.cpp)
target_link_libraries(any_view PRIVATE patrickroberts)
add_test(NAME test_any_view COMMAND any_view)
