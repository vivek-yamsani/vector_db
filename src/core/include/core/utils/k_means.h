//
// Created by Vivek Yamsani on 13/12/25.
//
#pragma once
#include <set>
#include "../float_vector.h"

namespace vector_db
{

struct k_means_result
{
  struct centroid_result
  {
    float_vector centroid;
    std::set< id > vector_ids;
    centroid_result() = default;
    explicit centroid_result( const float_vector& _centroid )
        : centroid( _centroid )
    {
    }
    bool operator==( const centroid_result& other ) const { return centroid == other.centroid; }
  };
  std::vector< centroid_result > centroids;
};

inline k_means_result k_means( const std::vector< float_vector >& vectors, unsigned int k )
{
  k_means_result result;
  result.centroids.reserve( k );
  for ( auto i = 0; i < k; ++i )
  {
    result.centroids.emplace_back( vectors[ i ] );
  }
  do
  {
    std::vector< float_vector > new_centroids( k );
  } while ( true );
  return {};
}

}  // namespace vector_db