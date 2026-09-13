# Shared process handling. Exact exit results distinguish rejection from crashes
# and timeouts; return stdout through an explicitly named caller variable.
function(assert_command_exit_code expected output_variable)
  execute_process(COMMAND ${ARGN}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
  if(NOT "${result}" STREQUAL "${expected}")
    message(FATAL_ERROR "${ARGN}: expected ${expected}, got ${result}\n${error}\n${output}")
  endif()
  set(${output_variable} "${output}" PARENT_SCOPE)
endfunction()

function(assert_flvmeta_exit_code expected output_variable)
  assert_command_exit_code("${expected}" output "${FLVMETA}" ${ARGN})
  set(${output_variable} "${output}" PARENT_SCOPE)
endfunction()

function(generate_fixture output_variable)
  file(MAKE_DIRECTORY "${TEST_DIR}")
  assert_command_exit_code(0 path "${GENERATOR}" "${TEST_DIR}" ${ARGN})
  string(STRIP "${path}" path)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "Generator did not create a fixture: ${path}")
  endif()
  set(${output_variable} "${path}" PARENT_SCOPE)
endfunction()

# Call after all assertions: failures retain their fixtures for diagnosis.
function(remove_fixtures)
  file(REMOVE ${ARGN})
endfunction()
