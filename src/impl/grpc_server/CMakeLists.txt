cmake_minimum_required(VERSION 3.30)

project(grpc_server)

get_filename_component(db_proto ../../protos/db.proto ABSOLUTE)
get_filename_component(db_proto_path "${db_proto}" PATH)

message("${db_proto} ${db_proto_path}")

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC REQUIRED)

set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)

# Generate protobuf sources
set(PROTO_SRCS "${CMAKE_CURRENT_BINARY_DIR}/db.pb.cc")
set(PROTO_HDRS "${CMAKE_CURRENT_BINARY_DIR}/db.pb.h")
set(GRPC_SRCS "${CMAKE_CURRENT_BINARY_DIR}/db.grpc.pb.cc")
set(GRPC_HDRS "${CMAKE_CURRENT_BINARY_DIR}/db.grpc.pb.h")

add_custom_command(
        OUTPUT ${PROTO_SRCS} ${PROTO_HDRS}
        COMMAND protobuf::protoc
        ARGS --cpp_out=${CMAKE_CURRENT_BINARY_DIR}
        -I${db_proto_path}
        ${db_proto}
        DEPENDS ${db_proto}
        COMMENT "Generating protobuf files"
)

add_custom_command(
        OUTPUT ${GRPC_SRCS} ${GRPC_HDRS}
        COMMAND protobuf::protoc
        ARGS --grpc_out=${CMAKE_CURRENT_BINARY_DIR}
        --plugin=protoc-gen-grpc=${_GRPC_CPP_PLUGIN_EXECUTABLE}
        -I${db_proto_path}
        ${db_proto}
        DEPENDS ${db_proto}
        COMMENT "Generating gRPC files"
)

add_library(grpc_server STATIC
        server.cpp
        ${PROTO_SRCS}
        ${PROTO_HDRS}
        ${GRPC_SRCS}
        ${GRPC_HDRS}
)

target_link_libraries(grpc_server PUBLIC
        vector_db::core
        gRPC::grpc++
        gRPC::grpc++_reflection
        protobuf::libprotobuf
        PRIVATE logger
)

target_include_directories(grpc_server PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/include>
)