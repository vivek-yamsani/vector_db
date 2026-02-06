//
// Created by Vivek Yamsani on 14/12/25.
//

#include "core/indices/hnsw.h"

#include <random>
#include <utility>
#include "core/collection.h"
#include "core/utils/util.h"


using namespace std;

namespace vector_db::indices::hnsw
{
index::index( wk_col_ptr _collection_ptr, const params& _params )
    : index_t( std::move( _collection_ptr ) )
    , params_( _params )
{
  if ( collection_ptr_.expired() )
  {
    throw std::runtime_error( "Collection pointer expired" );
  }
}

void index::no_lock_clear()
{
  to_be_inserted_.clear();
  to_be_removed_.clear();
  inserted_.clear();
  node_levels_.clear();
  neighbours_.clear();
  entry_point_ = 0;
  max_layer_ = -1;
}

void index::clear()
{
  std::unique_lock< std::shared_mutex > lock( mutex_ );
  no_lock_clear();
}

// distance helpers using collection-stored data
double index::dist( const id_t _a, const id_t _b, const col_ptr& col ) const
{
  if ( !col )
    throw std::runtime_error( "Collection expired" );

  auto va = col->get_vector_by_id( _a );
  auto vb = col->get_vector_by_id( _b );

  if ( !va || !vb )
    return std::numeric_limits< double >::max();
  return params_.distance_->compute( *va, *vb );
}

double index::dist( const float_vector& q, const id_t _b, const col_ptr& col ) const
{
  if ( !col )
    throw std::runtime_error( "Collection expired" );

  auto vb = col->get_vector_by_id( _b );
  if ( !vb )
    return std::numeric_limits< double >::max();
  return params_.distance_->compute( q, *vb );
}

int index::generate_random_level() const
{
  thread_local std::mt19937 rng( std::random_device{}() );
  std::uniform_real_distribution< double > dist( std::numeric_limits< double >::epsilon(), 1.0 );
  const double r = dist( rng );
  return static_cast< int >( -std::log( r ) * params_.ml_ );
}

index::cand_set_t index::search_layer( const float_vector& query,
                                       const id_set& entry_points,
                                       unsigned int ef,
                                       unsigned int level,
                                       const col_ptr& col )
{
  cand_set_t result;  // found nearest neighbors
  if ( entry_points.empty() )
    return result;

  visited_map_t visited;
  cand_set_t candidates;  // potential candidates
  for ( const auto& ep : entry_points )
  {
    result.emplace( dist( query, ep, col ), ep );
    candidates.emplace( dist( query, ep, col ), ep );
    visited[ ep ] = true;
  }

  while ( !candidates.empty() )
  {
    auto [ dist_curr_cand, cand_id ] = *candidates.begin();
    candidates.erase( candidates.begin() );  // extract nearest candidate

    auto [ dist_ele, _ ] = *result.rbegin();  // get farthest element dist
    if ( dist_curr_cand > dist_ele )
      break;

    for ( auto& neighbour : neighbours_[ level ][ cand_id ] )
    {
      if ( visited[ neighbour ] )
        continue;
      visited[ neighbour ] = true;
      auto [ farthest_ele_dist, _ ] = *result.rbegin();  // get farthest element dist
      auto d = dist( query, neighbour, col );
      if ( d < farthest_ele_dist || result.size() < ef )
      {
        candidates.emplace( d, neighbour );
        result.emplace( d, neighbour );
        if ( result.size() > ef )
          result.erase( --result.end() );
      }
    }
  }

  return result;
}

index::cand_set_t index::select_neighbors_heuristic( const cand_set_t& candidates, unsigned int no_of_cand ) const
{
  // Algorithm 4: Heuristic Neighbor Selection
  cand_set_t result_set;
  cand_set_t discarded_candidates;

  for ( const auto& element : candidates )
  {
    if ( result_set.size() >= no_of_cand )
      break;

    auto [ dist_from_q, _ ] = element;

    // element is closer to q compared to any element from result
    if ( result_set.empty() || dist_from_q < result_set.begin()->first )
    {
      result_set.insert( element );
    }
    else
    {
      discarded_candidates.insert( element );
    }
  }

  // "Backfill": fill remaining slots with closest from discarded
  if ( result_set.size() < no_of_cand )
  {
    for ( const auto& element : discarded_candidates )
    {
      if ( result_set.size() >= no_of_cand )
        break;
      result_set.insert( element );
    }
  }

  return result_set;
}

void index::init()
{
  std::unique_lock< std::shared_mutex > lock( mutex_ );
  auto col = collection_ptr_.lock();
  if ( !col )
    throw std::runtime_error( "Collection pointer expired during build" );
  const auto& src = col->get_vectors();
  no_lock_clear();
  if ( src.empty() )
    return;

  for ( const auto& [ fst, snd ] : src )
    to_be_inserted_.insert( fst );

  // Initialize the first point
  // entry_point_ = *to_be_inserted_.begin();
  // inserted_.insert( entry_point_ );
  // to_be_inserted_.erase( entry_point_ );
  // node_levels_[ entry_point_ ] = 0;
  // max_layer_ = 0;
  // neighbours_.resize( 1 );
  build( col );
}

void index::insert( id_t id, const col_ptr& col )
{
  cand_set_t candidates;  // currently found nearest elements
  int node_level = generate_random_level();
  node_levels_[ id ] = node_level;

  // Search from the top layer down to layer 0
  if ( entry_point_ == 0 )
  {
    entry_point_ = id;
  }
  id_set ep{ entry_point_ };
  for ( int lc = max_layer_; lc > node_level; --lc )
  {
    candidates = search_layer( *col->get_vector_by_id( id ), ep, 1, lc, col );
    ep = { candidates.begin()->second };
  }

  // Insert into layers from node_level down to 0
  for ( int lc = std::min( node_level, max_layer_ ); lc >= 0; --lc )
  {
    auto layer_M = lc == 0 ? params_.M0_ : params_.M_;
    candidates = search_layer( *col->get_vector_by_id( id ), { ep }, params_.ef_construction_, lc, col );
    auto selected_candidates = select_neighbors_heuristic( candidates, layer_M );

    // add bidirectional links
    for ( auto& [ _, neighbour_id ] : selected_candidates )
    {
      neighbours_[ lc ][ id ].insert( neighbour_id );
      neighbours_[ lc ][ neighbour_id ].insert( id );
    }

    // shrink connections
    for ( auto neighbour_id : neighbours_[ lc ][ id ] )
    {
      auto create_cand_set = [ this, inserting_id = id, col, lc ]( id_t node_id ) -> cand_set_t
      {
        cand_set_t candidates;
        for ( auto neighbour_id : neighbours_[ lc ][ node_id ] )
        {
          candidates.insert( { dist( inserting_id, node_id, col ), neighbour_id } );
        }

        return candidates;
      };

      auto create_neighbour_set = []( const cand_set_t& candidates )
      {
        id_set neighbour_set;
        for ( auto [ _, cand_id ] : candidates )
          neighbour_set.insert( cand_id );

        return neighbour_set;
      };

      if ( neighbours_[ lc ][ neighbour_id ].size() > layer_M )
      {
        const auto new_neighbour_cand = select_neighbors_heuristic( create_cand_set( neighbour_id ), layer_M );
        neighbours_[ lc ][ neighbour_id ] = create_neighbour_set( new_neighbour_cand );
      }
    }

    ep = [ candidates ]()
    {
      id_set res;
      for ( auto [ _, _id ] : candidates )
        res.insert( _id );
      return res;
    }();
  }

  if ( node_level > max_layer_ )
  {
    entry_point_ = id;
    max_layer_ = node_level;
    neighbours_.resize( max_layer_ + 1 );
  }

  inserted_.insert( id );
}

void index::build( const col_ptr& col )
{
  for ( const auto& id : to_be_removed_ )
  {
    for ( int layer = 0; layer <= max_layer_; ++layer )
    {
      for ( auto& nb : neighbours_[ layer ][ id ] )
      {
        neighbours_[ layer ][ nb ].erase( id );
      }
      neighbours_[ layer ].erase( id );
    }
    node_levels_.erase( id );
    inserted_.erase( id );
  }

  to_be_removed_.clear();
  if ( entry_point_ > 0 && !inserted_.count( entry_point_ ) )
  {
    if ( inserted_.empty() )
      entry_point_ = 0;
    else
    {
      for ( const auto& id : inserted_ )
      {
        if ( node_levels_[ id ] == max_layer_ )
        {
          entry_point_ = id;
          break;
        }
      }
    }
  }
  for ( const auto& id : to_be_inserted_ )
  {
    insert( id, col );
  }

  to_be_removed_.clear();
  to_be_inserted_.clear();
}

void index::search_knn( const float_vector& query, unsigned int k, vector< score_pair >& result )
{
  std::shared_lock< std::shared_mutex > lock( mutex_ );
  const auto col = collection_ptr_.lock();
  if ( !col )
    throw std::runtime_error( "Collection pointer expired during search" );

  {
    if ( !to_be_inserted_.empty() || !to_be_removed_.empty() )
    {
      lock.unlock();
      {
        std::unique_lock< std::shared_mutex > uni_lock( mutex_ );
        build( col );
      }
      lock.lock();
    }
  }
  result.clear();
  if ( inserted_.empty() || k == 0 )
    return;
  k = std::min< unsigned int >( k, inserted_.size() );

  // Search from the top layer down to layer 1
  id_set ep{ entry_point_ };
  for ( int lc = max_layer_; lc > 0; --lc )
  {
    ep = { search_layer( query, ep, 1, lc, col ).begin()->second };
  }
  auto candidates = search_layer( query, ep, std::max( k, params_.ef_search_ ), 0, col );

  result.reserve( k );
  for ( auto [ distance, id ] : candidates )
  {
    if ( result.size() >= k )
      break;
    result.emplace_back( distance, id_vector{ id, std::make_unique< float_vector >( *col->get_vector_by_id( id ) ) } );
  }
}

bool index::search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results )
{
  search_knn( query_vector, k, results );
  return true;
}

void index::on_vectors_added( const std::vector< id_t >& new_ids )
{
  unique_lock< shared_mutex > lock( mutex_ );
  for ( auto _id : new_ids )
  {
    if ( inserted_.count( _id ) )
    {
      to_be_removed_.insert( _id );
    }
    to_be_inserted_.insert( _id );
  }
}

void index::on_vectors_removed( const std::vector< id_t >& removed_ids )
{
  unique_lock< shared_mutex > lock( mutex_ );
  for ( auto _id : removed_ids )
  {
    if ( to_be_inserted_.count( _id ) )
      to_be_inserted_.erase( _id );
    else
      to_be_removed_.insert( _id );
  }
}

}  // namespace vector_db::indices::hnsw
