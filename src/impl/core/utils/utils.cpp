//
// Created by Vivek Yamsani on 18/12/25.
//
#include "core/indices/hnsw.h"
#include "core/utils/util.h"

namespace vector_db::utils
{

double get_euclidean_distance( const float_vector& a, const float_vector& b )
{
  double distance = 0.0;
  for ( unsigned int i = 0; i < static_cast< unsigned int >( a.dimension_ ); ++i )
  {
    const double diff = static_cast< double >( a.data_[ i ] ) - static_cast< double >( b.data_[ i ] );
    distance += diff * diff;
  }
  return std::sqrt( distance );
}

double get_cosine_distance( const float_vector& a, const float_vector& b )
{
  double dot_product = 0.0, mag_a = 0.0, mag_b = 0.0;
  for ( unsigned int i = 0; i < static_cast< unsigned int >( a.dimension_ ); ++i )
  {
    const double av = static_cast< double >( a.data_[ i ] );
    const double bv = static_cast< double >( b.data_[ i ] );
    dot_product += av * bv;
    mag_a += av * av;
    mag_b += bv * bv;
  }
  mag_a = std::sqrt( mag_a );
  mag_b = std::sqrt( mag_b );
  if ( mag_a == 0.0 || mag_b == 0.0 )
  {
    return 1.0;
  }
  return 1.0 - ( dot_product / ( mag_a * mag_b ) );
}

status is_collection_name_valid( const std::string& collection_name )
{
  if ( collection_name.empty() )
    return status::collection_name_cant_be_empty;

  if ( collection_name.size() > 128 )
    return status::collection_name_too_long;

  if ( !std::all_of( collection_name.begin(),
                     collection_name.end(),
                     []( const char c ) { return std::isalnum( static_cast< unsigned char >( c ) ) || c == '_'; } ) )
    return status::collection_name_invalid_characters;

  return status::success;
}

}  // namespace vector_db::utils