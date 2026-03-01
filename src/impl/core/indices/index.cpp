//
// Implementation for index_t base class
//
#include "core/indices/index.h"
#include "core/indices/hnsw.h"
#include "core/indices/ivfflat.h"

namespace vector_db
{

std::unique_ptr< index_t > index_t::deserialize( std::istream& is, const std::weak_ptr< collection >& col_ptr )
{
  index_type type;
  is.read( reinterpret_cast< char* >( &type ), sizeof( type ) );

  if ( type == index_type::hnsw )
  {
    auto params = indices::hnsw::params::deserialize( is );
    auto index = std::make_unique< indices::hnsw::index >( col_ptr, params );
    index->init();
    return index;
  }
  else if ( type == index_type::ivf_flat )
  {
    auto params = indices::ivf_flat::params::deserialize( is );
    auto index = std::make_unique< indices::ivf_flat::index >( col_ptr, params );
    index->init();
    return index;
  }

  return nullptr;
}

}  // namespace vector_db
