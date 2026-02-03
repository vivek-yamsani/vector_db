//
// Created by Vivek Yamsani on 14/12/25.
//
#pragma once
#include <functional>
#include <queue>
#include <set>
#include <shared_mutex>
#include <unordered_set>
#include <utility>
#include <vector>

#include "core/distance.h"
#include "core/float_vector.h"
#include "core/indices/index.h"
#include "core/utils/splitmix_hash.h"

namespace vector_db::indices::hnsw
{

struct params : params_t
{
  distance::ptr distance_{ nullptr };
  unsigned int M_;                // max neighbors per node in other layers
  unsigned int M0_;               // max neighbors per node in layer 0
  unsigned int ef_construction_;  // candidate list size during construction
  unsigned int ef_search_;        // candidate list size during search
  double ml_;                     // layer selection multiplier

  explicit params( const distance::dist_type _dist_type = distance::dist_type::cosine,
                   const unsigned int _m = 16,
                   const unsigned int _ef_construction = 64,
                   const unsigned int _ef_search = 32 )
      : M_( _m )
      , ef_construction_( _ef_construction )
      , ef_search_( _ef_search )
  {
    distance_ = get_distance_instance( _dist_type );
    if ( !distance_ )
      throw std::runtime_error( "Unknown distance type" );
    ml_ = 1.0 / std::log( M_ );
    M0_ = 2 * M_;
  }

  void clear()
  {
    M_ = 0;
    M0_ = 0;
    ef_construction_ = 0;
    ef_search_ = 0;
    ml_ = 0.0;
  }
};

// Layered HNSW graph index for KNN over float_vector
class index : public index_t
{
  using col_ptr = std::shared_ptr< collection >;
  using index_t::wk_col_ptr;
  using cand_t = std::pair< double, id_t >;
  using cand_set_t = std::set< cand_t >;
  using id_set = std::unordered_set< id_t, hash >;
  using id_map = std::unordered_map< id_t, int, hash >;
  using visited_map_t = std::unordered_map< id_t, bool, hash >;
  using neighbours_t = std::unordered_map< id_t, id_set, hash >;

  mutable std::shared_mutex mutex_;

  params params_;

  id_set inserted_;
  id_set to_be_inserted_;
  id_set to_be_removed_;
  id_map node_levels_;                      // max level for each node
  std::vector< neighbours_t > neighbours_;  // neighbours_[level][node] = neighbors of `node` in the `level`
  id_t entry_point_ = 0;                    // global entry point (node with max level)
  int max_layer_ = -1;                      // highest layer in the graph

public:
  index() = delete;

  explicit index( wk_col_ptr _collection_ptr, const params& _params = params( distance::dist_type::cosine ) );

  void clear();

  void init() override;

  // Search top-k neighbors for a query
  void search_knn( const float_vector& query, unsigned int k, std::vector< score_pair >& result );

  bool search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results ) override;

  index_type get_index_type() const override { return index_type::hnsw; }

  // Incremental update hooks; for now we invalidate and rebuild lazily
  void on_vectors_added( const std::vector< id_t >& new_ids ) override;
  void on_vectors_removed( const std::vector< id_t >& removed_ids ) override;

private:
  void no_lock_clear();

  cand_set_t search_layer( const float_vector& query,
                           const id_set& entry_points,
                           unsigned int ef,
                           unsigned int layer,
                           const col_ptr& col );

  void insert( id_t id, const col_ptr& col );

  void build( const col_ptr& col );

  // compute distance using vectors stored in the owning collection via weak ptr
  // arguments are internal indices
  double dist( id_t _a, id_t _b, const col_ptr& col ) const;
  double dist( const float_vector& q, id_t _b, const col_ptr& col ) const;

  int generate_random_level() const;

  cand_set_t select_neighbors_heuristic( const cand_set_t& candidates, unsigned int no_of_cand ) const;
};
}  // namespace vector_db::indices::hnsw
