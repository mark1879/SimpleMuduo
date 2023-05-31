include_directories(${CMAKE_SOURCE_DIR})

add_executable(test_simple_muduo test_simple_muduo.cc)

target_link_libraries(test_simple_muduo ${PROJECT_BINARY_DIR}/libsimple_muduo.a pthread)
