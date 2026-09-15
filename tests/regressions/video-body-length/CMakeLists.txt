add_executable(video_body_length_fixture fixture.c)
target_link_libraries(video_body_length_fixture PRIVATE regression_fixture)
add_test(NAME video_body_length COMMAND ${CMAKE_COMMAND}
  -DFLVMETA=$<TARGET_FILE:flvmeta>
  -DGENERATOR=$<TARGET_FILE:video_body_length_fixture>
  -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/fixtures
  -P ${CMAKE_CURRENT_SOURCE_DIR}/test.cmake)
set_tests_properties(video_body_length PROPERTIES
  TIMEOUT 60 LABELS "regression;video-body-length")
