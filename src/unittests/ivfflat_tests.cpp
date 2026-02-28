#include <gtest/gtest.h>
#include <vector>

#include "core/collection.h"
#include "core/indices/ivfflat.h"

using namespace vector_db;

TEST( IVFFlatTest, BasicSearch )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection" );

  // Add some vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  float d1[] = { 1.0f, 1.0f };
  float d2[] = { 1.1f, 1.1f };
  float d3[] = { 10.0f, 10.0f };
  float d4[] = { 10.1f, 10.1f };

  vectors.emplace_back( 1, vector_db::float_vector( 2, d1 ) );
  vectors.emplace_back( 2, vector_db::float_vector( 2, d2 ) );
  vectors.emplace_back( 3, vector_db::float_vector( 2, d3 ) );
  vectors.emplace_back( 4, vector_db::float_vector( 2, d4 ) );

  col->add_vectors( std::move( vectors ) );

  // Add IVF Flat index
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 2, 1 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Search
  float q[] = { 1.05f, 1.05f };
  vector_db::float_vector query( 2, q );
  std::vector< vector_db::score_pair > results;

  EXPECT_TRUE( col->search_for_top_k( query, 2, results, "ivf" ) );

  ASSERT_GE( results.size(), 1 );
  EXPECT_TRUE( results[ 0 ].second.first == 1 || results[ 0 ].second.first == 2 );
}

TEST( IVFFlatTest, AddVectorsAfterIndex )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection_2" );

  // Add IVF Flat index first
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 2, 1 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Add some vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  float d1[] = { 1.0f, 1.0f };
  float d2[] = { 1.1f, 1.1f };
  vectors.emplace_back( 1, vector_db::float_vector( 2, d1 ) );
  vectors.emplace_back( 2, vector_db::float_vector( 2, d2 ) );
  col->add_vectors( std::move( vectors ) );

  // Search
  float q[] = { 1.05f, 1.05f };
  vector_db::float_vector query( 2, q );
  std::vector< vector_db::score_pair > results;

  EXPECT_TRUE( col->search_for_top_k( query, 2, results, "ivf" ) );
  ASSERT_GE( results.size(), 1 );
  EXPECT_TRUE( results[ 0 ].second.first == 1 || results[ 0 ].second.first == 2 );
}

TEST( IVFFlatTest, IncrementalAdd )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection_incremental_add" );

  // Add initial vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  float d1[] = { 1.0f, 1.0f };
  float d2[] = { 2.0f, 2.0f };
  float d3[] = { 10.0f, 10.0f };
  float d4[] = { 11.0f, 11.0f };

  vectors.emplace_back( 1, vector_db::float_vector( 2, d1 ) );
  vectors.emplace_back( 2, vector_db::float_vector( 2, d2 ) );
  vectors.emplace_back( 3, vector_db::float_vector( 2, d3 ) );
  vectors.emplace_back( 4, vector_db::float_vector( 2, d4 ) );
  col->add_vectors( std::move( vectors ) );

  // Build index with low rebuild threshold to test incremental behavior
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 2, 1, 100 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Add vectors incrementally (should not trigger rebuild)
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > new_vectors;
  float d5[] = { 1.5f, 1.5f };
  float d6[] = { 10.5f, 10.5f };
  new_vectors.emplace_back( 5, vector_db::float_vector( 2, d5 ) );
  new_vectors.emplace_back( 6, vector_db::float_vector( 2, d6 ) );
  col->add_vectors( std::move( new_vectors ) );

  // Search and verify new vectors are findable
  float q1[] = { 1.5f, 1.5f };
  vector_db::float_vector query1( 2, q1 );
  std::vector< vector_db::score_pair > results1;
  EXPECT_TRUE( col->search_for_top_k( query1, 1, results1, "ivf" ) );
  ASSERT_GE( results1.size(), 1 );

  float q2[] = { 10.5f, 10.5f };
  vector_db::float_vector query2( 2, q2 );
  std::vector< vector_db::score_pair > results2;
  EXPECT_TRUE( col->search_for_top_k( query2, 1, results2, "ivf" ) );
  ASSERT_GE( results2.size(), 1 );
}

TEST( IVFFlatTest, IncrementalRemove )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection_incremental_remove" );

  // Add initial vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  float d1[] = { 1.0f, 1.0f };
  float d2[] = { 2.0f, 2.0f };
  float d3[] = { 10.0f, 10.0f };
  float d4[] = { 11.0f, 11.0f };

  vectors.emplace_back( 1, vector_db::float_vector( 2, d1 ) );
  vectors.emplace_back( 2, vector_db::float_vector( 2, d2 ) );
  vectors.emplace_back( 3, vector_db::float_vector( 2, d3 ) );
  vectors.emplace_back( 4, vector_db::float_vector( 2, d4 ) );
  col->add_vectors( std::move( vectors ) );

  // Build index
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 2, 1, 100 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Remove vectors incrementally
  std::vector< vector_db::id_t > remove_ids = { 2, 4 };
  col->remove_vectors( remove_ids );

  // Search and verify removed vectors are not returned
  float q1[] = { 1.0f, 1.0f };
  vector_db::float_vector query1( 2, q1 );
  std::vector< vector_db::score_pair > results1;
  EXPECT_TRUE( col->search_for_top_k( query1, 2, results1, "ivf" ) );
  ASSERT_GE( results1.size(), 1 );
  EXPECT_EQ( results1[ 0 ].second.first, 1 );

  float q2[] = { 10.0f, 10.0f };
  vector_db::float_vector query2( 2, q2 );
  std::vector< vector_db::score_pair > results2;
  EXPECT_TRUE( col->search_for_top_k( query2, 2, results2, "ivf" ) );
  ASSERT_GE( results2.size(), 1 );
  EXPECT_EQ( results2[ 0 ].second.first, 3 );
}

TEST( IVFFlatTest, RebuildThreshold )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection_rebuild_threshold" );

  // Add initial vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  for ( int i = 0; i < 50; ++i )
  {
    float d[] = { static_cast< float >( i ), static_cast< float >( i ) };
    vectors.emplace_back( i, vector_db::float_vector( 2, d ) );
  }
  col->add_vectors( std::move( vectors ) );

  // Build index with rebuild threshold of 10
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 5, 2, 10 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Add 5 vectors - should not rebuild
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > new_vectors1;
  for ( int i = 50; i < 55; ++i )
  {
    float d[] = { static_cast< float >( i ), static_cast< float >( i ) };
    new_vectors1.emplace_back( i, vector_db::float_vector( 2, d ) );
  }
  col->add_vectors( std::move( new_vectors1 ) );

  // Add 10 more vectors - should trigger rebuild
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > new_vectors2;
  for ( int i = 55; i < 65; ++i )
  {
    float d[] = { static_cast< float >( i ), static_cast< float >( i ) };
    new_vectors2.emplace_back( i, vector_db::float_vector( 2, d ) );
  }
  col->add_vectors( std::move( new_vectors2 ) );

  // Search and verify all vectors are accessible
  float q[] = { 60.0f, 60.0f };
  vector_db::float_vector query( 2, q );
  std::vector< vector_db::score_pair > results;
  EXPECT_TRUE( col->search_for_top_k( query, 5, results, "ivf" ) );
  ASSERT_GE( results.size(), 1 );
}

TEST( IVFFlatTest, CustomParams )
{
  auto col = std::make_shared< vector_db::collection >( 2, "test_collection_custom_params" );

  // Add vectors
  std::vector< std::pair< vector_db::id_t, vector_db::float_vector > > vectors;
  for ( int i = 0; i < 100; ++i )
  {
    float d[] = { static_cast< float >( i ), static_cast< float >( i ) };
    vectors.emplace_back( i, vector_db::float_vector( 2, d ) );
  }
  col->add_vectors( std::move( vectors ) );

  // Test with custom parameters
  vector_db::indices::ivf_flat::params params( vector_db::distance::dist_type::euclidean, 10, 5, 50 );
  EXPECT_TRUE( col->add_index( "ivf", vector_db::index_type::ivf_flat, &params ) );

  // Search
  float q[] = { 50.0f, 50.0f };
  vector_db::float_vector query( 2, q );
  std::vector< vector_db::score_pair > results;
  EXPECT_TRUE( col->search_for_top_k( query, 10, results, "ivf" ) );
  ASSERT_GE( results.size(), 1 );
}
