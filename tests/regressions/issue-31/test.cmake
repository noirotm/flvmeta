# https://github.com/noirotm/flvmeta/issues/31
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

generate_fixture(input)
assert_flvmeta_exit_code(9 output --check "${input}")
# These diagnostics follow get_flv_info() and its AVC decoder. A different
# rejection, including the original PoC's invalid metadata, must not pass.
if(NOT output MATCHES "E60077" OR NOT output MATCHES "E60078")
  message(FATAL_ERROR "Checker did not reach metadata verification\n${output}")
endif()
remove_fixtures("${input}")
