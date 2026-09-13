# https://github.com/noirotm/flvmeta/issues/30
# Issue #30: a string-valued onMetaData must be rejected before list traversal.
# Run the actual checker, since the Unity executable does not link check.c.
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")
generate_fixture(input)

foreach(format raw json xml)
  assert_flvmeta_exit_code(9 output --check --${format} "${input}")
  # This metadata-only fixture also reports a no-streams header error. Require
  # the specific wrong-type diagnostic so that error alone cannot pass the test.
  if(NOT output MATCHES "E70046" OR
      NOT output MATCHES "invalid onMetaData data type: 2, should be an associative array \\(8\\)")
    message(FATAL_ERROR "${format}: missing invalid metadata type diagnostic\n${output}")
  endif()
  # Check that structured reports reach their closing delimiters.
  if(format STREQUAL "json" AND NOT output MATCHES "}[\r\n ]*$")
    message(FATAL_ERROR "Incomplete JSON report\n${output}")
  elseif(format STREQUAL "xml" AND NOT output MATCHES "</report>[\r\n ]*$")
    message(FATAL_ERROR "Incomplete XML report\n${output}")
  endif()
endforeach()

# Keep failed fixtures for diagnosis; successful runs leave no media behind.
remove_fixtures("${input}")
