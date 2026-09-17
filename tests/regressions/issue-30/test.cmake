# https://github.com/noirotm/flvmeta/issues/30
# Reject a string-valued onMetaData before attempting to walk its properties.
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

set(TEST_CONTEXT "issue-30 / string metadata")
generate_fixture(input)

foreach(format raw json xml)
  set(TEST_CONTEXT "issue-30 / string metadata / ${format} check")
  assert_flvmeta_exit_code(9 check_output --check --${format} "${input}")
  # The missing media streams also cause errors. Require the type diagnostic
  # so an unrelated rejection cannot satisfy this test.
  assert_contains("${check_output}" "E70046")
  assert_contains("${check_output}"
    "invalid onMetaData data type: 2, should be an associative array (8)")

  # The report must reach its closing delimiter despite the rejected metadata.
  if(format STREQUAL "json")
    assert_matches("${check_output}" "}[\r\n ]*$")
  elseif(format STREQUAL "xml")
    assert_matches("${check_output}" "</report>[\r\n ]*$")
  endif()
endforeach()

remove_fixtures("${input}")
