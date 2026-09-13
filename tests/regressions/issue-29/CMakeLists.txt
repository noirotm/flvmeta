# https://github.com/noirotm/flvmeta/issues/29
add_flvmeta_regression(29)
target_sources(issue_29_fixture PRIVATE
  ${CMAKE_SOURCE_DIR}/tests/util.c
  ${CMAKE_SOURCE_DIR}/tests/util.h)
target_include_directories(issue_29_fixture PRIVATE
  ${CMAKE_SOURCE_DIR}/tests ${CMAKE_SOURCE_DIR})
