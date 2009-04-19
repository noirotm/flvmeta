bin_PROGRAMS = flvmeta flvdump

AM_CFLAGS = -Wall

common_sources = amf.c types.c flvmeta.h flv.h amf.h types.h flv.c flv.h

flvmeta_SOURCES = flvmeta.c avc.c avc.h $(common_sources)
flvdump_SOURCES = flvdump.c $(common_sources)

EXTRA_DIST = flvmeta.sln flvmeta.vcproj flvdump.vcproj Makefile.win config.h.win release.bat
