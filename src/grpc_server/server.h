//
// Created by Vivek Yamsani on 30/11/25.
//
#pragma once
#include "db.grpc.pb.h"

#include "core/database.h"
#include "worker_pool.h"

#include <grpcpp/grpcpp.h>
#include <atomic>
#include <memory>
#include <string>
#include <thread>

namespace vector_db
{

class server
{
public:
  explicit server( std::string address, int _num_cq_threads = 0 );

  void start();
  void shutdown();

  bool is_running() const { return running_; }

  void attach() const;

private:
  // Per-RPC handlers (CallData) manage their own lifetime
  struct create_collection_handler;
  struct delete_collection_handler;
  struct upsert_handler;
  struct search_handler;
  struct stream_upsert_handler;
  struct delete_vector_handler;
  struct add_index_handler;
  struct get_index_handler;

  template <typename ...rpc>
  void init_rpc_handlers();

  void cq_poll_loop() const;

  std::shared_ptr< spdlog::logger > logger_;
  std::unique_ptr< worker_pool > db_worker_pool_;
  std::string address_;
  std::unique_ptr< database > db_ptr_;
  std::unique_ptr< grpc::Server > server_;
  std::unique_ptr< grpc::ServerCompletionQueue > cq_;
  vectorService::AsyncService service_;
  std::vector< std::thread > cq_threads_;
  static uint8_t num_of_thread_;
  std::atomic< bool > running_{ false };
};
}  // namespace vector_db