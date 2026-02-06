cmake_minimum_required(VERSION 3.30)

project(unittests)

find_package(toml11 REQUIRED)

find_package(gtest REQUIRED)

enable_testing()

add_executable(run_tests main.cpp configuration_tests.cpp)

target_link_libraries(run_tests PUBLIC gtest::gtest gtest_main vector_db::core grpc_server configuration toml11::toml11)

include(GoogleTest)
gtest_discover_tests(run_tests)