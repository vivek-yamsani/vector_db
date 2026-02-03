//
// Created by Vivek Yamsani on 09/12/25.
//

#include "core/indices/ivfflat.h"

namespace vector_db::indices::ivf_flat
{

index::index( wk_col_ptr _collection_ptr )
    : index_t( std::move( _collection_ptr ) )
{
}



}  // namespace vector_db::indices::ivf_flat