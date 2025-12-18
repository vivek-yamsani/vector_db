//
// Created by Vivek Yamsani on 09/12/25.
//
#pragma once

#include "index.h"

namespace vector_db::indices::ivf_flat
{
class index : public index_t
{
  using index_t::wk_col_ptr;

public:
  explicit index( wk_col_ptr col_ptr );
};

}  // namespace vector_db::indices::ivf_flat