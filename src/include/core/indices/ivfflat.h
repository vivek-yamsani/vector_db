//
// Created by Vivek Yamsani on 09/12/25.
//
#pragma once

#include <shared_mutex>

#include "core/distance.h"
#include "index.h"

namespace vector_db::indices::ivf_flat
{

struct params : params_t
{
  distance::dist_type dist_type_{ distance::dist_type::euclidean };
  unsigned int k_{ 100 };             // number of clusters
  unsigned int n_probe_{ 10 };        // number of clusters to search
  size_t rebuild_threshold_{ 1000 };  // rebuild after this many changes

  explicit params( distance::dist_type dist_type = distance::dist_type::euclidean,
                   unsigned int k = 100,
                   unsigned int n_probe = 10,
                   size_t rebuild_threshold = 1000 )
      : dist_type_( dist_type )
      , k_( k )
      , n_probe_( n_probe )
      , rebuild_threshold_( rebuild_threshold )
  {
  }

  void serialize( std::ostream& os ) const override
  {
    os.write( reinterpret_cast< const char* >( &dist_type_ ), sizeof( dist_type_ ) );
    os.write( reinterpret_cast< const char* >( &k_ ), sizeof( k_ ) );
    os.write( reinterpret_cast< const char* >( &n_probe_ ), sizeof( n_probe_ ) );
    os.write( reinterpret_cast< const char* >( &rebuild_threshold_ ), sizeof( rebuild_threshold_ ) );
  }

  static params deserialize( std::istream& is )
  {
    distance::dist_type dist_type;
    unsigned int k, n_probe;
    size_t rebuild_threshold;
    is.read( reinterpret_cast< char* >( &dist_type ), sizeof( dist_type ) );
    is.read( reinterpret_cast< char* >( &k ), sizeof( k ) );
    is.read( reinterpret_cast< char* >( &n_probe ), sizeof( n_probe ) );
    is.read( reinterpret_cast< char* >( &rebuild_threshold ), sizeof( rebuild_threshold ) );
    return params( dist_type, k, n_probe, rebuild_threshold );
  }

  std::unique_ptr< params_t > clone() const override
  {
    return std::make_unique< params >( dist_type_, k_, n_probe_, rebuild_threshold_ );
  }
};

class index : public index_t
{
  using index_t::wk_col_ptr;
  mutable std::shared_mutex mutex_;
  params params_;

  struct cluster
  {
    float_vector centroid;
    std::vector< id_t > vector_ids;
  };

  std::vector< cluster > clusters_;
  size_t vectors_since_rebuild_{ 0 };

public:
  index() = delete;
  explicit index( wk_col_ptr _collection_ptr, const params& _params = params() );

  void init() override;
  bool search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results ) override;
  index_type get_index_type() const override { return index_type::ivf_flat; }

  const params* get_params() const override { return &params_; }

  void serialize( std::ostream& os ) const override { params_.serialize( os ); }

  void on_vectors_added( const std::vector< id_t >& new_ids ) override;
  void on_vectors_removed( const std::vector< id_t >& removed_ids ) override;

private:
  void build();
  void add_vectors_incremental( const std::vector< id_t >& new_ids );
  void remove_vectors_incremental( const std::vector< id_t >& removed_ids );
  size_t find_nearest_cluster( const float_vector& vec ) const;
};

}  // namespace vector_db::indices::ivf_flat