//
// Implementation for vector_db::collection
//
#include <algorithm>
#include <iostream>

#include "core/collection.h"
#include "core/indices/euclidean.h"
#include "core/indices/ivfflat.h"
#include "core/utils/util.h"

namespace vector_db
{
collection::collection( const unsigned int dimension, const std::string& name )
    : collection_properties( dimension, name )
{
  logger_ = logger_factory::create( "collection" );
}

std::pair< int, int > collection::add_vectors( std::vector< std::pair< id_t, float_vector > > vectors )
{
  int added = 0, updated = 0;
  std::vector< id_t > _new_ids;
  {
    std::unique_lock< std::shared_mutex > lock( vec_mutex_ );
    for ( auto& [ id, _vector ] : vectors )
    {
      _new_ids.push_back( id );
      auto it = vectors_.find( id );
      if ( it != vectors_.end() )
      {
        *( it->second ) = std::move( _vector );
        updated++;
      }
      else
      {
        vectors_.emplace( id, std::make_unique< float_vector >( std::move( _vector ) ) );
        added++;
      }
    }
  }

  {
    std::shared_lock lock( idx_mutex_ );
    for ( auto& [ _, index ] : indices_ )
    {
      index->on_vectors_added( _new_ids );
    }
  }

  return { added, updated };
}

std::optional< float_vector > collection::get_vector_by_id( id_t _id ) const
{
  std::shared_lock lock( vec_mutex_ );
  const auto it = vectors_.find( _id );
  if ( it == vectors_.end() )
    return std::nullopt;
  return *it->second;
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
    std::shared_lock lock2( idx_mutex_ );
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
    std::unique_lock< std::shared_mutex > lock( idx_mutex_ );

    switch ( _index_type )
    {
      case index_type::hnsw:
      {
        auto hnsw_params = dynamic_cast< indices::hnsw::params* >( params );
        if ( !hnsw_params )
        {
          logger_->error( "Invalid params type for HNSW index" );
          return false;
        }
        auto _index = std::make_unique< indices::hnsw::index >( weak_from_this(), *hnsw_params );
        _index->init();
        indices_.emplace( name, std::move( _index ) );
        return true;
      }
      case index_type::ivf_flat:
      {
        auto ivf_params = dynamic_cast< indices::ivf_flat::params* >( params );
        if ( !ivf_params )
        {
          logger_->error( "Invalid params type for HNSW index" );
          return false;
        }
        auto _index = std::make_unique< indices::ivf_flat::index >( weak_from_this(), *ivf_params );
        _index->init();
        indices_.emplace( name, std::move( _index ) );
        return true;
      }
      default:
        return false;
    }
  }
  catch ( const std::exception& e )
  {
    logger_->error( "Failed to add index: {}", e.what() );
    return false;
  }
  catch ( ... )
  {
    logger_->error( "Failed to add index due to an unknown exception" );
    return false;
  }
}

std::unordered_set< id_t, hash > collection::get_all_vector_ids() const
{
  std::shared_lock lock( vec_mutex_ );
  std::unordered_set< id_t, hash > _ids;
  for ( auto& [ id, vector ] : vectors_ )
  {
    _ids.insert( id );
  }
  return _ids;
}

bool collection::search_for_top_k( const float_vector& query_vector,
                                   const unsigned int k,
                                   std::vector< score_pair >& results,
                                   const std::string& index_name )
{
  std::shared_lock< std::shared_mutex > lock( idx_mutex_ );
  std::shared_lock< std::shared_mutex > lock2( vec_mutex_ );
  const auto it = indices_.find( index_name );
  if ( it == indices_.end() )
  {
    indices::euclidean::index _idx( weak_from_this() );
    return _idx.search_for_top_k( query_vector, k, results );
  }

  return it->second->search_for_top_k( query_vector, k, results );
}

std::pair< index_type, const params_t* > collection::get_index_params( const std::string& index_name ) const
{
  std::shared_lock< std::shared_mutex > lock( idx_mutex_ );
  const auto it = indices_.find( index_name );
  if ( it == indices_.end() )
    return { index_type::unknown, nullptr };
  return { it->second->get_index_type(), it->second->get_params() };
}

void collection::serialize( std::ostream& os ) const
{
  std::shared_lock vec_lock( vec_mutex_ );
  std::shared_lock idx_lock( idx_mutex_ );

  const auto name_len = static_cast< uint32_t >( name_.length() );
  os.write( reinterpret_cast< const char* >( &name_len ), sizeof( name_len ) );
  os.write( name_.data(), name_len );

  os.write( reinterpret_cast< const char* >( &dimension_ ), sizeof( dimension_ ) );

  auto vec_count = static_cast< uint32_t >( vectors_.size() );
  os.write( reinterpret_cast< const char* >( &vec_count ), sizeof( vec_count ) );
  for ( const auto& [ id, vec_ptr ] : vectors_ )
  {
    os.write( reinterpret_cast< const char* >( &id ), sizeof( id ) );
    vec_ptr->serialize( os );
  }

  auto idx_count = static_cast< uint32_t >( indices_.size() );
  os.write( reinterpret_cast< const char* >( &idx_count ), sizeof( idx_count ) );
  for ( const auto& [ idx_name, index ] : indices_ )
  {
    index_type type = index->get_index_type();
    if ( type == index_type::unknown )
      continue;
    auto idx_name_len = static_cast< uint32_t >( idx_name.length() );
    os.write( reinterpret_cast< const char* >( &idx_name_len ), sizeof( idx_name_len ) );
    os.write( idx_name.data(), idx_name_len );

    os.write( reinterpret_cast< const char* >( &type ), sizeof( type ) );
    index->serialize( os );
  }
}

std::shared_ptr< collection > collection::deserialize( std::istream& is )
{
  uint32_t name_len;
  is.read( reinterpret_cast< char* >( &name_len ), sizeof( name_len ) );
  std::string name( name_len, '\0' );
  is.read( name.data(), name_len );

  unsigned int dimension;
  is.read( reinterpret_cast< char* >( &dimension ), sizeof( dimension ) );

  auto col = std::make_shared< collection >( dimension, name );

  uint32_t vec_count;
  is.read( reinterpret_cast< char* >( &vec_count ), sizeof( vec_count ) );
  for ( uint32_t i = 0; i < vec_count; ++i )
  {
    id_t id;
    is.read( reinterpret_cast< char* >( &id ), sizeof( id ) );
    col->vectors_.emplace( id, std::make_unique< float_vector >( float_vector::deserialize( is ) ) );
  }

  uint32_t idx_count;
  is.read( reinterpret_cast< char* >( &idx_count ), sizeof( idx_count ) );
  for ( uint32_t i = 0; i < idx_count; ++i )
  {
    uint32_t idx_name_len;
    is.read( reinterpret_cast< char* >( &idx_name_len ), sizeof( idx_name_len ) );
    std::string idx_name( idx_name_len, '\0' );
    is.read( idx_name.data(), idx_name_len );

    auto index = index_t::deserialize( is, col );
    if ( index )
    {
      index->init();
      col->indices_[ idx_name ] = std::move( index );
    }
  }
  return col;
}
}  // namespace vector_db
