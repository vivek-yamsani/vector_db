//
// Created by Vivek Yamsani on 13/12/25.
//
#pragma once
#include <chrono>

namespace vector_db
{
// https://codeforces.com/blog/entry/62393
struct hash
{
  static uint64_t splitmix64( uint64_t x )
  {
    x += 0x9e3779b97f4a7c15ULL;
    x = ( x ^ ( x >> 30 ) ) * 0xbf58476d1ce4e5b9ULL;
    x = ( x ^ ( x >> 27 ) ) * 0x94d049bb133111ebULL;
    return x ^ ( x >> 31 );
  }
  size_t operator()( uint64_t x ) const
  {
    static const uint64_t FIXED_RANDOM = static_cast< uint64_t >( std::chrono::steady_clock::now().time_since_epoch().count() );
    return static_cast< size_t >( splitmix64( x + FIXED_RANDOM ) );
  }
};
}  // namespace vector_db
