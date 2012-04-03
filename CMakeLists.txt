cmake_minimum_required(VERSION 2.6)
project(flvmeta C)

set(FLVMETA_VERSION "1.1")
set(FLVMETA_STABLE no)

# check whether we're in an instable branch
if(FLVMETA_STABLE)
  message(STATUS "Set flvmeta version to ${FLVMETA_VERSION}")
else(FLVMETA_STABLE)
  # if so, check for svnversion, and retrieve current revision
  find_program(GIT "git")
  if(GIT)
    execute_process(COMMAND           "${GIT}" rev-parse --short HEAD
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
                    OUTPUT_VARIABLE   GIT_RELEASE
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
    set(FLVMETA_VERSION "${FLVMETA_VERSION}-g${GIT_RELEASE}")
    message(STATUS "Set unstable flvmeta version to ${FLVMETA_VERSION}")
    unset(GIT CACHE)
  else(GIT)
    message(STATUS "Git not found, set flvmeta version to ${FLVMETA_VERSION}")
    unset(GIT-NOT_FOUND CACHE)
  endif(GIT)
endif(FLVMETA_STABLE)

# generic variables
set(PACKAGE           "flvmeta")
set(PACKAGE_NAME      ${PACKAGE})
set(PACKAGE_BUGREPORT "flvmeta-discussion@googlegroups.com")
set(PACKAGE_VERSION   "${FLVMETA_VERSION}")
set(PACKAGE_STRING    "${PACKAGE_NAME} ${PACKAGE_VERSION}")

#platform tests
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckTypeSize)
include(TestBigEndian)

check_include_file(sys/types.h  HAVE_SYS_TYPES_H)
check_include_file(stdint.h     HAVE_STDINT_H)
check_include_file(stddef.h     HAVE_STDDEF_H)
check_include_file(inttypes.h   HAVE_INTTYPES_H)

check_type_size("double"      SIZEOF_DOUBLE)
check_type_size("float"       SIZEOF_FLOAT)
check_type_size("long double" SIZEOF_LONG_DOUBLE)

if(WIN32)
  add_definitions(-D_FILE_OFFSET_BITS=64)
endif(WIN32)

# MSVC before VS 2010 did not have stdint.h
if(MSVC AND NOT HAVE_STDINT_H)
  set(int16_t  1)
  set(int32_t  1)
  set(int64_t  1)
  set(int8_t   1)
  set(uint16_t 1)
  set(uint32_t 1)
  set(uint64_t 1)
  set(uint8_t  1)
endif(MSVC AND NOT HAVE_STDINT_H)

test_big_endian(IS_BIGENDIAN)
if(IS_BIGENDIAN)
  set(WORDS_BIGENDIAN 1)
endif(IS_BIGENDIAN)

# large file support
check_function_exists("fseeko" HAVE_FSEEKO)
if(HAVE_FSEEKO)
  execute_process(COMMAND getconf LFS_CFLAGS
    OUTPUT_VARIABLE LFS_CFLAGS
    ERROR_QUIET)
  if(LFS_CFLAGS)
    add_definitions(${LFS_CFLAGS})
  endif(LFS_CFLAGS)
endif(HAVE_FSEEKO)

# configuration file
configure_file(config-cmake.h.in ${CMAKE_BINARY_DIR}/config.h)
include_directories(${CMAKE_BINARY_DIR})
add_definitions(-DHAVE_CONFIG_H)

# Visual C++ specific configuration
if(MSVC)
  # use static library
  set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /MT")
  
  # C runtime deprecation in Visual C++ 2005 and later
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE -D_CRT_NONSTDC_NO_DEPRECATE)
endif(MSVC)

# installation
set(CPACK_PACKAGE_VENDOR "Marc Noirot <marc.noirot@gmail.com>")
set(CPACK_PACKAGE_VERSION ${FLVMETA_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_SOURCE_DIR}/COPYING)
set(CPACK_RESOURCE_FILE_README ${CMAKE_SOURCE_DIR}/README)
set(CPACK_SOURCE_IGNORE_FILES "/build.*;/CVS/;/\\\\.svn/;/\\\\.bzr/;/\\\\.hg/;/\\\\.git/;\\\\.swp$;\\\\.#;/#")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "flvmeta-${PACKAGE_VERSION}-src")

if(WIN32)
  if (CMAKE_CL_64)
    set(EXECUTABLE_TYPE "64")
  else(CMAKE_CL_64)
    set(EXECUTABLE_TYPE "32")
  endif(CMAKE_CL_64)

  set(CPACK_PACKAGE_FILE_NAME "flvmeta-${PACKAGE_VERSION}-win${EXECUTABLE_TYPE}")
  set(CPACK_GENERATOR ZIP)
  install(FILES ${CMAKE_SOURCE_DIR}/README  DESTINATION . RENAME Readme.txt)
  install(FILES ${CMAKE_SOURCE_DIR}/COPYING DESTINATION . RENAME License.txt)
  install(FILES ${CMAKE_SOURCE_DIR}/NEWS    DESTINATION . RENAME Changelog.txt)
else(WIN32)
  set(CPACK_PACKAGE_FILE_NAME "flvmeta-${PACKAGE_VERSION}")
endif(WIN32)

include(CPack)

add_subdirectory(src)

# tests
#enable_testing()
#add_subdirectory(tests)
