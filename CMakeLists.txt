cmake_minimum_required(VERSION 2.6)
project(flvmeta)
add_subdirectory(src)

# generic variables
set(PACKAGE           "flvmeta")
set(PACKAGE_NAME      ${PACKAGE})
set(PACKAGE_BUGREPORT "flvmeta-discussion@googlegroups.com")
set(PACKAGE_VERSION   "1.1-dev")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")

#platform tests
include(CheckTypeSize)
include(TestBigEndian)

check_type_size("double"      SIZEOF_DOUBLE)
check_type_size("float"       SIZEOF_FLOAT)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

test_big_endian(IS_BIGENDIAN)
if(IS_BIGENDIAN)
  set(WORDS_BIGENDIAN 1)
endif(IS_BIGENDIAN)

configure_file(config-cmake.h.in ${CMAKE_BINARY_DIR}/config.h)

