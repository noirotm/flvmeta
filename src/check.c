/*
    $Id: check.c 176 2010-03-05 14:54:21Z marc.noirot $

    FLV Metadata updater

    Copyright (C) 2007-2010 Marc Noirot <marc.noirot AT gmail.com>

    This file is part of FLVMeta.

    FLVMeta is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLVMeta is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLVMeta; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "check.h"

#include <time.h>

/* start the report */
static void report_start(const flvmeta_opts * opts) {
    if (opts->check_xml_report) {
        time_t now;
        struct tm * t;
        char datestr[128];

        puts("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        puts("<report xmlns=\"http://schemas.flvmeta.org/report/1.0/\">");
        puts("  <metadata>");
        printf("    <filename>%s</filename>\n", opts->input_file);

        now = time(NULL);
        tzset();
        t = localtime(&now);
        strftime(datestr, sizeof(datestr), "%Y-%m-%dT%H:%M:%S", t);
        printf("    <creation-date>%s</creation-date>\n", datestr);

        printf("    <generator>%s</generator>\n", PACKAGE_STRING);

        puts("  </metadata>");
        puts("  <messages>");
    }
}

/* end the report */
static void report_end(const flvmeta_opts * opts, int errors, int warnings) {
    if (opts->check_xml_report) {
        puts("  </messages>");
        puts("</report>");
    }
    else {
        printf("(%d) error(s), (%d) warning(s)\n", errors, warnings);
    }
}

/* report an error to stdout according to the current format */
static void report_print_message(
    int level,
    const char * code,
    file_offset_t offset,
    const char * message,
    const flvmeta_opts * opts
) {
    if (level >= opts->check_level) {
        char * levelstr;

        switch (level) {
            case FLVMETA_CHECK_LEVEL_INFO: levelstr = "info"; break;
            case FLVMETA_CHECK_LEVEL_WARNING: levelstr = "warning"; break;
            case FLVMETA_CHECK_LEVEL_ERROR: levelstr = "error"; break;
            case FLVMETA_CHECK_LEVEL_FATAL: levelstr = "fatal"; break;
            default: levelstr = "unknown";
        }

        if (opts->check_xml_report) {
            /* XML report entry */
            printf("    <message level=\"%s\" code=\"%s\"", levelstr, code);
            printf(" offset=\"%" FILE_OFFSET_PRINTF_FORMAT  "i\">", offset);
            printf("%s</message>\n", message);
        }
        else {
            /* raw report entry */
            printf("%s", opts->input_file);
            printf("(%" FILE_OFFSET_PRINTF_FORMAT "x) : ", offset);
            printf("%s %s: %s\n", levelstr, code, message);
        }
    }
}

/* convenience macros */
#define print_info(code, offset, message) \
    report_print_message(FLVMETA_CHECK_LEVEL_INFO, code, offset, message, opts)
#define print_warning(code, offset, message) \
    ++warnings; \
    report_print_message(FLVMETA_CHECK_LEVEL_WARNING, code, offset, message, opts)
#define print_error(code, offset, message) \
    ++errors; \
    report_print_message(FLVMETA_CHECK_LEVEL_ERROR, code, offset, message, opts)
#define print_fatal(code, offset, message) \
    ++errors; \
    report_print_message(FLVMETA_CHECK_LEVEL_FATAL, code, offset, message, opts)


/* check FLV file validity */
int check_flv_file(const flvmeta_opts * opts) {
    flv_stream * flv_in;
    int errors, warnings;

    flv_in = flv_open(opts->input_file);
    if (flv_in == NULL) {
        return ERROR_OPEN_READ;
    }
    
    errors = warnings = 0;

    report_start(opts);
    report_end(opts, errors, warnings);
    
    flv_close(flv_in);
    return OK;
}
