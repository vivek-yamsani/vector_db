//
// Created by Vivek Yamsani on 28/11/25.
//
#include <csignal>

#include "grpc_server/server.h"
#include "logger.h"

using namespace std::chrono_literals;

std::unique_ptr< vector_db::server > srv_ptr;
std::atomic_bool running = true;

vector_db::init_logger logger( "main" );

int main()
{
  const char* port = std::getenv( "PORT" );
  const char* log_level = std::getenv( "LOG_LEVEL" );
  auto _logger = spdlog::get( "main" );
  if ( log_level == nullptr )
  {
    _logger->info( "LOG_LEVEL env var not set, defaulting to info" );
    log_level = "info";
  }
  spdlog::set_level( spdlog::level::from_str( log_level ) );
  if ( port == nullptr || std::atoi( port ) == 0 )
  {
    _logger->info( "PORT env var not set, defaulting to 50051" );
    port = "50051";
  }
  auto kill = []( int )
  {
    spdlog::get( "main" )->info( "Received shutdown signal, shutting down server" );
    srv_ptr->shutdown();
    running.store( false );
  };

  std::signal( SIGINT, kill );
  std::signal( SIGTERM, kill );

  srv_ptr = std::make_unique< vector_db::server >( "0.0.0.0:" + std::string{ port }, 10 );
  srv_ptr->start();
  // srv_ptr->attach();  // attaching the main thread to grpc server processing
  while ( running.load() )
  {
    std::this_thread::sleep_for( 1000ms );
  }
  return 0;
}