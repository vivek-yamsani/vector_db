//
// Common utility types and helpers for vector_db
//
#pragma once

#include "core/float_vector.h"
#include "core/status.h"

namespace vector_db::utils
{

double get_euclidean_distance( const float_vector& a, const float_vector& b );

double get_cosine_distance( const float_vector& a, const float_vector& b );

status is_collection_name_valid( const std::string& collection_name );

template< typename Derived >
struct singleton
{
protected:
  singleton() = default;
  // NON-VIRTUAL, but PROTECTED.
  // 1. Protected: Prevents external code from calling 'delete basePtr'.
  // 2. Non-Virtual: Removes the vtable pointer, saving memory.
  ~singleton() = default;

public:
  singleton( const singleton& ) = delete;
  singleton& operator=( const singleton& ) = delete;

  singleton( singleton&& ) = delete;
  singleton& operator=( singleton&& ) = delete;

  static Derived* get_instance()
  {
    static Derived instance;
    return &instance;
  }
};

}  // namespace vector_db::utils
