cmake_minimum_required(VERSION 3.30)

project(grpc_server)

get_filename_component(db_proto ../protos/db.proto ABSOLUTE)
get_filename_component(db_proto_path "${db_proto}" PATH)

message("${db_proto} ${db_proto_path}")

message("hello: ${CMAKE_CURRENT_BINARY_DIR}, ${CMAKE_BINARY_DIR}")
# Generated sources
set(db_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/db.pb.cc")
set(db_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/db.pb.h")
set(db_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/db.grpc.pb.cc")
set(db_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/db.grpc.pb.h")

set(_PROTOBUF_PROTOC $<TARGET_FILE:protoc>)
set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)

add_custom_command(
        OUTPUT "${db_proto_srcs}" "${db_proto_hdrs}" "${db_grpc_srcs}" "${db_grpc_hdrs}"
        COMMAND ${_PROTOBUF_PROTOC}
        ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
        --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
        -I "${db_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${db_proto}"
        DEPENDS "${db_proto}")

message("output: ${db_proto_srcs} ${db_proto_hdrs} ${db_grpc_srcs} ${db_grpc_hdrs}")

# Include generated *.pb.h files
include_directories("${CMAKE_CURRENT_BINARY_DIR}")

add_library(db_grpc_proto
        ${db_grpc_srcs}
        ${db_grpc_hdrs}
        ${db_proto_srcs}
        ${db_proto_hdrs})
target_link_libraries(db_grpc_proto
        absl::check
        grpc++
        grpc++_reflection
        libprotobuf)

add_library(grpc_server STATIC server.h server.cpp)

target_link_libraries(grpc_server PUBLIC db_grpc_proto vector_db::core spdlog::spdlog_header_only)

target_include_directories(grpc_server INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # Path during build
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/grpc_server> # Path after installation
)