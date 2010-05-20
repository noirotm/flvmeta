include(MiniCppUnit.cmake)

include_directories(${CMAKE_SOURCE_DIR}/src)

ADD_MINI_CPP_UNIT_TEST(check_amf
  ${CMAKE_SOURCE_DIR}/src/amf.c
  ${CMAKE_SOURCE_DIR}/src/amf.h
  ${CMAKE_SOURCE_DIR}/src/types.c
  ${CMAKE_SOURCE_DIR}/src/types.h
)

#ADD_MINI_CPP_UNIT_TEST(check_flv)
#ADD_MINI_CPP_UNIT_TEST(check_flvmeta)