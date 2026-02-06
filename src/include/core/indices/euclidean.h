//
// Created by Vivek Yamsani on 18/12/25.
//
#pragma once
#include "core/indices/index.h"
namespace vector_db::indices::euclidean
{

class index : public index_t
{
  using index_t::wk_col_ptr;

public:
  explicit index( const wk_col_ptr& col_ptr );
  bool search_for_top_k( const float_vector& query_vector, unsigned int k, std::vector< score_pair >& results ) override;
};

}  // namespace vector_db::indices::euclidean