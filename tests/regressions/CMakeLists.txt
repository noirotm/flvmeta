# CLI regression tests, grouped by their originating GitHub issue.
add_library(regression_fixture STATIC support/fixture.c support/fixture.h)
target_include_directories(regression_fixture PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/support)

function(add_flvmeta_regression issue)
  set(generator issue_${issue}_fixture)
  set(test_name issue_${issue})
  add_executable(${generator} fixture.c)
  target_link_libraries(${generator} PRIVATE regression_fixture)
  add_test(NAME ${test_name} COMMAND ${CMAKE_COMMAND}
    -DFLVMETA=$<TARGET_FILE:flvmeta>
    -DGENERATOR=$<TARGET_FILE:${generator}>
    -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/fixtures
    -P ${CMAKE_CURRENT_SOURCE_DIR}/test.cmake)
  set_tests_properties(${test_name} PROPERTIES
    TIMEOUT 60 LABELS "regression;issue-${issue}")
endfunction()

add_subdirectory(issue-29)
add_subdirectory(issue-30)
add_subdirectory(issue-31)
add_subdirectory(issue-32)
add_subdirectory(video-body-length)
