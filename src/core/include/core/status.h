//
// Created by Vivek Yamsani on 18/12/25.
//
#pragma once
namespace vector_db
{
enum class status : uint8_t
{
  success = 0,
  collection_already_exists = 1,
  collection_does_not_exist = 2,
  dimension_cant_be_zero = 3,
  collection_name_cant_be_empty = 4,
  collection_name_too_long = 5,
  collection_name_invalid_characters = 6,
  vector_dimension_mismatch = 7,
  internal_error = 255
};


inline std::string status_to_string( const status s )
{
  switch ( s )
  {
    case status::success:
      return "success";
    case status::collection_already_exists:
      return "collection already exists";
    case status::collection_does_not_exist:
      return "collection does not exist";
    case status::dimension_cant_be_zero:
      return "dimension cant be zero";
    case status::collection_name_cant_be_empty:
      return "collection name cant be empty";
    case status::collection_name_too_long:
      return "collection name is too long";
    case status::collection_name_invalid_characters:
      return "collection name contains invalid characters";
    case status::vector_dimension_mismatch:
      return "vector dimension mismatch with collection";
    case status::internal_error:
      return "internal error";
  }
  return "unknown error";
}

inline std::ostream& operator<<( std::ostream& os, const status& s )
{
  os << status_to_string( s );
  return os;
}

}  // namespace vector_db