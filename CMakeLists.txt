CMAKE_MINIMUM_REQUIRED(VERSION 3.10)

set(CMAKE_PROJECT_TOP_LEVEL_INCLUDES "conan_provider.cmake")

PROJECT(vector_db LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(USE_ASAN "Use Address Sanitizer" OFF)
option(WITH_TESTS "Build Tests" OFF)

if (USE_ASAN)
  add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
  add_link_options(-fsanitize=address)
endif ()

if (WITH_TESTS)
  enable_testing()
endif ()

add_subdirectory(src)

add_executable(vector_db src/main.cpp)

target_link_libraries(
        vector_db
        PUBLIC grpc_server
        PRIVATE logger configuration
)