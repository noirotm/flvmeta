# unit tests
add_executable(flvmeta_tests
  check_flvmeta.c
  check_flv.c
  check_amf.c
  unity.c

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
