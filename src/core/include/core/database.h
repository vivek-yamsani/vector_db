//
// Created by Vivek Yamsani on 28/11/25.
//
#pragma once
#include <memory>
#include <shared_mutex>
#include <unordered_map>

#include "core/collection.h"
#include "core/log.h"
#include "core/utils/util.h"

namespace vector_db
{

class database
{
  std::shared_mutex mutex_;
  std::shared_ptr< spdlog::logger > logger_;

  using shd_collection_ptr = std::shared_ptr< collection >;

public:
  database();

  std::unordered_map< std::string, shd_collection_ptr > collections_;

  status add_vectors( const std::string& collection_name, std::vector< std::pair< id_t, float_vector > > vectors );

  status get_all_collections( std::vector< std::pair< std::string, collection_properties > >& collections );

  status get_collection_info( const std::string& collection_name, collection_properties& collection_data );

  status add_collection( const std::string& collection_name, unsigned int dimension );

  status delete_collection( const std::string& collection_name );

  status get_nearest_k( const std::string& collection_name,
                        const float_vector& query,
                        unsigned int k,
                        std::vector< score_pair >& result );

  status delete_vectors( const std::string& collection_name, const std::vector< id_t >& _ids );

  status add_index( const std::string& collection_name, const std::string& index_name, index_type index_type, params_t* params );
};
}  // namespace vector_db
