#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(flvmeta, 1.0.7, flvmeta-discussion@googlegroups.com)
AC_CONFIG_SRCDIR([flvmeta.c])
AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AC_LANG([C])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h stdio.h string.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_C_BIGENDIAN
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T
AC_TYPE_UINT16_T
AC_TYPE_INT16_T
AC_TYPE_UINT32_T
AC_TYPE_INT32_T
AC_TYPE_UINT64_T
AC_CHECK_SIZEOF([float])
AC_CHECK_SIZEOF([double])
AC_CHECK_SIZEOF([long double])

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_STRFTIME
AC_CHECK_FUNCS([tzset])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
