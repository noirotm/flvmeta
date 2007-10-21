AM_CFLAGS = -Wall

bin_PROGRAMS = flvmeta flvdump

common_sources = amf.c types.c flvmeta.h flv.h amf.h types.h flv.h 

flvmeta_SOURCES = flvmeta.c mmap.c mmap.h $(common_sources)
flvdump_SOURCES = flvdump.c $(common_sources)
