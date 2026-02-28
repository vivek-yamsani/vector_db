//
// Created by Vivek Yamsani on 13/12/25.
//
#pragma once
#include <algorithm>
#include <limits>
#include <random>

#include "../distance.h"
#include "../float_vector.h"

namespace vector_db
{

struct k_means_result
{
  struct centroid_result
  {
    float_vector centroid;
    std::vector< id_t > vector_ids;
    centroid_result() = default;
    explicit centroid_result( const float_vector& _centroid )
        : centroid( _centroid )
    {
    }
  };
  std::vector< centroid_result > centroids;
};

inline k_means_result k_means( const std::vector< float_vector >& vectors,
                               unsigned int k,
                               distance::dist_type dist_type = distance::dist_type::euclidean,
                               int max_iterations = 100 )
{
  if ( vectors.empty() || k == 0 )
    return {};
  if ( vectors.size() < k )
    k = vectors.size();

  distance::ptr dist_fn = distance::get_distance_instance( dist_type );

  k_means_result result;
  result.centroids.reserve( k );

  // 1. Initial centroids: For simplicity, pick the first k vectors.
  // In a more robust implementation, we might use k-means++ or random selection.
  for ( unsigned int i = 0; i < k; ++i )
  {
    result.centroids.emplace_back( vectors[ i ] );
  }

  bool changed = true;
  int iteration = 0;
  while ( changed && iteration < max_iterations )
  {
    changed = false;
    iteration++;

    // Clear vector assignments
    for ( auto& c : result.centroids )
      c.vector_ids.clear();

    // 2. Assignment step
    for ( size_t i = 0; i < vectors.size(); ++i )
    {
      double min_dist = std::numeric_limits< double >::max();
      int best_centroid = -1;

      for ( unsigned int j = 0; j < k; ++j )
      {
        double d = dist_fn->compute( vectors[ i ], result.centroids[ j ].centroid );
        if ( d < min_dist )
        {
          min_dist = d;
          best_centroid = j;
        }
      }
      if ( best_centroid != -1 )
        result.centroids[ best_centroid ].vector_ids.push_back( i );
    }

    // 3. Update step
    for ( unsigned int j = 0; j < k; ++j )
    {
      if ( result.centroids[ j ].vector_ids.empty() )
        continue;

      int dim = vectors[ 0 ].dimension_;
      std::vector< double > sum( dim, 0.0 );
      for ( auto idx : result.centroids[ j ].vector_ids )
      {
        for ( int d = 0; d < dim; ++d )
        {
          sum[ d ] += vectors[ idx ].data_[ d ];
        }
      }

      float_vector new_centroid;
      new_centroid.dimension_ = dim;
      new_centroid.data_ = std::make_unique< float[] >( dim );
      for ( int d = 0; d < dim; ++d )
      {
        new_centroid.data_[ d ] = static_cast< float >( sum[ d ] / result.centroids[ j ].vector_ids.size() );
      }

      if ( !( new_centroid == result.centroids[ j ].centroid ) )
      {
        result.centroids[ j ].centroid = std::move( new_centroid );
        changed = true;
      }
    }
  }

  return result;
}

}  // namespace vector_db