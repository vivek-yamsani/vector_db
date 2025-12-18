add_library(core STATIC
        impl/float_vector.cpp
        impl/indices/ivfflat.cpp
        impl/indices/euclidean.cpp
        impl/indices/hnsw.cpp
        impl/utils/utils.cpp
        impl/collection.cpp
        impl/database.cpp
)
add_library(vector_db::core ALIAS core)

target_link_libraries(core PUBLIC spdlog::spdlog_header_only)

target_include_directories(core PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)