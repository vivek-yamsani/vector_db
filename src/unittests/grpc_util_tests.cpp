//
// Unit tests for gRPC utility functions
//

#include <gtest/gtest.h>

#include "core/distance.h"
#include "db.grpc.pb.h"
#include "grpc_server/util.h"

using namespace vector_db;

TEST( GrpcUtilTests, ProtoToDbDistConversion )
{
  // Test proto to db distance type conversions
  EXPECT_EQ( proto_to_db_dist( DistanceType::COSINE ), distance::dist_type::cosine );
  EXPECT_EQ( proto_to_db_dist( DistanceType::EUCLIDEAN ), distance::dist_type::euclidean );
  EXPECT_EQ( proto_to_db_dist( DistanceType::INNER_PRODUCT ), distance::dist_type::inner_product );
}

TEST( GrpcUtilTests, DbDistToProtoConversion )
{
  // Test db to proto distance type conversions
  EXPECT_EQ( db_dist_to_proto( distance::dist_type::cosine ), DistanceType::COSINE );
  EXPECT_EQ( db_dist_to_proto( distance::dist_type::euclidean ), DistanceType::EUCLIDEAN );
  EXPECT_EQ( db_dist_to_proto( distance::dist_type::inner_product ), DistanceType::INNER_PRODUCT );
}

TEST( GrpcUtilTests, DbDistToProtoUnknownDefault )
{
  // Test that unknown distance type returns a default (EUCLIDEAN)
  EXPECT_EQ( db_dist_to_proto( distance::dist_type::unknown ), DistanceType::EUCLIDEAN );
}

TEST( GrpcUtilTests, RoundTripConversionProtoToDb )
{
  // Test round-trip conversion: proto -> db -> proto
  EXPECT_EQ( db_dist_to_proto( proto_to_db_dist( DistanceType::COSINE ) ), DistanceType::COSINE );
  EXPECT_EQ( db_dist_to_proto( proto_to_db_dist( DistanceType::EUCLIDEAN ) ), DistanceType::EUCLIDEAN );
  EXPECT_EQ( db_dist_to_proto( proto_to_db_dist( DistanceType::INNER_PRODUCT ) ), DistanceType::INNER_PRODUCT );
}

TEST( GrpcUtilTests, RoundTripConversionDbToProto )
{
  // Test round-trip conversion: db -> proto -> db
  EXPECT_EQ( proto_to_db_dist( db_dist_to_proto( distance::dist_type::cosine ) ), distance::dist_type::cosine );
  EXPECT_EQ( proto_to_db_dist( db_dist_to_proto( distance::dist_type::euclidean ) ), distance::dist_type::euclidean );
  EXPECT_EQ( proto_to_db_dist( db_dist_to_proto( distance::dist_type::inner_product ) ),
             distance::dist_type::inner_product );
}

TEST( GrpcUtilTests, StatusToGrpcStatusConversion )
{
  // Test status to grpc status conversions
  EXPECT_EQ( status_to_grpc_status( status::success ).error_code(), grpc::StatusCode::OK );
  EXPECT_EQ( status_to_grpc_status( status::collection_already_exists ).error_code(), grpc::StatusCode::ALREADY_EXISTS );
  EXPECT_EQ( status_to_grpc_status( status::collection_does_not_exist ).error_code(), grpc::StatusCode::NOT_FOUND );
  EXPECT_EQ( status_to_grpc_status( status::dimension_cant_be_zero ).error_code(), grpc::StatusCode::INVALID_ARGUMENT );
  EXPECT_EQ( status_to_grpc_status( status::collection_name_cant_be_empty ).error_code(),
             grpc::StatusCode::INVALID_ARGUMENT );
  EXPECT_EQ( status_to_grpc_status( status::collection_name_too_long ).error_code(), grpc::StatusCode::INVALID_ARGUMENT );
  EXPECT_EQ( status_to_grpc_status( status::collection_name_invalid_characters ).error_code(),
             grpc::StatusCode::INVALID_ARGUMENT );
  EXPECT_EQ( status_to_grpc_status( status::vector_dimension_mismatch ).error_code(),
             grpc::StatusCode::INVALID_ARGUMENT );
  EXPECT_EQ( status_to_grpc_status( status::index_does_not_exist ).error_code(), grpc::StatusCode::NOT_FOUND );
  EXPECT_EQ( status_to_grpc_status( status::internal_error ).error_code(), grpc::StatusCode::INTERNAL );
}

TEST( GrpcUtilTests, ProtoToDbIndexConversion )
{
  // Test proto to db index type conversions
  EXPECT_EQ( proto_to_db_index( IndexType::HNSW ), index_type::hnsw );
  EXPECT_EQ( proto_to_db_index( IndexType::IVF_FLAT ), index_type::ivf_flat );
}
