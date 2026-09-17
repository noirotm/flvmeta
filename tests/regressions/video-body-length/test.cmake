include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

foreach(mode avc header-only)
  set(TEST_CONTEXT "video-body-length / ${mode}")
  generate_fixture(input "${mode}")

  set(TEST_CONTEXT "video-body-length / ${mode} / check")
  # Missing width/height metadata gives exit 9. Require the corresponding
  # diagnostics to establish that file information was computed successfully.
  assert_flvmeta_exit_code(9 check_output --check "${input}")
  assert_contains("${check_output}" "E60077")
  assert_contains("${check_output}" "E60078")

  set(TEST_CONTEXT "video-body-length / ${mode} / update")
  set(rewritten "${input}.updated.flv")
  assert_flvmeta_exit_code(0 update_output --update "${input}" "${rewritten}")

  set(TEST_CONTEXT "video-body-length / ${mode} / generated dimensions")
  assert_flvmeta_exit_code(0 metadata_output --dump --json "${rewritten}")
  if(mode STREQUAL "avc")
    assert_matches("${metadata_output}" [["width" *: *320]])
    assert_matches("${metadata_output}" [["height" *: *240]])
  else()
    string(REGEX MATCH [["(width|height)" *:]] dimension_field "${metadata_output}")
    assert_equal("${dimension_field}" "")
  endif()
  remove_fixtures("${input}" "${rewritten}")
endforeach()
