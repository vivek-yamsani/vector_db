//
// Created by Vivek Yamsani on 28/11/25.
//
#include <csignal>
#include <filesystem>

#include "configuration/provider.h"
#include "grpc_server/server.h"
#include "logger/logger.h"

using namespace std::chrono_literals;
namespace fs = std::filesystem;

std::atomic_bool running{ true };

void sig_handler( int ) { running.store( false, std::memory_order_relaxed ); }

int main( int argc, char** argv )
{
  using vector_db::config_provider;
  using vector_db::server;

  vector_db::logger_factory::initialize();

  auto config_provider_ = config_provider::get_instance();

  if ( argc > 1 )
  {
    if ( !fs::exists( argv[ 1 ] ) )
    {
      throw std::runtime_error( std::string{ argv[ 1 ] } + " Configuration file does not exist" );
    }
    config_provider_->load( argv[ 1 ] );
  }

  const auto log_level = config_provider_->get_string( "main.log_level" ).value_or( "info" );

  const auto _logger = vector_db::logger_factory::create( "main" );
  _logger->set_level( log_level );

  std::signal( SIGINT, sig_handler );
  std::signal( SIGTERM, sig_handler );

  const auto srv_ptr = std::make_unique< server >();
  srv_ptr->start();

  while ( running.load( std::memory_order_relaxed ) )
  {
    std::this_thread::sleep_for( 1000ms );
  }

  vector_db::logger_factory::create( "main" )->info( "Received shutdown signal, shutting down server" );
  srv_ptr->shutdown();
  return 0;
}