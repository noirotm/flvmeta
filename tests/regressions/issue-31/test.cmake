# https://github.com/noirotm/flvmeta/issues/31
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

set(TEST_CONTEXT "issue-31 / 47-zero prefix")
generate_fixture(input)
set(TEST_CONTEXT "issue-31 / 47-zero prefix / check")
assert_flvmeta_exit_code(9 check_output --check "${input}")
# Missing width/height metadata gives exit 9. These diagnostics occur after
# AVC decoding, so an earlier, unrelated rejection cannot pass the test.
assert_contains("${check_output}" "E60077")
assert_contains("${check_output}" "E60078")
remove_fixtures("${input}")
