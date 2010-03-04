/*
    $Id$

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
#include "flvmeta.h"
#include "dump_json.h"
#include "dump_raw.h"
#include "dump_xml.h"
#include "dump_yaml.h"

#include <string.h>

/* dump metadata from a FLV file */
int dump_metadata(const flvmeta_opts * options) {
    int retval;
    flv_parser parser;
    memset(&parser, 0, sizeof(flv_parser));

    switch (options->dump_format) {
        case FLVMETA_FORMAT_JSON:
            dump_json_setup_metadata_dump(&parser);
            break;
        case FLVMETA_FORMAT_RAW:
            dump_raw_setup_metadata_dump(&parser);
            break;
        case FLVMETA_FORMAT_XML:
            dump_xml_setup_metadata_dump(&parser);
            break;
        case FLVMETA_FORMAT_YAML:
            dump_yaml_setup_metadata_dump(&parser);
            break;
    }

    retval = flv_parse(options->input_file, &parser);
    if (retval == FLVMETA_DUMP_STOP_OK) {
        retval = FLV_OK;
    }

    return retval;
}

/* dump the full contents of an FLV file */
int dump_flv_file(const flvmeta_opts * options) {
    flv_parser parser;
    memset(&parser, 0, sizeof(flv_parser));

    switch (options->dump_format) {
        case FLVMETA_FORMAT_JSON:
            dump_json_setup_file_dump(&parser);
            break;
        case FLVMETA_FORMAT_RAW:
            dump_raw_setup_file_dump(&parser);
            break;
        case FLVMETA_FORMAT_XML:
            dump_xml_setup_file_dump(&parser);
            break;
        case FLVMETA_FORMAT_YAML:
            dump_yaml_setup_file_dump(&parser);
            break;
    }

    return flv_parse(options->input_file, &parser);
}

/* dump AMF data directly */
int dump_amf_data(const amf_data * data, const flvmeta_opts * options) {
    switch (options->dump_format) {
        case FLVMETA_FORMAT_JSON:
            return dump_json_amf_data(data);
        case FLVMETA_FORMAT_RAW:
            return dump_raw_amf_data(data);
        case FLVMETA_FORMAT_XML:
            return dump_xml_amf_data(data);
        case FLVMETA_FORMAT_YAML:
            return dump_yaml_amf_data(data);
        default:
            return OK;
    }
}
