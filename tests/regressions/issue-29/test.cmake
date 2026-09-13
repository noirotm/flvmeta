# https://github.com/noirotm/flvmeta/issues/29
# Run against the real executable: unit tests do not link the dumpers or updater.
# Every generated file contains nested metadata followed by {recovery: true}.

include("${CMAKE_CURRENT_LIST_DIR}/../support/cli.cmake")

# A normal dump selects the first usable onMetaData tag. Full dump must
# reach the second tag in both cases and never emit a rejected partial tree.
function(check_dump_formats input depth)
  foreach(format raw xml json yaml)
    assert_flvmeta_exit_code(0 dump_output --dump --${format} "${input}")
    if(depth GREATER 128 AND NOT dump_output MATCHES "recovery")
      message(FATAL_ERROR "${format}: dump did not recover after rejected metadata")
    endif()
    if(depth EQUAL 128 AND NOT dump_output MATCHES "nested")
      message(FATAL_ERROR "${format}: metadata at the depth limit was not dumped")
    endif()

    assert_flvmeta_exit_code(0 full_dump_output --full-dump --${format} "${input}")
    if(NOT full_dump_output MATCHES "recovery")
      message(FATAL_ERROR "${format}: full dump did not reach the second tag")
    endif()
    if(depth GREATER 128 AND full_dump_output MATCHES "nested")
      message(FATAL_ERROR "${format}: rejected metadata was partially dumped")
    endif()
  endforeach()
endfunction()

# These metadata-only fixtures are not valid playable files, even at depth
# 128. Exit 9 means the checker reported errors; deeper inputs must also
# produce the specific invalid-metadata diagnostic.
function(check_validation input depth)
  assert_flvmeta_exit_code(9 check_output --check "${input}")
  if(depth GREATER 128 AND NOT check_output MATCHES "invalid metadata")
    message(FATAL_ERROR "Check did not report rejected metadata")
  endif()
endfunction()

# Default update policy stops with ERROR_INVALID_TAG (7). With --ignore,
# update should finish and produce metadata that can be parsed again.
function(check_update input output depth kind)
  if(depth GREATER 128)
    assert_flvmeta_exit_code(7 update_output --update "${input}" "${output}")
  endif()
  assert_flvmeta_exit_code(0 update_output --update --ignore --preserve --no-lastsecond "${input}" "${output}")
  assert_flvmeta_exit_code(0 dump_output --dump --json "${output}")
  if(kind EQUAL 2 AND depth EQUAL 128 AND NOT dump_output MATCHES "nested")
    message(FATAL_ERROR "Update did not preserve accepted nested metadata")
  endif()
endfunction()

# Keep fixture paths and cleanup together. Failed cases retain their files
# for diagnosis; successful cases leave no generated media behind.
function(check_nesting_case kind depth)
  generate_fixture(input ${depth} ${kind})
  set(output "${input}.updated.flv")
  check_dump_formats("${input}" ${depth})
  check_validation("${input}" ${depth})
  check_update("${input}" "${output}" ${depth} ${kind})
  remove_fixtures("${input}" "${output}")
endfunction()

# Kinds: strict arrays, objects, ECMA arrays, and alternating container types.
# Depths: highest accepted value, first rejected value, and stack-overflow-scale
# input. Counts include the outer ECMA array added by the fixture generator.
foreach(kind RANGE 0 3)
  foreach(depth 128 129 70000)
    check_nesting_case(${kind} ${depth})
  endforeach()
endforeach()
