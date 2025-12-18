cmake_minimum_required(VERSION 3.30)

project(unittests)

enable_testing()

add_executable(run_tests main.cpp hnsw_test.cpp)

target_link_libraries(run_tests gtest_main vector_db::core grpc_server)

include(GoogleTest)
gtest_discover_tests(run_tests)