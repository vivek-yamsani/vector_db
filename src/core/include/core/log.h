//
// Created by Vivek Yamsani on 07/12/25.
//

#pragma once
#include <memory>
#include <string>
#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

namespace vector_db
{
class init_logger
{
public:
  explicit init_logger( std::string name )
  {
    static bool _exec_only_once = []()
    {
      spdlog::init_thread_pool( 8192, 2 );
      return true;
    }();
    auto logger = std::make_shared< spdlog::async_logger >(
        std::move( name ), get_sink(), spdlog::thread_pool(), spdlog::async_overflow_policy::block );
    spdlog::register_logger( logger );
    logger->set_pattern( "[%H:%M:%S:%e][thread %t][%=8l][%=6n] %v" );
    spdlog::flush_every( std::chrono::seconds( 1 ) );
  }

  static std::shared_ptr< spdlog::sinks::sink > get_sink()
  {
    static auto _sink = std::make_shared< spdlog::sinks::rotating_file_sink_mt >( "logs/vector_db.log", 1048576 * 30, 3 );
    return _sink;
  }
};
}  // namespace vector_db
