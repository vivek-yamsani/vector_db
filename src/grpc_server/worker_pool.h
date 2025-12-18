//
// Created by Vivek Yamsani on 03/12/25.
//
#pragma once
#include <condition_variable>
#include <functional>
#include <thread>
#include "spdlog/spdlog.h"

namespace vector_db
{
class worker_pool
{
  using cb = std::function< void() >;
  size_t num_threads_;
  std::vector< std::thread > threads_;
  std::queue< cb > tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic< bool > stopped{ false };

  void worker_thread()
  {
    while ( !stopped.load() )
    {
      cb task;
      {
        std::unique_lock< std::mutex > lock( mutex_ );
        cv_.wait( lock, [ this ] { return !tasks_.empty() || stopped; } );
        if ( stopped )
          return;
        task = std::move( tasks_.front() );
        tasks_.pop();
      }
      try
      {
        task();
      }
      catch ( const std::exception& e )
      {
        std::cerr << e.what() << std::endl;
      }
    }
  }

  void init()
  {
    spdlog::get( "db" )->info( "Starting {} DB worker threads", num_threads_ );
    for ( size_t i = 0; i < num_threads_; ++i )
      threads_.emplace_back( [ this ]() { worker_thread(); } );
  }

public:
  worker_pool()
  {
    num_threads_ = std::thread::hardware_concurrency();
    init();
  }

  explicit worker_pool( const size_t num_threads )
  {
    num_threads_ = num_threads;
    init();
  }

  void submit( cb task )
  {
    {
      std::lock_guard< std::mutex > lock( mutex_ );
      tasks_.emplace( std::move( task ) );
    }
    cv_.notify_one();
  }

  void shutdown()
  {
    {
      std::lock_guard< std::mutex > lock( mutex_ );
      stopped = true;
    }
    cv_.notify_all();
    for ( auto& t : threads_ )
      if ( t.joinable() )
        t.join();
  }
};
}  // namespace vector_db