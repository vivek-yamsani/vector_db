//
// Created by Vivek Yamsani on 06/02/26.
//
#include "logger/logger.h"

#include <algorithm>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <unordered_map>

#include "spdlog/async.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/spdlog.h"

namespace vector_db
{

namespace
{
// Constants
constexpr size_t BYTES_PER_MB = 1048576;

// Convert our log_level to spdlog level
spdlog::level::level_enum to_spdlog_level( log_level level )
{
  switch ( level )
  {
    case log_level::trace:
      return spdlog::level::trace;
    case log_level::debug:
      return spdlog::level::debug;
    case log_level::info:
      return spdlog::level::info;
    case log_level::warn:
      return spdlog::level::warn;
    case log_level::error:
      return spdlog::level::err;
    case log_level::critical:
      return spdlog::level::critical;
    case log_level::off:
      return spdlog::level::off;
    default:
      return spdlog::level::info;
  }
}

// Convert spdlog level to our log_level
log_level from_spdlog_level( spdlog::level::level_enum level )
{
  switch ( level )
  {
    case spdlog::level::trace:
      return log_level::trace;
    case spdlog::level::debug:
      return log_level::debug;
    case spdlog::level::info:
      return log_level::info;
    case spdlog::level::warn:
      return log_level::warn;
    case spdlog::level::err:
      return log_level::error;
    case spdlog::level::critical:
      return log_level::critical;
    case spdlog::level::off:
      return log_level::off;
    default:
      return log_level::info;
  }
}

// Global state for logger factory
struct logger_factory_state
{
  std::mutex mutex;
  bool initialized = false;
  std::shared_ptr< spdlog::sinks::sink > sink;
};

logger_factory_state& get_factory_state()
{
  static logger_factory_state state;
  return state;
}

}  // anonymous namespace

// Parse log level from string
log_level parse_log_level( std::string_view level )
{
  std::string lower;
  lower.reserve( level.size() );
  std::transform( level.begin(), level.end(), std::back_inserter( lower ), []( unsigned char c ) { return std::tolower( c ); } );

  if ( lower == "trace" )
    return log_level::trace;
  if ( lower == "debug" )
    return log_level::debug;
  if ( lower == "info" )
    return log_level::info;
  if ( lower == "warn" || lower == "warning" )
    return log_level::warn;
  if ( lower == "error" || lower == "err" )
    return log_level::error;
  if ( lower == "critical" || lower == "crit" )
    return log_level::critical;
  if ( lower == "off" )
    return log_level::off;

  return log_level::info;  // Default
}

// Concrete implementation of logger_impl using spdlog
namespace details
{
class spdlog_logger_impl : public logger_impl
{
public:
  explicit spdlog_logger_impl( std::shared_ptr< spdlog::logger > logger )
      : logger_( std::move( logger ) )
  {
  }

  void trace( std::string_view msg ) override { logger_->trace( msg ); }

  void debug( std::string_view msg ) override { logger_->debug( msg ); }

  void info( std::string_view msg ) override { logger_->info( msg ); }

  void warn( std::string_view msg ) override { logger_->warn( msg ); }

  void error( std::string_view msg ) override { logger_->error( msg ); }

  void critical( std::string_view msg ) override { logger_->critical( msg ); }

  void set_level( log_level level ) override { logger_->set_level( to_spdlog_level( level ) ); }

  void set_level( std::string_view level ) override { set_level( parse_log_level( level ) ); }

  log_level get_level() const override { return from_spdlog_level( logger_->level() ); }

  std::string_view name() const override { return logger_->name(); }

  void* get_native_logger() { return logger_.get(); }

private:
  std::shared_ptr< spdlog::logger > logger_;
};

}  // namespace details

// logger_factory implementation

void logger_factory::initialize()
{
  auto& state = get_factory_state();
  std::lock_guard< std::mutex > lock( state.mutex );

  if ( state.initialized )
  {
    return;  // Already initialized
  }

  try
  {
    // Use hardcoded defaults - logger is foundational infrastructure
    // that should not depend on config_provider to avoid circular dependencies
    constexpr std::string_view log_dir = "logs";
    constexpr std::string_view log_filename = "vector_db.log";
    constexpr size_t max_file_size_mb = 30;
    constexpr size_t max_files = 3;
    constexpr size_t thread_pool_queue_size = 8192;
    constexpr size_t thread_pool_threads = 2;
    constexpr int flush_interval_seconds = 1;

    // Create log directory if it doesn't exist
    std::filesystem::path log_directory{ log_dir };
    if ( !std::filesystem::exists( log_directory ) )
    {
      std::filesystem::create_directories( log_directory );
    }

    // Initialize thread pool for async logging
    spdlog::init_thread_pool( thread_pool_queue_size, thread_pool_threads );

    // Create rotating file sink
    auto log_path = log_directory / log_filename;
    state.sink =
        std::make_shared< spdlog::sinks::rotating_file_sink_mt >( log_path.string(), max_file_size_mb * BYTES_PER_MB, max_files );

    // Set up automatic flushing
    spdlog::flush_every( std::chrono::seconds( flush_interval_seconds ) );

    state.initialized = true;
  }
  catch ( const std::exception& ex )
  {
    throw std::runtime_error( "Failed to initialize logger: " + std::string( ex.what() ) );
  }
}

bool logger_factory::is_initialized()
{
  auto& state = get_factory_state();
  std::lock_guard< std::mutex > lock( state.mutex );
  return state.initialized;
}

std::shared_ptr< details::logger_impl > logger_factory::create( std::string name, log_level level )
{
  auto& state = get_factory_state();
  std::lock_guard< std::mutex > lock( state.mutex );

  if ( !state.initialized )
  {
    throw std::runtime_error( "Logger factory not initialized. Call logger_factory::initialize() first." );
  }

  // Check if logger already exists
  auto existing = spdlog::get( name );
  if ( existing )
  {
    return std::make_shared< details::spdlog_logger_impl >( existing );
  }

  // Create new async logger
  auto logger = std::make_shared< spdlog::async_logger >(
      name, state.sink, spdlog::thread_pool(), spdlog::async_overflow_policy::discard_new );

  // Set pattern: [HH:MM:SS:ms][thread id][level][logger name] message
  logger->set_pattern( "[%H:%M:%S:%e][thread %t][%=8l][%=11n] %v" );

  // Set log level
  logger->set_level( to_spdlog_level( level ) );

  // Register logger
  spdlog::register_logger( logger );

  return std::make_shared< details::spdlog_logger_impl >( logger );
}

void logger_factory::flush_all()
{
  spdlog::apply_all( []( std::shared_ptr< spdlog::logger > logger ) { logger->flush(); } );
}

void logger_factory::shutdown()
{
  auto& state = get_factory_state();
  std::lock_guard< std::mutex > lock( state.mutex );

  if ( state.initialized )
  {
    spdlog::shutdown();
    state.sink.reset();
    state.initialized = false;
  }
}

}  // namespace vector_db
