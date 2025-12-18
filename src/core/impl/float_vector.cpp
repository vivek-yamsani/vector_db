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
}  // namespace vector_db
