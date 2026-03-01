//
// Unit tests for database result-based API
//

#include <gtest/gtest.h>

#include "core/database.h"
#include "core/indices/hnsw.h"
#include "core/indices/ivfflat.h"

using namespace vector_db;

class DatabaseResultTests : public ::testing::Test
{
protected:
  database db;

  void SetUp() override
  {
    // Create a test collection
    auto status = db.add_collection( "test_collection", 3 );
    ASSERT_EQ( status, status::success );
  }
};

TEST_F( DatabaseResultTests, GetAllCollectionsReturnsResult )
{
  // Test get_all_collections returns result with payload
  auto result = db.get_all_collections();

  EXPECT_TRUE( result.is_success() );
  EXPECT_EQ( result.status_, status::success );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value().size(), 1 );
  EXPECT_EQ( result.value()[ 0 ].first, "test_collection" );
  EXPECT_EQ( result.value()[ 0 ].second.dimension_, 3 );
}

TEST_F( DatabaseResultTests, GetAllCollectionsMultipleCollections )
{
  // Add more collections
  db.add_collection( "collection2", 5 );
  db.add_collection( "collection3", 10 );

  auto result = db.get_all_collections();

  EXPECT_TRUE( result.is_success() );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value().size(), 3 );
}

TEST_F( DatabaseResultTests, GetCollectionInfoSuccess )
{
  // Test get_collection_info returns result with correct data
  auto result = db.get_collection_info( "test_collection" );

  EXPECT_TRUE( result.is_success() );
  EXPECT_EQ( result.status_, status::success );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value().dimension_, 3 );
  EXPECT_EQ( result.value().name_, "test_collection" );
}

TEST_F( DatabaseResultTests, GetCollectionInfoNotFound )
{
  // Test get_collection_info for non-existent collection
  auto result = db.get_collection_info( "non_existent" );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_does_not_exist );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetCollectionInfoInvalidName )
{
  // Test get_collection_info with invalid name
  auto result = db.get_collection_info( "" );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_name_cant_be_empty );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetNearestKReturnsResult )
{
  // Add some vectors
  std::vector< std::pair< vector_db::id_t, float_vector > > vectors;
  vectors.emplace_back( 1, float_vector{ 3, std::vector< float >{ 1.0f, 2.0f, 3.0f }.data() } );
  vectors.emplace_back( 2, float_vector{ 3, std::vector< float >{ 4.0f, 5.0f, 6.0f }.data() } );
  vectors.emplace_back( 3, float_vector{ 3, std::vector< float >{ 7.0f, 8.0f, 9.0f }.data() } );

  auto add_status = db.add_vectors( "test_collection", vectors );
  ASSERT_EQ( add_status, status::success );

  // Query for nearest neighbors
  float_vector query{ 3, std::vector< float >{ 1.0f, 2.0f, 3.0f }.data() };
  auto result = db.get_nearest_k( "test_collection", query, 2 );

  EXPECT_TRUE( result.is_success() );
  EXPECT_EQ( result.status_, status::success );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_LE( result.value().size(), 2 );
}

TEST_F( DatabaseResultTests, GetNearestKCollectionNotFound )
{
  // Test get_nearest_k for non-existent collection
  float_vector query{ 3, std::vector< float >{ 1.0f, 2.0f, 3.0f }.data() };
  auto result = db.get_nearest_k( "non_existent", query, 5 );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_does_not_exist );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetNearestKInvalidCollectionName )
{
  // Test get_nearest_k with invalid collection name
  float_vector query{ 3, std::vector< float >{ 1.0f, 2.0f, 3.0f }.data() };
  auto result = db.get_nearest_k( "", query, 5 );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_name_cant_be_empty );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetIndexParamsHNSWSuccess )
{
  // Add an HNSW index
  auto hnsw_params = indices::hnsw::params();
  hnsw_params.M_ = 16;
  hnsw_params.ef_construction_ = 200;
  hnsw_params.ef_search_ = 50;

  auto add_status = db.add_index( "test_collection", "test_index", index_type::hnsw, &hnsw_params );
  ASSERT_EQ( add_status, status::success );

  // Get index params
  auto result = db.get_index_params( "test_collection", "test_index" );

  EXPECT_TRUE( result.is_success() );
  EXPECT_EQ( result.status_, status::success );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value().first, index_type::hnsw );
  EXPECT_NE( result.value().second, nullptr );

  // Cast to HNSW params and verify
  auto* params = dynamic_cast< const indices::hnsw::params* >( result.value().second );
  ASSERT_NE( params, nullptr );
  EXPECT_EQ( params->M_, 16 );
  EXPECT_EQ( params->ef_construction_, 200 );
  EXPECT_EQ( params->ef_search_, 50 );
}

TEST_F( DatabaseResultTests, GetIndexParamsIVFFlatSuccess )
{
  // Add some vectors first
  std::vector< std::pair< vector_db::id_t, float_vector > > vectors;
  for ( int i = 0; i < 100; ++i )
  {
    vectors.emplace_back( i, float_vector{ 3, std::vector< float >{ 1.0f * i, 2.0f * i, 3.0f * i }.data() } );
  }
  db.add_vectors( "test_collection", vectors );

  // Add an IVF-Flat index
  auto ivf_params = indices::ivf_flat::params();
  ivf_params.k_ = 10;
  ivf_params.n_probe_ = 3;
  ivf_params.rebuild_threshold_ = 1000;

  auto add_status = db.add_index( "test_collection", "ivf_index", index_type::ivf_flat, &ivf_params );
  ASSERT_EQ( add_status, status::success );

  // Get index params
  auto result = db.get_index_params( "test_collection", "ivf_index" );

  EXPECT_TRUE( result.is_success() );
  EXPECT_EQ( result.status_, status::success );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value().first, index_type::ivf_flat );
  EXPECT_NE( result.value().second, nullptr );

  // Cast to IVF-Flat params and verify
  auto* params = dynamic_cast< const indices::ivf_flat::params* >( result.value().second );
  ASSERT_NE( params, nullptr );
  EXPECT_EQ( params->k_, 10 );
  EXPECT_EQ( params->n_probe_, 3 );
  EXPECT_EQ( params->rebuild_threshold_, 1000 );
}

TEST_F( DatabaseResultTests, GetIndexParamsIndexNotFound )
{
  // Test get_index_params for non-existent index
  auto result = db.get_index_params( "test_collection", "non_existent_index" );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::index_does_not_exist );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetIndexParamsCollectionNotFound )
{
  // Test get_index_params for non-existent collection
  auto result = db.get_index_params( "non_existent_collection", "some_index" );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_does_not_exist );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, GetIndexParamsInvalidCollectionName )
{
  // Test get_index_params with invalid collection name
  auto result = db.get_index_params( "", "some_index" );

  EXPECT_FALSE( result.is_success() );
  EXPECT_EQ( result.status_, status::collection_name_cant_be_empty );
  EXPECT_FALSE( result.has_payload() );
}

TEST_F( DatabaseResultTests, ResultStructBasicFunctionality )
{
  // Test result struct basic operations
  result< int > success_result( status::success, 42 );
  EXPECT_TRUE( success_result.is_success() );
  EXPECT_TRUE( success_result.has_payload() );
  EXPECT_EQ( success_result.value(), 42 );

  result< int > failure_result( status::internal_error );
  EXPECT_FALSE( failure_result.is_success() );
  EXPECT_FALSE( failure_result.has_payload() );
}

TEST_F( DatabaseResultTests, ResultStructWithString )
{
  // Test result with string payload
  result< std::string > result( status::success, std::string( "test_value" ) );
  EXPECT_TRUE( result.is_success() );
  EXPECT_TRUE( result.has_payload() );
  EXPECT_EQ( result.value(), "test_value" );
}
