//
// Implementation for vector_db::collection
//
#include "core/collection.h"
#include "core/indices/euclidean.h"
#include "core/indices/hnsw.h"
#include "core/utils/util.h"

#include <algorithm>
#include <iostream>

namespace vector_db
{
collection::collection( const unsigned int dimension, const std::string& name )
    : collection_properties( dimension, name )
{
}

std::pair< int, int > collection::add_vectors( std::vector< std::pair< id_t, float_vector > > vectors )
{
  std::unique_lock< std::shared_mutex > lock( vec_mutex_ );
  int added = 0, updated = 0;
  std::vector< id_t > _new_ids;
  for ( auto& [ id, _vector ] : vectors )
  {
    _new_ids.push_back( id );
    if ( auto [ _, _added ] = vectors_.emplace( id, std::make_unique< float_vector >( std::move( _vector ) ) ); _added )
    {
      added++;
    }
    else
    {
      *vectors_[ id ] = _vector;
      updated++;
    }
  }

  for ( auto& [ _, index ] : indices_ )
  {
    index->on_vectors_added( _new_ids );
  }

  return { added, updated };
}

const float_vector* collection::get_vector_by_id( id_t _id ) const
{
  auto it = vectors_.find( _id );
  if ( it == vectors_.end() )
    return nullptr;
  return it->second.get();
}

int collection::remove_vectors( const std::vector< id_t >& ids )
{
  std::unique_lock< std::shared_mutex > lock( vec_mutex_ );
  std::vector< id_t > _removed_ids;
  for ( auto _id : ids )
  {
    auto it = vectors_.find( _id );
    if ( it != vectors_.end() )
    {
      vectors_.erase( it );
      _removed_ids.push_back( _id );
    }
  }
  if ( !_removed_ids.empty() )
  {
    for ( auto& [ _, _index ] : indices_ )
    {
      _index->on_vectors_removed( _removed_ids );
    }
  }
  return _removed_ids.size();
}

bool collection::add_index( const std::string& name, index_type _index_type, params_t* params )
{
  if ( _index_type == index_type::unknown )
    return false;
  try
  {
    std::shared_lock< std::shared_mutex > lock( idx_mutex_ );

    switch ( _index_type )
    {
      case index_type::hnsw:
      {
        auto hnsw_params = static_cast< indices::hnsw::params* >( params );
        auto _index = std::make_unique< indices::hnsw::index >( weak_from_this(), *hnsw_params );
        _index->init();
        indices_.emplace( name, std::move( _index ) );
        return true;
      }
      default:
        return false;
    }
  }
  catch ( ... )
  {
    return false;
  }
}

bool collection::search_for_top_k( const float_vector& query_vector,
                                   const unsigned int k,
                                   std::vector< score_pair >& results,
                                   const std::string& index_name )
{
  std::shared_lock< std::shared_mutex > lock( idx_mutex_ );
  if ( auto it = indices_.find( index_name ); it == indices_.end() )
  {
    indices::euclidean::index _idx( weak_from_this() );
    _idx.search_for_top_k( query_vector, k, results );
  }
  else
  {
    return it->second->search_for_top_k( query_vector, k, results );
  }
  return false;
}

}  // namespace vector_db
