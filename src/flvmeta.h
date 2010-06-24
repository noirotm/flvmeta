/*
    $Id: flvmeta.h 205 2010-06-24 14:48:46Z marc.noirot $

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
#ifndef __FLVMETA_H__
#define __FLVMETA_H__

/* Configuration of the sources */
#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "flv.h"

/* copyright string */
#define COPYRIGHT_STR "Copyright (C) 2007-2010 Marc Noirot <marc.noirot AT gmail.com>"

/* error statuses */
#define OK                  FLV_OK
#define ERROR_OPEN_READ     FLV_ERROR_OPEN_READ
#define ERROR_NO_FLV        FLV_ERROR_NO_FLV
#define ERROR_EOF           FLV_ERROR_EOF
#define ERROR_MEMORY        FLV_ERROR_MEMORY
#define ERROR_OPEN_WRITE    5
#define ERROR_INVALID_TAG   6
#define ERROR_WRITE         7
#define ERROR_SAME_FILE     8

/* invalid flv file reported by the check command (one or more errors) */
#define ERROR_INVALID_FLV_FILE 9

/* stop file parsing without error */
#define FLVMETA_DUMP_STOP_OK 10

/* commands */
#define FLVMETA_DEFAULT_COMMAND     0
#define FLVMETA_DUMP_COMMAND        1
#define FLVMETA_FULL_DUMP_COMMAND   2
#define FLVMETA_CHECK_COMMAND       3
#define FLVMETA_UPDATE_COMMAND      4
#define FLVMETA_VERSION_COMMAND     5
#define FLVMETA_HELP_COMMAND        6

/* tags handling */
#define FLVMETA_EXIT_ON_ERROR       0
#define FLVMETA_FIX_ERRORS          1
#define FLVMETA_IGNORE_ERRORS       2

/* check levels */
#define FLVMETA_CHECK_LEVEL_INFO    0
#define FLVMETA_CHECK_LEVEL_WARNING 1
#define FLVMETA_CHECK_LEVEL_ERROR   2
#define FLVMETA_CHECK_LEVEL_FATAL   3

/* dump formats */
#define FLVMETA_FORMAT_XML          0
#define FLVMETA_FORMAT_RAW          1
#define FLVMETA_FORMAT_JSON         2
#define FLVMETA_FORMAT_YAML         3

/* flvmeta options */
typedef struct __flvmeta_opts {
    int command;
    char * input_file;
    char * output_file;
    amf_data * metadata;
    int dump_metadata;
    int check_level;
    int quiet;
    int check_xml_report;
    int insert_onlastsecond;
    int reset_timestamps;
    int preserve_metadata;
    int error_handling;
    int dump_format;
    int verbose;
} flvmeta_opts;

#endif /* __FLVMETA_H__ */
