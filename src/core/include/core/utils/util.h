//
// Common utility types and helpers for vector_db
//
#pragma once

#include "core/float_vector.h"
#include "core/status.h"

namespace vector_db::utils
{

double get_euclidean_distance( const float_vector& a, const float_vector& b );

double get_cosine_distance( const float_vector& a, const float_vector& b );

status is_collection_name_valid( const std::string& collection_name );

}  // namespace vector_db::utils
