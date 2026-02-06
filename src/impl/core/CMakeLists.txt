find_package(spdlog REQUIRED)

add_library(core STATIC
        float_vector.cpp
        indices/ivfflat.cpp
        indices/euclidean.cpp
        indices/hnsw.cpp
        utils/utils.cpp
        collection.cpp
        database.cpp
)
add_library(vector_db::core ALIAS core)

target_link_libraries(
        core
        PUBLIC logger
        PRIVATE configuration
)

target_include_directories(core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:include>
)