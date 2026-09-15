/* Shared driver and FLV framing for regression fixtures. */
#ifndef FLVMETA_REGRESSION_FIXTURE_H
#define FLVMETA_REGRESSION_FIXTURE_H

#include <stdio.h>

/* Implemented by each issue. Write tags to file; return zero on success.
   argc/argv contain only the issue-specific arguments, without argv[0]. */
int write_flv_fixture(FILE * file, int argc, char ** argv);

/* Write a script-tag header and the AMF event name "onMetaData". The caller
   writes value_size bytes of AMF data, then calls the footer with that size.
   Helpers return nonzero on success. No production parser/writer is used. */
int fixture_metadata_header(FILE * file, size_t value_size);
int fixture_metadata_footer(FILE * file, size_t value_size);

/* Complete framing for fixtures containing audio/video tags. */
int fixture_header(FILE * file, unsigned int flags);
int fixture_tag(FILE * file, unsigned int type, const unsigned char * data, size_t size);

#endif
