# https://github.com/noirotm/flvmeta/issues/29
# Each file contains nested metadata followed by {recovery: true}.
include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

function(check_dump_formats input depth)
  foreach(format raw xml json yaml)
    set(TEST_CONTEXT "${case_context} / ${format} dump")
    assert_flvmeta_exit_code(0 dump_output --dump --${format} "${input}")
    # A normal dump selects the first usable metadata tag.
    if(depth GREATER 128)
      assert_contains("${dump_output}" "recovery")
    elseif(depth EQUAL 128)
      assert_contains("${dump_output}" "nested")
    endif()

    set(TEST_CONTEXT "${case_context} / ${format} full dump")
    assert_flvmeta_exit_code(0 full_dump_output --full-dump --${format} "${input}")
    assert_contains("${full_dump_output}" "recovery")
    # Rejected metadata must not appear as a partially decoded tree.
    if(depth GREATER 128)
      assert_not_contains("${full_dump_output}" "nested")
    endif()
  endforeach()
endfunction()

function(check_validation input depth)
  set(TEST_CONTEXT "${case_context} / check")
  # These files have no media streams, so even accepted metadata gives exit 9.
  assert_flvmeta_exit_code(9 check_output --check "${input}")
  if(depth GREATER 128)
    assert_contains("${check_output}" "invalid metadata")
  endif()
endfunction()

function(check_update input rewritten depth kind)
  if(depth GREATER 128)
    set(TEST_CONTEXT "${case_context} / update with default error policy")
    assert_flvmeta_exit_code(7 update_output --update "${input}" "${rewritten}")
  endif()
  set(TEST_CONTEXT "${case_context} / update with --ignore --preserve")
  assert_flvmeta_exit_code(0 update_output
    --update --ignore --preserve --no-lastsecond "${input}" "${rewritten}")
  set(TEST_CONTEXT "${case_context} / generated metadata")
  assert_flvmeta_exit_code(0 metadata_output --dump --json "${rewritten}")
  if(kind STREQUAL "ecma-array" AND depth EQUAL 128)
    assert_contains("${metadata_output}" "nested")
  endif()
endfunction()

function(check_nesting_case kind depth)
  set(case_context "issue-29 / ${kind} / depth ${depth}")
  set(TEST_CONTEXT "${case_context}")
  generate_fixture(input ${depth} "${kind}")
  set(rewritten "${input}.updated.flv")
  check_dump_formats("${input}" ${depth})
  check_validation("${input}" ${depth})
  check_update("${input}" "${rewritten}" ${depth} "${kind}")
  remove_fixtures("${input}" "${rewritten}")
endfunction()

# Depth includes the outer ECMA array: last accepted, first rejected, then
# enough nesting to exhaust the stack before the recursion limit was added.
foreach(kind strict-array object ecma-array mixed)
  foreach(depth 128 129 70000)
    check_nesting_case("${kind}" ${depth})
  endforeach()
endforeach()
