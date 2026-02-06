//
// Created by Vivek Yamsani on 06/02/26.
//
#include "configuration/provider.h"

#include <stdexcept>
#include <toml.hpp>

namespace vector_db
{

namespace details
{

/// Implementation class that encapsulates toml11 details
class config_provider_impl
{
public:
  config_provider_impl() = default;

  void load( const std::filesystem::path& config_path )
  {
    if ( !std::filesystem::exists( config_path ) )
    {
      throw std::runtime_error( "Configuration file not found: " + config_path.string() );
    }

    try
    {
      data_ = toml::parse( config_path.string() );
      config_path_ = config_path;
      loaded_ = true;
    }
    catch ( const toml::syntax_error& err )
    {
      throw std::runtime_error( "Failed to parse TOML file: " + std::string( err.what() ) );
    }
    catch ( const std::exception& err )
    {
      throw std::runtime_error( "Failed to load configuration: " + std::string( err.what() ) );
    }
  }

  bool is_loaded() const { return loaded_; }

  std::filesystem::path get_config_path() const { return config_path_; }

  template< typename T >
  std::optional< T > get_value( const std::vector< std::string >& keys ) const
  {
    if ( !loaded_ || keys.empty() )
    {
      return std::nullopt;
    }

    try
    {
      const toml::value* current = &data_;

      // Navigate through the hierarchy
      for ( size_t i = 0; i < keys.size() - 1; ++i )
      {
        if ( !current->is_table() )
        {
          return std::nullopt;
        }
        const auto& table = current->as_table();
        auto it = table.find( keys[ i ] );
        if ( it == table.end() )
        {
          return std::nullopt;
        }
        current = &it->second;
      }

      // Get the final value
      if ( !current->is_table() )
      {
        return std::nullopt;
      }
      const auto& table = current->as_table();
      auto it = table.find( keys.back() );
      if ( it == table.end() )
      {
        return std::nullopt;
      }

      return toml::get< T >( it->second );
    }
    catch ( ... )
    {
      // Silently return nullopt on error - caller can handle missing values
      return std::nullopt;
    }
  }

  template< typename T >
  std::optional< std::vector< T > > get_array( const std::vector< std::string >& keys ) const
  {
    if ( !loaded_ || keys.empty() )
    {
      return std::nullopt;
    }

    try
    {
      const toml::value* current = &data_;

      // Navigate through the hierarchy
      for ( size_t i = 0; i < keys.size() - 1; ++i )
      {
        if ( !current->is_table() )
        {
          return std::nullopt;
        }
        const auto& table = current->as_table();
        auto it = table.find( keys[ i ] );
        if ( it == table.end() )
        {
          return std::nullopt;
        }
        current = &it->second;
      }

      // Get the final value
      if ( !current->is_table() )
      {
        return std::nullopt;
      }
      const auto& table = current->as_table();
      auto it = table.find( keys.back() );
      if ( it == table.end() )
      {
        return std::nullopt;
      }

      if ( !it->second.is_array() )
      {
        return std::nullopt;
      }

      const auto& arr = it->second.as_array();
      std::vector< T > result;
      result.reserve( arr.size() );

      for ( const auto& elem : arr )
      {
        result.push_back( toml::get< T >( elem ) );
      }

      return result;
    }
    catch ( ... )
    {
      // Silently return nullopt on error - caller can handle missing values
      return std::nullopt;
    }
  }

  bool has_key( const std::vector< std::string >& keys ) const
  {
    if ( !loaded_ || keys.empty() )
    {
      return false;
    }

    try
    {
      const toml::value* current = &data_;

      for ( const auto& key : keys )
      {
        if ( !current->is_table() )
        {
          return false;
        }
        const auto& table = current->as_table();
        auto it = table.find( key );
        if ( it == table.end() )
        {
          return false;
        }
        current = &it->second;
      }

      return true;
    }
    catch ( ... )
    {
      // Silently return false on error
      return false;
    }
  }

private:
  toml::value data_;
  std::filesystem::path config_path_;
  bool loaded_ = false;
};

}  // namespace details

// config_provider implementation

config_provider::config_provider()
    : impl_( std::make_unique< details::config_provider_impl >() )
{
}

config_provider::config_provider( const std::filesystem::path& config_path )
    : impl_( std::make_unique< details::config_provider_impl >() )
{
  load( config_path );
}

config_provider::~config_provider() = default;

void config_provider::load( const std::filesystem::path& config_path ) { impl_->load( config_path ); }

bool config_provider::is_loaded() const { return impl_->is_loaded(); }

std::filesystem::path config_provider::get_config_path() const { return impl_->get_config_path(); }

// Base getters (called by variadic templates in header)
std::optional< std::string > config_provider::get_string( const std::vector< std::string >& keys ) const
{
  return impl_->get_value< std::string >( keys );
}

std::optional< int64_t > config_provider::get_int( const std::vector< std::string >& keys ) const
{
  return impl_->get_value< int64_t >( keys );
}

std::optional< double > config_provider::get_double( const std::vector< std::string >& keys ) const
{
  return impl_->get_value< double >( keys );
}

std::optional< bool > config_provider::get_bool( const std::vector< std::string >& keys ) const
{
  return impl_->get_value< bool >( keys );
}

// Array getters
std::optional< std::vector< std::string > > config_provider::get_string_array( const std::vector< std::string >& keys ) const
{
  return impl_->get_array< std::string >( keys );
}

std::optional< std::vector< int64_t > > config_provider::get_int_array( const std::vector< std::string >& keys ) const
{
  return impl_->get_array< int64_t >( keys );
}

// has_key implementation
bool config_provider::has_key( const std::vector< std::string >& keys ) const { return impl_->has_key( keys ); }

}  // namespace vector_db
