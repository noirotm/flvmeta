bin_PROGRAMS = flvmeta

if USE_SYSTEM_LIBS
EXPORT_CFLAGS = $(YAML_CFLAGS)
EXPORT_LDADD = $(YAML_LIBS)
else
SUBDIRS = libyaml
EXPORT_CFLAGS = -I@srcdir@/libyaml
EXPORT_LDADD = @builddir@/libyaml/libyaml.a
endif

AM_CFLAGS = -Wall $(EXPORT_CFLAGS)
flvmeta_LDADD = $(EXPORT_LDADD)

flvmeta_SOURCES = amf.c \
                  amf.h \
                  avc.c \
                  avc.h \
                  check.c \
                  check.h \
                  dump.c \
                  dump.h \
                  dump_json.c \
                  dump_json.h \
                  dump_raw.c \
                  dump_raw.h \
                  dump_xml.c \
                  dump_xml.h \
                  dump_yaml.c \
                  dump_yaml.h \
                  flv.c \
                  flv.h \
                  flvmeta.c \
                  flvmeta.h \
                  info.c \
                  info.h \
                  json.c \
                  json.h \
                  types.c \
                  types.h \
                  update.c \
                  update.h \
                  util.c \
                  util.h

EXTRA_DIST =      CMakeLists.txt \
                  compat/gettext.h \
                  compat/getopt.h \
                  compat/getopt.c \
                  compat/getopt1.c \
                  compat/win32_tmpfile.c \
                  compat/win32_tmpfile.h
