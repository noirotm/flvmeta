# unit tests
include_directories(BEFORE ${CHECK_INCLUDE_DIR} ${CMAKE_SOURCE_DIR})

add_executable(check_flvmeta
  check_flvmeta.c
  check_flv.c
  check_amf.c

  ${CMAKE_SOURCE_DIR}/src/amf.c
  ${CMAKE_SOURCE_DIR}/src/flv.c
  ${CMAKE_SOURCE_DIR}/src/types.c
)
target_link_libraries(check_flvmeta ${CHECK_LIBRARIES} pthread)

add_test(check_flvmeta ${CMAKE_CURRENT_BINARY_DIR}/check_flvmeta)
