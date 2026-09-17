# Set TEST_CONTEXT to "issue / case / operation" before a group of assertions.
# Functions inherit it from the caller, including nested test functions.
function(assert_contains actual expected)
  string(FIND "${actual}" "${expected}" position)
  if(position EQUAL -1)
    message(FATAL_ERROR "${TEST_CONTEXT}: expected text:\n${expected}\nActual output:\n${actual}")
  endif()
endfunction()

function(assert_not_contains actual unexpected)
  string(FIND "${actual}" "${unexpected}" position)
  if(NOT position EQUAL -1)
    message(FATAL_ERROR "${TEST_CONTEXT}: unexpected text:\n${unexpected}\nActual output:\n${actual}")
  endif()
endfunction()

function(assert_equal actual expected)
  if(NOT "${actual}" STREQUAL "${expected}")
    message(FATAL_ERROR "${TEST_CONTEXT}: expected '${expected}', got '${actual}'")
  endif()
endfunction()

function(assert_matches actual pattern)
  if(NOT "${actual}" MATCHES "${pattern}")
    message(FATAL_ERROR "${TEST_CONTEXT}: expected pattern:\n${pattern}\nActual output:\n${actual}")
  endif()
endfunction()

# Exact exit results distinguish rejection from crashes and timeouts.
# Return stdout through the named caller variable; retain stderr for failures.
function(assert_command_exit_code expected output_variable)
  execute_process(COMMAND ${ARGN}
    RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE error TIMEOUT 10)
  if(NOT "${result}" STREQUAL "${expected}")
    message(FATAL_ERROR "${TEST_CONTEXT}: expected exit ${expected}, got ${result}\nCommand: ${ARGN}\nStderr:\n${error}\nStdout:\n${output}")
  endif()
  set(${output_variable} "${output}" PARENT_SCOPE)
endfunction()

function(assert_flvmeta_exit_code expected output_variable)
  assert_command_exit_code("${expected}" output "${FLVMETA}" ${ARGN})
  set(${output_variable} "${output}" PARENT_SCOPE)
endfunction()

function(generate_fixture output_variable)
  set(TEST_CONTEXT "${TEST_CONTEXT} / generate fixture")
  file(MAKE_DIRECTORY "${TEST_DIR}")
  assert_command_exit_code(0 path "${GENERATOR}" "${TEST_DIR}" ${ARGN})
  string(STRIP "${path}" path)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR "${TEST_CONTEXT}: generator did not create ${path}")
  endif()
  set(${output_variable} "${path}" PARENT_SCOPE)
endfunction()

# Call after all assertions: failures retain their fixtures for diagnosis.
function(remove_fixtures)
  file(REMOVE ${ARGN})
endfunction()
