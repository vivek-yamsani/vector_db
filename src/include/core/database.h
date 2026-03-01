//
// Created by Vivek Yamsani on 28/11/25.
//
#pragma once
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "core/collection.h"
#include "core/utils/util.h"
#include "logger/logger.h"

namespace vector_db
{

class database
{
  using shd_collection_ptr = std::shared_ptr< collection >;

  std::shared_mutex mutex_;
  std::shared_ptr< details::logger_impl > logger_;
  std::unordered_map< std::string, shd_collection_ptr > collections_;

public:
  database();


  status add_vectors( const std::string& collection_name, std::vector< std::pair< id_t, float_vector > > vectors );

  result< std::vector< std::pair< std::string, collection_properties > > > get_all_collections();

  result< collection_properties > get_collection_info( const std::string& collection_name );

  status add_collection( const std::string& collection_name, unsigned int dimension );

  status delete_collection( const std::string& collection_name );

  result< std::vector< score_pair > > get_nearest_k( const std::string& collection_name,
                                                       const float_vector& query,
                                                       unsigned int k );

  status delete_vectors( const std::string& collection_name, const std::vector< id_t >& _ids );

  status add_index( const std::string& collection_name, const std::string& index_name, index_type index_type, params_t* params );

  result< std::pair< index_type, const params_t* >> get_index_params( const std::string& collection_name, const std::string& index_name );
};
}  // namespace vector_db
