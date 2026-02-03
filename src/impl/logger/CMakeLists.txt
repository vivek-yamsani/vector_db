project(logger)

add_library(logger INTERFACE logger.h)

target_include_directories(logger INTERFACE .)