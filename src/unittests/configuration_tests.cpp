//
// Created by Vivek Yamsani on 05/02/26.
//

#include <gtest/gtest.h>
#include <toml.hpp>

#include "configuration/provider.h"

TEST( ConfigurationTests, Basic )
{
  using namespace toml::toml_literals;
  const std::string config = R"(
[main]
log_level = "debug"

[core]
log_level = "debug"

[server]
threads = 4
port = 50051
db_worker_pool_size = 10
log_level = "debug"

)";

  // auto parsed = toml::parse_str( config );

  auto parsed = toml::parse( "/Users/vivek.yamsani/Projects/vector_db/src/config.toml" );

  using vector_db::config_provider;

  config_provider* config_provider_ = config_provider::get_instance();
  config_provider_->load( "/Users/vivek.yamsani/Projects/vector_db/src/config.toml" );


  EXPECT_TRUE( parsed.at( "server" ).at( "threads" ).as_integer() == config_provider_->get_int( "server", "threads" ).value() );

  EXPECT_TRUE( 4 == config_provider_->get_int( "server", "threads" ).value() );
}