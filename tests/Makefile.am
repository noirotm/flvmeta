if MAKE_TESTS
  TESTS = check_flvmeta
  check_PROGRAMS = check_flvmeta
  check_flvmeta_SOURCES = check_amf.c $(top_srcdir)/src/amf.h
  check_flvmeta_CFLAGS = -I$(top_srcdir)/src @CHECK_CFLAGS@
  check_flvmeta_LDADD = @CHECK_LIBS@ $(top_builddir)/src/amf.o $(top_builddir)/src/types.o
endif
