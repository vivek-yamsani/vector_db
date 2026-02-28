//
// Created by Vivek Yamsani on 09/12/25.
//

#include <algorithm>
#include <queue>

#include "core/collection.h"
#include "core/indices/ivfflat.h"
#include "core/utils/k_means.h"

namespace vector_db::indices::ivf_flat
{

index::index( wk_col_ptr _collection_ptr, const params& _params )
    : index_t( std::move( _collection_ptr ) )
    , params_( _params )
{
}

void index::init() { build(); }

void index::build()
{
  auto col = collection_ptr_.lock();
  if ( !col )
    return;

  std::unique_lock lock( mutex_ );
  clusters_.clear();
  vectors_since_rebuild_ = 0;

  auto all_ids = col->get_all_vector_ids();
  if ( all_ids.empty() )
    return;

  std::vector< float_vector > vectors;
  std::vector< id_t > ids;
  for ( auto id : all_ids )
  {
    auto vec = col->get_vector_by_id( id );
    if ( vec )
    {
      vectors.push_back( std::move( *vec ) );
      ids.push_back( id );
    }
  }

  if ( vectors.empty() )
    return;

  auto km_res = k_means( vectors, params_.k_, params_.dist_type_ );

  clusters_.reserve( km_res.centroids.size() );
  for ( auto& c : km_res.centroids )
  {
    cluster new_cluster;
    new_cluster.centroid = std::move( c.centroid );
    for ( auto idx : c.vector_ids )
    {
      new_cluster.vector_ids.push_back( ids[ idx ] );
    }
    clusters_.push_back( std::move( new_cluster ) );
  }
}

bool index::search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results )
{
  std::shared_lock lock( mutex_ );
  if ( clusters_.empty() )
    return false;

  auto col = collection_ptr_.lock();
  if ( !col )
    return false;

  distance::ptr dist_fn = distance::get_distance_instance( params_.dist_type_ );

  // 1. Find the top n_probe clusters
  using cluster_dist = std::pair< double, size_t >;
  std::priority_queue< cluster_dist, std::vector< cluster_dist >, std::greater<> > pq_clusters;

  for ( size_t i = 0; i < clusters_.size(); ++i )
  {
    double d = dist_fn->compute( query_vector, clusters_[ i ].centroid );
    pq_clusters.push( { d, i } );
  }

  // 2. Search within these clusters
  std::priority_queue< score_pair, std::vector< score_pair >, std::less<> > pq_results;
  unsigned int probes = std::min< unsigned int >( params_.n_probe_, pq_clusters.size() );

  for ( unsigned int i = 0; i < probes; ++i )
  {
    size_t cluster_idx = pq_clusters.top().second;
    pq_clusters.pop();

    for ( auto id : clusters_[ cluster_idx ].vector_ids )
    {
      auto vec = col->get_vector_by_id( id );
      if ( vec )
      {
        double d = dist_fn->compute( query_vector, *vec );
        pq_results.emplace( d, id_vector{ id, std::make_unique< float_vector >( std::move( *vec ) ) } );
        if ( pq_results.size() > k )
          pq_results.pop();
      }
    }
  }

  results.clear();
  while ( !pq_results.empty() )
  {
    results.push_back( std::move( const_cast< score_pair& >( pq_results.top() ) ) );
    pq_results.pop();
  }
  std::reverse( results.begin(), results.end() );

  return !results.empty();
}

void index::on_vectors_added( const std::vector< id_t >& new_ids )
{
  if ( clusters_.empty() )
  {
    build();
    return;
  }

  vectors_since_rebuild_ += new_ids.size();
  if ( vectors_since_rebuild_ >= params_.rebuild_threshold_ )
  {
    build();
  }
  else
  {
    add_vectors_incremental( new_ids );
  }
}

void index::on_vectors_removed( const std::vector< id_t >& removed_ids )
{
  if ( clusters_.empty() )
  {
    return;
  }

  vectors_since_rebuild_ += removed_ids.size();
  if ( vectors_since_rebuild_ >= params_.rebuild_threshold_ )
  {
    build();
  }
  else
  {
    remove_vectors_incremental( removed_ids );
  }
}

size_t index::find_nearest_cluster( const float_vector& vec ) const
{
  distance::ptr dist_fn = distance::get_distance_instance( params_.dist_type_ );
  size_t nearest_idx = 0;
  double min_dist = std::numeric_limits< double >::max();

  for ( size_t i = 0; i < clusters_.size(); ++i )
  {
    double d = dist_fn->compute( vec, clusters_[ i ].centroid );
    if ( d < min_dist )
    {
      min_dist = d;
      nearest_idx = i;
    }
  }

  return nearest_idx;
}

void index::add_vectors_incremental( const std::vector< id_t >& new_ids )
{
  auto col = collection_ptr_.lock();
  if ( !col )
    return;

  std::unique_lock lock( mutex_ );

  for ( auto id : new_ids )
  {
    auto vec = col->get_vector_by_id( id );
    if ( vec )
    {
      size_t cluster_idx = find_nearest_cluster( *vec );
      clusters_[ cluster_idx ].vector_ids.push_back( id );
    }
  }
}

void index::remove_vectors_incremental( const std::vector< id_t >& removed_ids )
{
  std::unique_lock lock( mutex_ );

  for ( auto id : removed_ids )
  {
    for ( auto& cluster : clusters_ )
    {
      auto it = std::find( cluster.vector_ids.begin(), cluster.vector_ids.end(), id );
      if ( it != cluster.vector_ids.end() )
      {
        cluster.vector_ids.erase( it );
        break;
      }
    }
  }
}

}  // namespace vector_db::indices::ivf_flat