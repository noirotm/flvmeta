AM_CFLAGS = -Wall -I@srcdir@/libyaml

SUBDIRS = libyaml

bin_PROGRAMS = flvmeta

flvmeta_LDADD = @builddir@/libyaml/libyaml.a

flvmeta_SOURCES = amf.c \
                  amf.h \
                  avc.c \
                  avc.h \
                  dump.c \
                  dump.h \
                  dump_json.c \
                  dump_json.h \
                  dump_xml.c \
                  dump_xml.h \
                  dump_yaml.c \
                  dump_yaml.h \
                  flv.c \
                  flv.h \
                  flvmeta.c \
                  flvmeta.h \
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
