cmake_minimum_required(VERSION 3.30)

set(protobuf_INSTALL OFF CACHE BOOL "" FORCE)
set(gRPC_INSTALL OFF CACHE BOOL "" FORCE)
set(utf8_range_ENABLE_INSTALL OFF)

add_subdirectory(grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)
add_subdirectory(gtest)
add_subdirectory(spdlog)
