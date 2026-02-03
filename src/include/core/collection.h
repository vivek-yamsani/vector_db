//
// Collection definitions for vector_db
//
#pragma once

#include <iostream>
#include <shared_mutex>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/float_vector.h"
#include "core/indices/hnsw.h"
#include "core/indices/index.h"
#include "core/utils/splitmix_hash.h"

namespace vector_db
{

struct collection_properties
{
  unsigned int dimension_{ 0 };
  std::string name_{};
  collection_properties() = default;
  explicit collection_properties( const unsigned int dimension, std::string name )
      : dimension_( dimension )
      , name_( std::move( name ) )
  {
  }
};

class collection
    : public collection_properties
    , public std::enable_shared_from_this< collection >
{
  std::shared_mutex vec_mutex_, idx_mutex_;
  std::unordered_map< id_t, vector_ptr, hash > vectors_;
  std::unordered_map< std::string, index_ptr > indices_;

public:
  explicit collection( unsigned int dimension, const std::string& name );

  std::pair< int, int > add_vectors( std::vector< std::pair< id_t, float_vector > > vectors );

  // Remove vectors by id; returns count removed
  int remove_vectors( const std::vector< id_t >& ids );

  const std::unordered_map< id_t, vector_ptr, hash >& get_vectors() const { return vectors_; }

  bool search_for_top_k( const float_vector& query_vector,
                         unsigned int k,
                         std::vector< score_pair >& results,
                         const std::string& index_name = "" );

  bool add_index( const std::string& name, index_type, params_t* params );

  // Accessor for index to fetch data stored in collection without copying
  const float_vector* get_vector_by_id( id_t _id ) const;
};

}  // namespace vector_db
