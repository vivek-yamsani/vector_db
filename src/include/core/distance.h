//
// Created by Vivek Yamsani on 02/02/26.
//
#pragma once
#include "float_vector.h"
#include "utils/util.h"

namespace vector_db::distance
{

enum class dist_type : uint8_t
{
  cosine = 0,
  euclidean = 1,
  inner_product = 2,
  unknown = 255
};

struct distance_t
{
  virtual double compute( const float_vector& a, const float_vector& b ) = 0;
  virtual ~distance_t() = default;
};

using ptr = distance_t*;

struct euclidean
    : distance_t
    , utils::singleton< euclidean >
{
  friend utils::singleton< euclidean >;
  double compute( const float_vector& a, const float_vector& b ) override
  {
    double distance = 0.0;
    for ( unsigned int i = 0; i < static_cast< unsigned int >( a.dimension_ ); ++i )
    {
      const double diff = static_cast< double >( a.data_[ i ] ) - static_cast< double >( b.data_[ i ] );
      distance += diff * diff;
    }
    return std::sqrt( distance );
  }
};

struct cosine
    : distance_t
    , utils::singleton< cosine >
{
  friend utils::singleton< cosine >;
  double compute( const float_vector& a, const float_vector& b ) override
  {
    double dot_product = 0.0, mag_a = 0.0, mag_b = 0.0;
    for ( unsigned int i = 0; i < static_cast< unsigned int >( a.dimension_ ); ++i )
    {
      const auto av = static_cast< double >( a.data_[ i ] );
      const auto bv = static_cast< double >( b.data_[ i ] );
      dot_product += av * bv;
      mag_a += av * av;
      mag_b += bv * bv;
    }
    mag_a = std::sqrt( mag_a );
    mag_b = std::sqrt( mag_b );
    if ( mag_a == 0.0 || mag_b == 0.0 )
    {
      return 1.0;
    }
    return 1.0 - ( dot_product / ( mag_a * mag_b ) );
  }
};

struct inner_product
    : distance_t
    , utils::singleton< inner_product >
{
  friend utils::singleton< inner_product >;
  double compute( const float_vector& a, const float_vector& b ) override
  {
    double dot_product = 0.0;
    for ( unsigned int i = 0; i < static_cast< unsigned int >( a.dimension_ ); ++i )
      dot_product += static_cast< double >( a.data_[ i ] ) * static_cast< double >( b.data_[ i ] );
    return dot_product;
  }
};

inline distance_t* get_distance_instance( const dist_type type_ )
{
  switch ( type_ )
  {
    case dist_type::cosine:
      return cosine::get_instance();
    case dist_type::euclidean:
      return euclidean::get_instance();
    case dist_type::inner_product:
      return inner_product::get_instance();
    default:
      return nullptr;
  }
}

}  // namespace vector_db::distance