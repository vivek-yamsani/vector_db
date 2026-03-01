#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

#include "configuration/provider.h"
#include "core/database.h"
#include "core/indices/hnsw.h"
#include "core/indices/ivfflat.h"

namespace vector_db::test
{

class PersistenceTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    std::filesystem::remove_all( "test_data" );
    std::filesystem::create_directories( "test_data" );

    // Setup config for test
    auto config = config_provider::get_instance();
    // Since we can't easily modify the singleton's internal state without a file,
    // we hope it uses our "test_data" if we set it up right,
    // OR we can just rely on the default and clean it up.
    // Actually, let's create a temporary config file.
    std::ofstream ofs( "test_config.toml" );
    ofs << "[storage]\npath = \"test_data\"\nenabled = true\n";
    ofs.close();
    config->load( "test_config.toml" );
  }

  void TearDown() override
  {
    std::filesystem::remove_all( "test_data" );
    std::filesystem::remove( "test_config.toml" );
  }
};

TEST_F( PersistenceTest, SaveAndLoadDatabase )
{
  {
    database db;
    db.add_collection( "test_col", 3 );

    std::vector< float > v1_data = { 1.0f, 2.0f, 3.0f };
    float_vector v1( 3, v1_data.data() );
    v1.add_metadata( "key", "value" );

    std::vector< std::pair< id_t, float_vector > > vectors;
    vectors.push_back( { 1, std::move( v1 ) } );

    db.add_vectors( "test_col", std::move( vectors ) );

    // Add an index
    indices::hnsw::params params( distance::dist_type::euclidean, 16, 64, 32 );
    db.add_index( "test_col", "hnsw_idx", index_type::hnsw, &params );

    auto status = db.save();
    EXPECT_EQ( status, status::success );
  }

  // Now load in a new database instance
  {
    database db;
    auto status = db.load();
    EXPECT_EQ( status, status::success );

    auto info_res = db.get_collection_info( "test_col" );
    ASSERT_EQ( info_res.status_, status::success );
    EXPECT_EQ( info_res.payload_->dimension_, 3 );
    EXPECT_EQ( info_res.payload_->name_, "test_col" );

    // Verify vector exists
    float_vector query( 3, std::vector< float >{ 1.0f, 2.0f, 3.0f }.data() );
    auto search_res = db.get_nearest_k( "test_col", query, 1 );
    ASSERT_EQ( search_res.status_, status::success );
    ASSERT_EQ( search_res.payload_->size(), 1 );
    EXPECT_EQ( search_res.payload_->at( 0 ).second.first, 1 );  // ID 1

    // Verify index exists and params are correct
    auto idx_res = db.get_index_params( "test_col", "hnsw_idx" );
    ASSERT_EQ( idx_res.status_, status::success );
    EXPECT_EQ( idx_res.payload_->first, index_type::hnsw );
    auto* loaded_params = static_cast< const indices::hnsw::params* >( idx_res.payload_->second );
    EXPECT_EQ( loaded_params->M_, 16 );
  }
}

TEST_F( PersistenceTest, IVFFlatPersistence )
{
  {
    database db;
    db.add_collection( "ivf_col", 3 );

    std::vector< float > v1_data = { 1.0f, 0.0f, 0.0f };
    std::vector< float > v2_data = { 0.0f, 1.0f, 0.0f };

    std::vector< std::pair< id_t, float_vector > > vectors;
    vectors.push_back( { 1, float_vector( 3, v1_data.data() ) } );
    vectors.push_back( { 2, float_vector( 3, v2_data.data() ) } );

    db.add_vectors( "ivf_col", std::move( vectors ) );

    indices::ivf_flat::params params( distance::dist_type::euclidean, 2, 1, 10 );
    db.add_index( "ivf_col", "ivf_idx", index_type::ivf_flat, &params );

    db.save();
  }

  {
    database db;
    db.load();

    auto idx_res = db.get_index_params( "ivf_col", "ivf_idx" );
    ASSERT_EQ( idx_res.status_, status::success );
    EXPECT_EQ( idx_res.payload_->first, index_type::ivf_flat );
    auto* loaded_params = static_cast< const indices::ivf_flat::params* >( idx_res.payload_->second );
    EXPECT_EQ( loaded_params->k_, 2 );
    EXPECT_EQ( loaded_params->n_probe_, 1 );

    float_vector query( 3, std::vector< float >{ 1.0f, 0.1f, 0.1f }.data() );
    auto search_res = db.get_nearest_k( "ivf_col", query, 1 );
    ASSERT_EQ( search_res.status_, status::success );
    ASSERT_EQ( search_res.payload_->size(), 1 );
    EXPECT_EQ( search_res.payload_->at( 0 ).second.first, 1 );
  }
}

}  // namespace vector_db::test
