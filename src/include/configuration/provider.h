//
// Created by Vivek Yamsani on 06/02/26.
//
#pragma once

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "core/utils/util.h"

namespace vector_db
{

// Forward declaration to hide implementation details
namespace details
{
class config_provider_impl;
}

/// Configuration provider interface that abstracts the underlying TOML parser
/// Usage:
///   config_provider config;
///   config.load("/path/to/config.toml");
///   auto value = config.get_string("server", "host");
///   auto port = config.get_int("server", "port").value_or(8080);
class config_provider : public utils::singleton< config_provider >
{
  friend utils::singleton< config_provider >;
  config_provider();
  explicit config_provider( const std::filesystem::path& config_path );

public:
  ~config_provider();


  /// Load configuration from a file
  /// @param config_path Path to the TOML configuration file
  /// @throws std::runtime_error if file cannot be read or parsed
  void load( const std::filesystem::path& config_path );

  /// Check if configuration has been loaded
  /// @return true if configuration is loaded and ready
  bool is_loaded() const;

  /// Get a string value from the configuration
  /// @param keys Hierarchical keys to navigate the config (e.g., "server", "host")
  /// @return Optional string value, empty if not found
  std::optional< std::string > get_string( const std::vector< std::string >& keys ) const;

  /// Get an integer value from the configuration
  /// @param keys Hierarchical keys to navigate the config
  /// @return Optional integer value, empty if not found
  std::optional< int64_t > get_int( const std::vector< std::string >& keys ) const;

  /// Get a floating point value from the configuration
  /// @param keys Hierarchical keys to navigate the config
  /// @return Optional double value, empty if not found
  std::optional< double > get_double( const std::vector< std::string >& keys ) const;

  /// Get a boolean value from the configuration
  /// @param keys Hierarchical keys to navigate the config
  /// @return Optional boolean value, empty if not found
  std::optional< bool > get_bool( const std::vector< std::string >& keys ) const;

  /// Get a string array from the configuration
  /// @param keys Hierarchical keys to navigate the config
  /// @return Optional vector of strings, empty if not found
  std::optional< std::vector< std::string > > get_string_array( const std::vector< std::string >& keys ) const;

  /// Get an integer array from the configuration
  /// @param keys Hierarchical keys to navigate the config
  /// @return Optional vector of integers, empty if not found
  std::optional< std::vector< int64_t > > get_int_array( const std::vector< std::string >& keys ) const;

  // Variadic convenience overloads for multiple keys
  template< typename... Keys, std::enable_if_t< ( std::is_convertible_v< Keys, std::string > && ... ), int > = 0 >
  std::optional< std::string > get_string( Keys&&... keys ) const
  {
    return get_string( std::vector< std::string >{ std::string( std::forward< Keys >( keys ) )... } );
  }

  template< typename... Keys, std::enable_if_t< ( std::is_convertible_v< Keys, std::string > && ... ), int > = 0 >
  std::optional< int64_t > get_int( Keys&&... keys ) const
  {
    return get_int( std::vector< std::string >{ std::string( std::forward< Keys >( keys ) )... } );
  }

  template< typename... Keys, std::enable_if_t< ( std::is_convertible_v< Keys, std::string > && ... ), int > = 0 >
  std::optional< double > get_double( Keys&&... keys ) const
  {
    return get_double( std::vector< std::string >{ std::string( std::forward< Keys >( keys ) )... } );
  }

  template< typename... Keys, std::enable_if_t< ( std::is_convertible_v< Keys, std::string > && ... ), int > = 0 >
  std::optional< bool > get_bool( Keys&&... keys ) const
  {
    return get_bool( std::vector< std::string >{ std::string( std::forward< Keys >( keys ) )... } );
  }

  /// Check if a key exists in the configuration
  /// @param keys Hierarchical keys to check
  /// @return true if the key path exists
  bool has_key( const std::vector< std::string >& keys ) const;

  template< typename... Keys, std::enable_if_t< ( std::is_convertible_v< Keys, std::string > && ... ), int > = 0 >
  bool has_key( Keys&&... keys ) const
  {
    return has_key( std::vector< std::string >{ std::string( std::forward< Keys >( keys ) )... } );
  }

  /// Get the path of the currently loaded configuration file
  /// @return Path to the config file, empty if not loaded
  std::filesystem::path get_config_path() const;

private:
  std::unique_ptr< details::config_provider_impl > impl_;
};

}  // namespace vector_db
