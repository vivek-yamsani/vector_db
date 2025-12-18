CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

PROJECT(vector_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_ASAN "Use Address Sanitizer" OFF)
if (USE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address)
endif ()

enable_testing()

set(ASIO_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/asio/include")

add_subdirectory(third-party)
add_subdirectory(src/core)
add_subdirectory(src/grpc_server)
add_subdirectory(src/unittests)

add_executable(vector_db src/main.cpp)

target_link_libraries(vector_db PUBLIC grpc_server)