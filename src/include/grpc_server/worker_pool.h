//
// Created by Vivek Yamsani on 03/12/25.
//
#pragma once
#include <condition_variable>
#include <functional>
#include <thread>
#include "logger/logger.h"

namespace vector_db
{
class worker_pool
{
  using cb = std::function< void() >;
  static constexpr size_t MAX_QUEUE_SIZE = 10000;
  size_t num_threads_;
  std::vector< std::thread > threads_;
  std::queue< cb > tasks_;
  std::mutex mutex_;
  std::condition_variable cv_;
  std::atomic< bool > stopped{ false };
  std::shared_ptr< details::logger_impl > logger_;

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
        logger_->error( "Worker thread exception: {}", e.what() );
      }
    }
  }

  void init()
  {
    logger_ = logger_factory::create( "db_wrk_pool" );
    logger_->info( "Starting {} DB worker threads", num_threads_ );
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

  ~worker_pool() { shutdown(); }

  void submit( cb task )
  {
    {
      std::lock_guard< std::mutex > lock( mutex_ );
      if ( tasks_.size() >= MAX_QUEUE_SIZE )
      {
        throw std::runtime_error( "Task queue full" );
      }
      tasks_.emplace( std::move( task ) );
    }
    cv_.notify_one();
  }

  void shutdown()
  {
    {
      std::lock_guard< std::mutex > lock( mutex_ );
      if ( stopped.load() )
        return;
      stopped.store( true );
    }
    cv_.notify_all();
    for ( auto& t : threads_ )
      if ( t.joinable() )
        t.join();
  }
};
}  // namespace vector_db