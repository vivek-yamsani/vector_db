//
// Implementation for vector_db::vector
//
#include "core/float_vector.h"

namespace vector_db
{
float_vector::float_vector()
    : data_( nullptr )
    , dimension_( 0 )
{
}

float_vector::float_vector( const int dimension, const float* data )
{
  dimension_ = dimension;
  data_ = std::make_unique< float[] >( dimension );
  std::memcpy( data_.get(), data, static_cast< size_t >( dimension ) * sizeof( float ) );
}

float_vector::float_vector( const float_vector& other )
{
  dimension_ = other.dimension_;
  data_ = std::make_unique< float[] >( dimension_ );
  std::memcpy( data_.get(), other.data_.get(), static_cast< size_t >( dimension_ ) * sizeof( float ) );
  if ( other.metadata_ )
    metadata_ = std::make_unique< std::vector< std::pair< std::string, std::string > > >( other.metadata_->begin(),
                                                                                          other.metadata_->end() );
}

float_vector& float_vector::operator=( const float_vector& other )
{
  if ( this == &other )
    return *this;
  dimension_ = other.dimension_;
  data_ = std::make_unique< float[] >( dimension_ );
  std::memcpy( data_.get(), other.data_.get(), static_cast< size_t >( dimension_ ) * sizeof( float ) );
  if ( other.metadata_ )
    metadata_ = std::make_unique< std::vector< std::pair< std::string, std::string > > >( other.metadata_->begin(),
                                                                                          other.metadata_->end() );
  return *this;
}

float_vector::float_vector( float_vector&& other ) noexcept
{
  dimension_ = other.dimension_;
  data_ = std::move( other.data_ );
  metadata_ = std::move( other.metadata_ );
}

float_vector& float_vector::operator=( float_vector&& other ) noexcept
{
  if ( this == &other )
    return *this;
  dimension_ = other.dimension_;
  data_ = std::move( other.data_ );
  metadata_ = std::move( other.metadata_ );
  return *this;
}

bool float_vector::operator==( const float_vector& other ) const
{
  return dimension_ == other.dimension_ && std::memcmp( data_.get(), other.data_.get(), dimension_ * sizeof( float ) ) == 0;
}

void float_vector::add_metadata( const std::string& key, const std::string& value )
{
  if ( !metadata_ )
    metadata_ = std::make_unique< std::vector< std::pair< std::string, std::string > > >();
  metadata_->emplace_back( key, value );
}

void float_vector::serialize( std::ostream& os ) const
{
  os.write( reinterpret_cast< const char* >( &dimension_ ), sizeof( dimension_ ) );
  if ( dimension_ > 0 )
  {
    os.write( reinterpret_cast< const char* >( data_.get() ), static_cast< std::streamsize >( dimension_ ) * sizeof( float ) );
  }

  bool has_metadata = ( metadata_ != nullptr );
  os.write( reinterpret_cast< const char* >( &has_metadata ), sizeof( has_metadata ) );
  if ( has_metadata )
  {
    uint32_t size = static_cast< uint32_t >( metadata_->size() );
    os.write( reinterpret_cast< const char* >( &size ), sizeof( size ) );
    for ( const auto& [ key, value ] : *metadata_ )
    {
      uint32_t key_len = static_cast< uint32_t >( key.length() );
      os.write( reinterpret_cast< const char* >( &key_len ), sizeof( key_len ) );
      os.write( key.data(), key_len );

      uint32_t val_len = static_cast< uint32_t >( value.length() );
      os.write( reinterpret_cast< const char* >( &val_len ), sizeof( val_len ) );
      os.write( value.data(), val_len );
    }
  }
}

float_vector float_vector::deserialize( std::istream& is )
{
  int dimension;
  is.read( reinterpret_cast< char* >( &dimension ), sizeof( dimension ) );
  float_vector vec;
  vec.dimension_ = dimension;
  if ( dimension > 0 )
  {
    vec.data_ = std::make_unique< float[] >( dimension );
    is.read( reinterpret_cast< char* >( vec.data_.get() ), static_cast< std::streamsize >( dimension ) * sizeof( float ) );
  }

  bool has_metadata;
  is.read( reinterpret_cast< char* >( &has_metadata ), sizeof( has_metadata ) );
  if ( has_metadata )
  {
    uint32_t size;
    is.read( reinterpret_cast< char* >( &size ), sizeof( size ) );
    vec.metadata_ = std::make_unique< std::vector< std::pair< std::string, std::string > > >();
    vec.metadata_->reserve( size );
    for ( uint32_t i = 0; i < size; ++i )
    {
      uint32_t key_len;
      is.read( reinterpret_cast< char* >( &key_len ), sizeof( key_len ) );
      std::string key( key_len, '\0' );
      is.read( key.data(), key_len );

      uint32_t val_len;
      is.read( reinterpret_cast< char* >( &val_len ), sizeof( val_len ) );
      std::string value( val_len, '\0' );
      is.read( value.data(), val_len );

      vec.metadata_->emplace_back( std::move( key ), std::move( value ) );
    }
  }
  return vec;
}
}  // namespace vector_db
