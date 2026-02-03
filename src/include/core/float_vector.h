//
// Created by Vivek Yamsani on 07/12/25.
//
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace vector_db
{
struct float_vector
{
  std::unique_ptr< float[] > data_;
  std::unique_ptr< std::vector< std::pair< std::string, std::string > > > metadata_;
  int dimension_;

  float_vector();
  float_vector( int dimension, const float* data );
  float_vector( const float_vector& other );
  float_vector& operator=( const float_vector& other );
  float_vector( float_vector&& other ) noexcept;
  float_vector& operator=( float_vector&& other ) noexcept;

  bool operator==( const float_vector& other ) const;

  void add_metadata( const std::string& key, const std::string& value );
};

using id_t = unsigned long long;
using vector_ptr = std::unique_ptr< float_vector >;
using id_vector = std::pair< id_t, vector_ptr >;
using score_pair = std::pair< double, id_vector >;
}  // namespace vector_db
