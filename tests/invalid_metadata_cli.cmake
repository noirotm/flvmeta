# Issue #30: a string-valued onMetaData must be rejected before list traversal.
# Run the actual checker, since the Unity executable does not link check.c.
execute_process(COMMAND "${GENERATOR}"
  RESULT_VARIABLE result OUTPUT_VARIABLE input ERROR_VARIABLE error
  OUTPUT_STRIP_TRAILING_WHITESPACE TIMEOUT 10)
if(NOT "${result}" STREQUAL "0")
  message(FATAL_ERROR "Fixture generation failed: ${result}\n${error}")
endif()

foreach(format raw json xml)
  execute_process(COMMAND "${FLVMETA}" --check --${format} "${input}"
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
  # An arbitrary nonzero result could be a crash, so require ERROR_INVALID_FLV_FILE.
  if(NOT "${result}" STREQUAL "9")
    message(FATAL_ERROR "${format}: expected exit 9, got ${result}\n${error}\n${output}")
  endif()
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
file(REMOVE "${input}")
