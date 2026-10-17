# https://github.com/noirotm/flvmeta/issues/33
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

# Date formatting uses the process timezone; fix it for valid-date assertions.
set(ENV{TZ} "UTC0")

function(check_date name expected_date)
  set(TEST_CONTEXT "issue-33 / ${name}")
  generate_fixture(input "${name}")

  set(TEST_CONTEXT "issue-33 / ${name} / default XML")
  assert_flvmeta_exit_code(0 metadata_output "${input}")
  assert_contains("${metadata_output}" "value=\"${expected_date}\"")

  foreach(format xml json yaml raw)
    foreach(command --dump --full-dump)
      set(TEST_CONTEXT "issue-33 / ${name} / ${command} ${format}")
      assert_flvmeta_exit_code(0 metadata_output ${command} "--${format}" "${input}")
      assert_contains("${metadata_output}" "${expected_date}")
    endforeach()
  endforeach()
  remove_fixtures("${input}")
endfunction()

foreach(name nan negative-nan infinity negative-infinity maximum minimum signed-limit)
  check_date("${name}" "0000-00-00T00:00:00")
endforeach()
check_date(epoch "1970-01-01T00:00:00")
check_date(ordinary "1970-01-01T00:00:01")
