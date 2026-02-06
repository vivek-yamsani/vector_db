//
// Created by Vivek Yamsani on 18/12/25.
//
#include "core/indices/euclidean.h"
#include "core/collection.h"
#include "core/utils/util.h"

namespace vector_db::indices::euclidean
{
index::index( const wk_col_ptr& col_ptr )
    : index_t( col_ptr )
{
}

bool index::search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results )
{
  try
  {
    const auto col = collection_ptr_.lock();
    if ( !col )
      throw std::runtime_error( "Collection pointer expired during search" );
    const auto& vectors_ = col->get_vectors();
    results.clear();
    if ( k > vectors_.size() )
    {
      k = static_cast< unsigned int >( vectors_.size() );
    }
    std::vector< std::pair< double, id_t > > dist_vec;
    dist_vec.reserve( vectors_.size() );
    for ( const auto& [ id, vector ] : vectors_ )
    {
      const auto dist_func = distance::euclidean::get_instance();
      dist_vec.emplace_back( dist_func->compute( query_vector, *vector.get() ), id );
    }

    std::partial_sort( dist_vec.begin(), dist_vec.begin() + k, dist_vec.end() );

    results.resize( k );
    for ( size_t i = 0; i < k; ++i )
    {
      auto& [ dist, _id ] = dist_vec[ i ];
      results[ i ].first = dist;
      results[ i ].second = { _id, std::make_unique< float_vector >( *vectors_.at( _id ).get() ) };
    }
  }
  catch ( const std::exception& e )
  {
    logger_->error( "Failed to search for top k: {}", e.what() );
    return false;
  }
  catch ( ... )
  {
    logger_->error( "Failed to search for top k due to an unknown exception" );
    return false;
  }

  return true;
}

}  // namespace vector_db::indices::euclidean