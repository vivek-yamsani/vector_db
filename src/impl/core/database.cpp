//
// Implementation for vector_db::database
//
#include "core/database.h"

#include "configuration/provider.h"
#include "logger/logger.h"


namespace vector_db
{

using utils::is_collection_name_valid;

database::database()
{
  auto _log_level = config_provider::get_instance()->get_string( "logger", "log_level" ).value_or( "info" );
  logger_ = logger_factory::create( "core" );
  logger_->set_level( _log_level );
}

status database::add_vectors( const std::string& collection_name, std::vector< std::pair< id_t, float_vector > > vectors )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return _status;
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return status::collection_does_not_exist;
  for ( auto& [ id, vector ] : vectors )
  {
    if ( vector.dimension_ != it->second->dimension_ )
      return status::vector_dimension_mismatch;
  }
  auto [ added, updated ] = it->second->add_vectors( std::move( vectors ) );
  if ( logger_ )
    logger_->info( "Added {}, Updated {} vectors in collection: {}", added, updated, collection_name );
  return status::success;
}

result< std::vector< std::pair< std::string, collection_properties > > > database::get_all_collections()
{
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  std::vector< std::pair< std::string, collection_properties > > collections;
  collections.reserve( collections_.size() );
  for ( auto& [ id, collection ] : collections_ )
  {
    collection_properties data( collection->dimension_, collection->name_ );
    collections.emplace_back( id, data );
  }
  return { status::success, std::move( collections ) };
}

result< collection_properties > database::get_collection_info( const std::string& collection_name )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return { _status };
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return { status::collection_does_not_exist };
  collection_properties collection_data( it->second->dimension_, it->second->name_ );
  return { status::success, std::move( collection_data ) };
}

status database::add_collection( const std::string& collection_name, unsigned int dimension )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return _status;
  std::unique_lock< std::shared_mutex > lock( mutex_ );
  if ( const auto it = collections_.find( collection_name ); it != collections_.end() )
    return status::collection_already_exists;
  collections_.emplace( collection_name, std::make_shared< collection >( dimension, collection_name ) );
  return status::success;
}

status database::delete_collection( const std::string& collection_name )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return _status;
  std::unique_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return status::collection_does_not_exist;
  collections_.erase( it );
  return status::success;
}

result< std::vector< score_pair > > database::get_nearest_k( const std::string& collection_name,
                                                             const float_vector& query,
                                                             const unsigned int k )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return { _status };
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return { status::collection_does_not_exist };
  std::vector< score_pair > search_result;
  it->second->search_for_top_k( query, k, search_result );
  return { status::success, std::move( search_result ) };
}

status database::delete_vectors( const std::string& collection_name, const std::vector< id_t >& _ids )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return _status;
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return status::collection_does_not_exist;
  it->second->remove_vectors( _ids );

  return status::success;
}

status database::add_index( const std::string& collection_name,
                            const std::string& index_name,
                            index_type index_type,
                            params_t* params )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return _status;
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return status::collection_does_not_exist;
  it->second->add_index( index_name, index_type, params );

  return status::success;
}

result< std::pair< index_type, const params_t* > > database::get_index_params( const std::string& collection_name,
                                                                               const std::string& index_name )
{
  if ( const auto _status = is_collection_name_valid( collection_name ); _status != status::success )
    return result< std::pair< index_type, const params_t* > >( _status );
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto it = collections_.find( collection_name );
  if ( it == collections_.end() )
    return result< std::pair< index_type, const params_t* > >( status::collection_does_not_exist );

  auto params_pair = it->second->get_index_params( index_name );
  return params_pair.second ? result( status::success, params_pair )
                            : result< std::pair< index_type, const params_t* > >( status::index_does_not_exist );
}

}  // namespace vector_db
