project(configuration)

find_package(toml11 REQUIRED)

add_library(configuration provider.cpp)

target_link_libraries(configuration PRIVATE toml11::toml11)

target_include_directories(configuration PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)