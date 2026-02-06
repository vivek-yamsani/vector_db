project(logger)

find_package(spdlog REQUIRED)
find_package(fmt REQUIRED)

add_library(logger STATIC logger.cpp)

target_link_libraries(logger
        PUBLIC fmt::fmt
        PRIVATE configuration spdlog::spdlog_header_only)

target_include_directories(logger PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)