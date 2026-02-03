//
// Created by Vivek Yamsani on 30/11/25.
//

#include "grpc_server/server.h"
#include "grpc_server/util.h"
#include "logger.h"

using namespace std::chrono_literals;

namespace vector_db
{
inline uint8_t server::num_of_thread_ = std::thread::hardware_concurrency();

struct rpc_facade
{
  virtual ~rpc_facade() = default;
  virtual void run_flow( const bool _ok, const bool _shutdown ) = 0;
};

// Base class helper for per-RPC state machines
template< typename derived_t, typename request_t, typename response_t >
struct rpc_base : public rpc_facade
{
  grpc::ServerContext server_ctx_;
  std::shared_ptr< spdlog::logger > logger_;
  vectorService::AsyncService* service_{ nullptr };
  grpc::ServerCompletionQueue* cq_{ nullptr };
  database* db_ptr_{ nullptr };
  worker_pool* db_worker_pool_{ nullptr };
  void* tag_{ this };

  // CREATED -> READ -> PROCESSED

  enum class state
  {
    CREATED,    // just created
    READ,       // request has been read and ready to be processed
    WRITTEN,    // for server streaming [reserved]
    PROCESSED,  // request has been processed. can be deleted
  } state_{ state::CREATED };

  request_t request_{};
  response_t response_{};
  grpc::Status status_;

  explicit rpc_base( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : service_( s )
      , cq_( q )
      , db_ptr_( db )
      , db_worker_pool_( pool )
  {
    logger_ = spdlog::get( "server" );
  }
  ~rpc_base() override = default;
  virtual void do_delete() { delete this; }
  virtual bool do_read() { return true; }  // normal unary req/res will already have the request obj populated
  virtual void handle_unknown_error() = 0;
  void creat_new_instance() { new derived_t( service_, cq_, db_ptr_, db_worker_pool_ ); }
  virtual void process() = 0;
  void run_flow( const bool _ok, const bool _shutdown ) override
  {
    if ( _shutdown )
    {
      do_delete();
      return;
    }

    if ( !_ok && state_ != state::PROCESSED )
    {
      handle_unknown_error();
      state_ = state::PROCESSED;
      return;
    }

    switch ( state_ )
    {
      case state::CREATED:
        creat_new_instance();
        if ( !do_read() )  // if read has already happened,
                           // then we receive true and process the request, else we trigger an async read and return false
          break;
      case state::READ:
        process();
        break;
      case state::PROCESSED:
        do_delete();
        break;
      default:
        handle_unknown_error();
        state_ = state::PROCESSED;
        break;
    }
  };
};

// CreateCollection unary
struct server::create_collection_handler : public rpc_base< create_collection_handler, CreateCollectionRequest, EmptyResponse >
{
  grpc::ServerAsyncResponseWriter< EmptyResponse > responder;

  create_collection_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , responder( &server_ctx_ )
  {
    service_->RequestCreateCollection( &server_ctx_, &request_, &responder, cq_, cq_, this );
  }

  void handle_unknown_error() override
  {
    responder.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
  }
  void process() override
  {
    logger_->info( "Create collection request: name {}, dimension {}", request_.name(), request_.dimension() );
    db_worker_pool_->submit(
        [ this ]()
        {
          try
          {
            const auto _status = db_ptr_->add_collection( request_.name(), request_.dimension() );
            status_ = status_to_grpc_status( _status );
          }
          catch ( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
            status_ = grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" );
          }
          logger_->info< int >( "Create collection response: code {}", status_.error_code() );
          responder.Finish( response_, status_, this );
          state_ = state::PROCESSED;
        } );
  }
};

// DeleteCollection unary
struct server::delete_collection_handler : public rpc_base< delete_collection_handler, DeleteCollectionRequest, EmptyResponse >
{
  grpc::ServerAsyncResponseWriter< EmptyResponse > responder;

  delete_collection_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , responder( &server_ctx_ )
  {
    service_->RequestDeleteCollection( &server_ctx_, &request_, &responder, cq_, cq_, this );
  }

  void handle_unknown_error() override
  {
    responder.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
    state_ = state::PROCESSED;
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]
        {
          try
          {
            const auto _status = db_ptr_->delete_collection( request_.collectionname() );
            status_ = status_to_grpc_status( _status );
          }
          catch ( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
            status_ = grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" );
          }
          responder.Finish( response_, status_, this );
          state_ = state::PROCESSED;
        } );
  }
};

// Upsert unary
struct server::upsert_handler : public rpc_base< upsert_handler, UpsertRequest, EmptyResponse >
{
  grpc::ServerAsyncResponseWriter< EmptyResponse > responder;

  upsert_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , responder( &server_ctx_ )
  {
    service_->RequestUpsert( &server_ctx_, &request_, &responder, cq_, cq_, this );
  }

  void handle_unknown_error() override
  {
    responder.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
    state_ = state::PROCESSED;
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]
        {
          try
          {
            std::vector< std::pair< id_t, float_vector > > _vectors;
            _vectors.reserve( request_.vectors_size() );
            for ( auto& _vector : request_.vectors() )
            {
              float_vector _new_vector{ _vector.values_size(), _vector.values().data() };
              if ( _vector.has_metadata() )
                for ( auto& [ k, v ] : _vector.metadata().map() )
                  _new_vector.add_metadata( k, v );
              _vectors.emplace_back( _vector.id(), std::move( _new_vector ) );
            }
            const auto _status = db_ptr_->add_vectors( request_.collectionname(), _vectors );
            status_ = status_to_grpc_status( _status );
          }
          catch ( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
            status_ = grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" );
          }
          responder.Finish( response_, status_, this );
        } );
  }
};

// Search unary
struct server::search_handler : public rpc_base< search_handler, SearchRequest, SearchResponse >
{
  grpc::ServerAsyncResponseWriter< SearchResponse > responder;

  search_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , responder( &server_ctx_ )
  {
    service_->RequestSearch( &server_ctx_, &request_, &responder, cq_, cq_, this );
  }

  void handle_unknown_error() override
  {
    responder.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]()
        {
          grpc::Status _grpc_status;
          try
          {
            std::vector< score_pair > _result;
            float_vector _query{ request_.queryvector_size(), request_.queryvector().data() };
            const auto _status = db_ptr_->get_nearest_k( request_.collectionname(), _query, request_.top_k(), _result );
            _grpc_status = status_to_grpc_status( _status );
            for ( auto& [ score, _id_vector ] : _result )
            {
              auto& [ _id, _vector_ptr ] = _id_vector;
              auto _new_result = response_.add_results();
              _new_result->set_score( static_cast< float >( score ) );
              auto _new_vector = _new_result->mutable_vector();
              _new_vector->set_id( _id );
              for ( size_t it = 0; it < _vector_ptr->dimension_; ++it )
                _new_vector->add_values( _vector_ptr->data_[ it ] );
            }
          }
          catch ( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
          }

          responder.Finish( response_, _grpc_status, this );
          state_ = state::PROCESSED;
        } );
  }
};

// StreamUpsert client-streaming -> single response
struct server::stream_upsert_handler : public rpc_base< stream_upsert_handler, UpsertRequest, EmptyResponse >
{
  grpc::ServerAsyncReader< EmptyResponse, UpsertRequest > reader;

  grpc::Status status_{ grpc::StatusCode::INTERNAL, "Internal error" };

  stream_upsert_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , reader( &server_ctx_ )
  {
    service_->RequestStreamUpsert( &server_ctx_, &reader, cq_, cq_, this );
  }

  void handle_unknown_error() override
  {
    reader.Finish( response_, status_, this );
    state_ = state::PROCESSED;
  }

  bool do_read() override
  {
    reader.Read( &request_, this );
    state_ = state::READ;
    return false;
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]()
        {
          try
          {
            std::vector< std::pair< id_t, float_vector > > _vectors;
            _vectors.reserve( request_.vectors_size() );
            for ( auto& _vector : request_.vectors() )
            {
              float_vector _new_vector{ _vector.values_size(), _vector.values().data() };
              if ( _vector.has_metadata() )
                for ( auto& [ k, v ] : _vector.metadata().map() )
                  _new_vector.add_metadata( k, v );
              _vectors.emplace_back( _vector.id(), std::move( _new_vector ) );
            }
            const auto _status = db_ptr_->add_vectors( request_.collectionname(), _vectors );
            status_ = status_to_grpc_status( _status );
            if ( _status != status::success )
            {
              reader.Finish( response_, status_, this );
              state_ = state::PROCESSED;
            }
            else
            {
              status_ = grpc::Status::OK;
              do_read();
            }
          }
          catch ( std::exception& e )
          {
            std::cerr << e.what() << std::endl;
          }
        } );
  }
};

struct server::delete_vector_handler : public rpc_base< delete_vector_handler, DelVectorRequest, EmptyResponse >
{
  grpc::ServerAsyncResponseWriter< EmptyResponse > reader;
  grpc::Status status_{};

  delete_vector_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , reader( &server_ctx_ )
  {
  }

  void handle_unknown_error() override
  {
    state_ = state::PROCESSED;
    reader.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]()
        {
          try
          {
            std::vector< id_t > _ids;
            for ( const auto _id : request_.id() )
            {
              _ids.push_back( _id );
            }
            auto _status = db_ptr_->delete_vectors( request_.collection_name(), _ids );
            status_ = status_to_grpc_status( _status );
            reader.Finish( response_, status_, this );
            state_ = state::PROCESSED;
          }
          catch ( std::exception& e )
          {
            handle_unknown_error();
            std::cerr << e.what() << std::endl;
          }
        } );
  }
};

struct server::add_index_handler : public rpc_base< add_index_handler, AddIndexRequest, EmptyResponse >
{
  grpc::ServerAsyncResponseWriter< EmptyResponse > reader;
  grpc::Status status_{};

  add_index_handler( vectorService::AsyncService* s, grpc::ServerCompletionQueue* q, database* db, worker_pool* pool )
      : rpc_base( s, q, db, pool )
      , reader( &server_ctx_ )
  {
  }

  void handle_unknown_error() override
  {
    state_ = state::PROCESSED;
    reader.Finish( response_, grpc::Status( grpc::StatusCode::INTERNAL, "Internal error" ), this );
  }

  void process() override
  {
    db_worker_pool_->submit(
        [ this ]()
        {
          try
          {
            const auto collection_name = request_.collectionname();
            const auto index_name = request_.indexname();
            switch ( const auto index_type = request_.index(); index_type )
            {
              case vector_db::IndexType::HNSW:
              {
                auto hnsw_params = indices::hnsw::params();
                if ( request_.has_hnswparams() )
                {
                  auto& req_params = request_.hnswparams();
                  hnsw_params = indices::hnsw::params{ proto_to_db_dist( req_params.distancetype() ),
                                                       static_cast< unsigned int >( req_params.m() ),
                                                       static_cast< unsigned int >( req_params.efconstruction() ),
                                                       static_cast< unsigned int >( req_params.efsearch() ) };
                }

                auto _status = db_ptr_->add_index( collection_name, index_name, index_type::hnsw, &hnsw_params );
                status_ = status_to_grpc_status( _status );
                reader.Finish( response_, status_, this );
                break;
              }
              case vector_db::IndexType::IVF_FLAT:
              default:
                break;
            }
          }
          catch ( std::exception& e )
          {
            handle_unknown_error();
            std::cerr << e.what() << std::endl;
          }
        } );
  }
};

server::server( std::string address, int _num_cq_threads )
    : address_( std::move( address ) )
{
  static init_logger logger( "server" );
  if ( _num_cq_threads != 0 )
    num_of_thread_ = _num_cq_threads;
  db_ptr_ = std::make_unique< database >();
  db_worker_pool_ = std::make_unique< worker_pool >( 10 );
  logger_ = spdlog::get( "server" );
}

template< typename... rpc >
void server::init_rpc_handlers()
{
  ( new rpc( &service_, cq_.get(), db_ptr_.get(), db_worker_pool_.get() ), ... );
}

void server::start()
{
  if ( running_ )
    return;
  grpc::ServerBuilder builder;
  builder.AddListeningPort( address_, grpc::InsecureServerCredentials() );
  builder.RegisterService( &service_ );
  cq_ = builder.AddCompletionQueue();
  server_ = builder.BuildAndStart();
  running_ = true;

  init_rpc_handlers< create_collection_handler,
                     delete_collection_handler,
                     upsert_handler,
                     search_handler,
                     stream_upsert_handler,
                     delete_vector_handler,
                     add_index_handler >();

  // Start CQ polling thread
  logger_->info( "Starting {} Completion Queue worker threads", num_of_thread_ );
  for ( auto i = 0; i < num_of_thread_; ++i )
  {
    std::thread _thread( [ this ] { cq_poll_loop(); } );
    cq_threads_.push_back( std::move( _thread ) );
  }
  logger_->info( "Server started, listening on {}", address_ );
}

void server::shutdown()
{
  if ( !running_ )
    return;
  running_ = false;
  if ( db_worker_pool_ )
    db_worker_pool_->shutdown();
  if ( server_ )
    server_->Shutdown();
  if ( cq_ )
    cq_->Shutdown();
  for ( auto& _thread : cq_threads_ )
    if ( _thread.joinable() )
      _thread.join();
  logger_->info( "Server shutdown" );
}

void server::cq_poll_loop() const
{
  void* _tag = nullptr;
  bool _ok = false;
  while ( true )
  {
    if ( const bool _got = cq_->Next( &_tag, &_ok ); !_got )
      break;  // CQ shutdown
    static_cast< rpc_facade* >( _tag )->run_flow( _ok, running_ == false );
  }
}

void server::attach() const { cq_poll_loop(); }

}  // namespace vector_db