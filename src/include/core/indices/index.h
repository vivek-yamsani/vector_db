//
// Created by Vivek Yamsani on 09/12/25.
//
#pragma once
#include <cstdint>
#include <memory>
#include <vector>
#include "core/float_vector.h"
#include "logger/logger.h"

namespace vector_db
{
class collection;

// Distance/index selection across the project
enum class index_type : std::uint8_t
{
  ivf_flat = 0,
  hnsw = 1,
  unknown = 255
};

struct params_t
{
};

class index_t
{
public:
  using wk_col_ptr = std::weak_ptr< collection >;

protected:
  wk_col_ptr collection_ptr_;
  bool dirty_{ false };
  std::shared_ptr< details::logger_impl > logger_;

public:
  index_t() { logger_ = logger_factory::create( "index" ); }
  explicit index_t( wk_col_ptr&& _collection_ptr )
      : collection_ptr_( std::move( _collection_ptr ) )
  {
    logger_ = logger_factory::create( "index" );
  }
  explicit index_t( const std::weak_ptr< collection >& _collection_ptr )
      : collection_ptr_( _collection_ptr )
  {
    logger_ = logger_factory::create( "index" );
  }

  virtual ~index_t() = default;

  virtual void init() {}
  virtual bool search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results ) = 0;
  virtual index_type get_index_type() const { return index_type::unknown; }

  // Incremental update hooks (default no-op)
  virtual void on_vectors_added( const std::vector< id_t >& /*new_ids*/ ) {}
  virtual void on_vectors_removed( const std::vector< id_t >& /*removed_ids*/ ) {}
};

typedef std::unique_ptr< index_t > index_ptr;
}  // namespace vector_db
