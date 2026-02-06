//
// Created by Vivek Yamsani on 06/02/26.
//
#pragma once

#include <fmt/format.h>
#include <memory>
#include <string>
#include <string_view>

namespace vector_db
{

namespace details
{
class logger_impl;
}

/// Logging levels
enum class log_level
{
  trace,
  debug,
  info,
  warn,
  error,
  critical,
  off
};

/// Parse log level from string (case-insensitive)
/// @param level String representation (e.g., "debug", "info")
/// @return Corresponding log_level, defaults to info if invalid
log_level parse_log_level( std::string_view level );

/// Logger factory that manages logger creation and initialization
class logger_factory
{
public:
  /// Initialize the logging system from config_provider singleton
  /// Reads configuration from [logger] section in TOML
  /// @throws std::runtime_error if initialization fails
  static void initialize();

  /// Check if the logging system has been initialized
  /// @return true if initialized
  static bool is_initialized();

  /// Create or get a named logger instance
  /// @param name Logger name (used for identification in logs)
  /// @param level Log level for this logger (defaults to info)
  /// @return Shared pointer to logger implementation
  /// @throws std::runtime_error if not initialized
  static std::shared_ptr< details::logger_impl > create( std::string name, log_level level = log_level::info );

  /// Flush all loggers immediately
  static void flush_all();

  /// Shutdown the logging system (flushes and releases resources)
  static void shutdown();
};

/// Logger interface (PIMPL wrapper around spdlog)
namespace details
{

class logger_impl
{
public:
  virtual ~logger_impl() = default;

  // Simple message logging
  virtual void trace( std::string_view msg ) = 0;
  virtual void debug( std::string_view msg ) = 0;
  virtual void info( std::string_view msg ) = 0;
  virtual void warn( std::string_view msg ) = 0;
  virtual void error( std::string_view msg ) = 0;
  virtual void critical( std::string_view msg ) = 0;

  // Formatted message logging with variadic arguments
  // Formats the string using fmt, then logs it (no spdlog exposure)
  template< typename... Args >
  void trace( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    trace( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  template< typename... Args >
  void debug( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    debug( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  template< typename... Args >
  void info( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    info( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  template< typename... Args >
  void warn( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    warn( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  template< typename... Args >
  void error( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    error( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  template< typename... Args >
  void critical( fmt::format_string< Args... > fmt_str, Args&&... args )
  {
    critical( fmt::format( fmt_str, std::forward< Args >( args )... ) );
  }

  virtual void set_level( log_level level ) = 0;
  virtual void set_level( std::string_view level ) = 0;
  virtual log_level get_level() const = 0;

  virtual std::string_view name() const = 0;
};

}  // namespace details

}  // namespace vector_db
