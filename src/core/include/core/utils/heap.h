//
// Created by Vivek Yamsani on 13/12/25.
//
#pragma once
#include <vector>

template< typename T, typename cmp, bool min >
struct heap
{
  static_assert( std::is_invocable_r_v< bool, cmp, const T&, const T& >,
                 "cmp must be callable as: bool cmp(const T&, const T&)" );

  std::vector< T > heap_;
  heap() = default;

  void push( const T& val )
  {
    heap_.push_back( val );
    size_t i = heap_.size() - 1;
    while ( i > 0 && cmp{}( heap_[ ( i - 1 ) / 2 ], heap_[ i ] ) == min )
    {
      std::swap( heap_[ i ], heap_[ ( i - 1 ) / 2 ] );
      i = ( i - 1 ) / 2;
    }
  }
};

using min_heap = heap< float, std::less<>, true >;
using max_heap = heap< float, std::greater<>, false >;
