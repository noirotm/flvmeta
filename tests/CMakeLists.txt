# unit tests
add_executable(flvmeta_tests
  check_flvmeta.c
  check_flv.c
  check_amf.c
  unity.c
  util.c
  util.h

  ${CMAKE_SOURCE_DIR}/src/amf.c
  ${CMAKE_SOURCE_DIR}/src/flv.c
  ${CMAKE_SOURCE_DIR}/src/types.c
)

target_include_directories(flvmeta_tests BEFORE PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR})
target_compile_definitions(flvmeta_tests PRIVATE
  FLVMETA_TEST_TMP_DIR="${CMAKE_CURRENT_BINARY_DIR}"
  UNITY_INCLUDE_DOUBLE
)

add_test(NAME flvmeta_tests COMMAND flvmeta_tests)

add_executable(make_nested_flv make_nested_flv.c util.c util.h)
target_include_directories(make_nested_flv PRIVATE ${CMAKE_SOURCE_DIR})
add_test(NAME nesting_cli COMMAND ${CMAKE_COMMAND}
  -DFLVMETA=$<TARGET_FILE:flvmeta>
  -DGENERATOR=$<TARGET_FILE:make_nested_flv>
  -DTEST_DIR=${CMAKE_CURRENT_BINARY_DIR}/nesting-cli
  -P ${CMAKE_CURRENT_SOURCE_DIR}/nesting_cli.cmake)
set_tests_properties(nesting_cli PROPERTIES TIMEOUT 60)
