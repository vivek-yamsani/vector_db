#include <gtest/gtest.h>

#include "configuration/provider.h"
#include "logger/logger.h"

int main( int argc, char** argv )
{
  ::testing::InitGoogleTest( &argc, argv );

  auto* config = vector_db::config_provider::get_instance();
  config->load( "/Users/vivek.yamsani/Projects/vector_db/src/config.toml" );

  vector_db::logger_factory::initialize();

  return RUN_ALL_TESTS();
}
