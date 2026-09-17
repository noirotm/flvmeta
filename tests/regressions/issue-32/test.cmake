# https://github.com/noirotm/flvmeta/issues/32
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

function(check_delay name expected_delay_seconds)
  set(TEST_CONTEXT "issue-32 / ${name}")
  generate_fixture(input "${name}")

  set(TEST_CONTEXT "issue-32 / ${name} / check")
  # Missing width/height metadata gives exit 9 independently of audiodelay.
  assert_flvmeta_exit_code(9 check_output --check "${input}")
  assert_contains("${check_output}" "W80062")
  assert_contains("${check_output}"
    "audiodelay should be ${expected_delay_seconds}, got 42")

  set(TEST_CONTEXT "issue-32 / ${name} / update")
  set(rewritten "${input}.updated.flv")
  assert_flvmeta_exit_code(0 update_output --update "${input}" "${rewritten}")

  set(TEST_CONTEXT "issue-32 / ${name} / generated audiodelay (seconds)")
  assert_flvmeta_exit_code(0 metadata_output --dump --json "${rewritten}")
  string(REGEX MATCH [["audiodelay" *: *([-+0-9.eE]+)]] delay_field "${metadata_output}")
  set(actual_delay_seconds "${CMAKE_MATCH_1}")
  assert_contains("${metadata_output}" [["audiodelay"]])
  assert_equal("${actual_delay_seconds}" "${expected_delay_seconds}")
  remove_fixtures("${input}" "${rewritten}")
endfunction()

check_delay(positive                      2147483.393)
check_delay(negative                     -2147483.393)
check_delay(cross-sign-boundary-positive  0.001)
check_delay(cross-sign-boundary-negative -0.001)
check_delay(maximum                       4294967.295)
check_delay(minimum                      -4294967.295)
check_delay(zero                          0)
check_delay(ordinary                      1)
