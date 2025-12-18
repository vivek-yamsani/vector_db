#pragma once

#include "core/database.h"
#include "db.grpc.pb.h"

namespace vector_db
{
inline index_type proto_to_db_index( const IndexType& _algo )
{
  switch ( _algo )
  {
    case IndexType::IVF_FLAT:
      return index_type::ivf_flat;
    case IndexType::HNSW:
      return index_type::hnsw;
    default:
      return index_type::unknown;
  }
}

inline distance::dist_type proto_to_db_dist( const DistanceType& _algo )
{
  switch ( _algo )
  {
    case DistanceType::COSINE:
      return distance::dist_type::cosine;
    case DistanceType::EUCLIDEAN:
      return distance::dist_type::euclidean;
    case DistanceType::INNER_PRODUCT:
      return distance::dist_type::inner_product;
    default:
      return distance::dist_type::unknown;
  }
}

inline grpc::Status status_to_grpc_status( const status s )
{
  switch ( s )
  {
    case status::success:
      return grpc::Status::OK;
    case status::collection_already_exists:
      return { grpc::ALREADY_EXISTS, "collection already exists" };
    case status::collection_does_not_exist:
      return { grpc::NOT_FOUND, "collection does not exist" };
    case status::dimension_cant_be_zero:
      return { grpc::INVALID_ARGUMENT, "dimension cant be zero" };
    case status::collection_name_cant_be_empty:
      return { grpc::INVALID_ARGUMENT, "collection name cant be empty" };
    case status::collection_name_too_long:
      return { grpc::INVALID_ARGUMENT, "collection name is too long" };
    case status::collection_name_invalid_characters:
      return { grpc::INVALID_ARGUMENT, "collection name contains invalid characters" };
    case status::vector_dimension_mismatch:
      return { grpc::INVALID_ARGUMENT, "vector dimension mismatch with collection" };
    default:
      break;
  }
  return { grpc::INTERNAL, "internal error" };
}
}  // namespace vector_db