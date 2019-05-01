# unit tests
add_executable(check_flvmeta
  check_flvmeta.c
  check_flv.c
  check_amf.c

  ${CMAKE_SOURCE_DIR}/src/amf.c
  ${CMAKE_SOURCE_DIR}/src/flv.c
  ${CMAKE_SOURCE_DIR}/src/types.c
)

target_include_directories(check_flvmeta BEFORE PRIVATE ${CHECK_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})
target_link_libraries(check_flvmeta ${CHECK_LIBRARIES} pthread)

add_test(check_flvmeta ${CMAKE_CURRENT_BINARY_DIR}/check_flvmeta)
