dnl -*- Autoconf -*-
dnl Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)

dnl SVN revision
define([svnversion], esyscmd([sh -c "svnversion -n"]))dnl

AC_INIT([flvmeta], [1.1-r]svnversion, [flvmeta-discussion@googlegroups.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AC_LANG([C])

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

dnl Checks for check.
PKG_CHECK_MODULES([CHECK], [check >= 0.9.4],
  [have_check=yes],
  [have_check=no]
)
AM_CONDITIONAL([MAKE_TESTS], [test "$have_check" = yes])

dnl Checks for header files.
AC_CHECK_HEADERS([stdlib.h stdio.h string.h])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_INT16_T
AC_TYPE_UINT32_T
AC_TYPE_INT32_T
AC_TYPE_UINT64_T
AC_TYPE_INT64_T
AC_CHECK_SIZEOF([float])
AC_CHECK_SIZEOF([double])
AC_CHECK_SIZEOF([long double])

dnl Checks for large file support
AC_SYS_LARGEFILE
AC_FUNC_FSEEKO

dnl Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([tzset])

dnl Output files
AC_CONFIG_FILES([ 
Makefile
src/Makefile
src/libyaml/Makefile
tests/Makefile
schemas/Makefile
])
AC_OUTPUT
