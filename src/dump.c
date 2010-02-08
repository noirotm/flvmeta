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
#include "dump_xml.h"
#include "dump_yaml.h"

#include <string.h>

/* dump metadata from a FLV file */
int dump_metadata(const flvmeta_opts * options) {
    int retval;
    flv_parser parser;
    memset(&parser, 0, sizeof(flv_parser));

    switch (options->dump_format) {
        case FLVMETA_FORMAT_XML:
            parser.on_metadata_tag = xml_on_metadata_tag_only;
            break;
        case FLVMETA_FORMAT_JSON:
            parser.on_metadata_tag = json_on_metadata_tag_only;
            break;
        case FLVMETA_FORMAT_YAML:
            parser.on_metadata_tag = yaml_on_metadata_tag_only;
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
        case FLVMETA_FORMAT_XML:
            parser.on_header = xml_on_header;
            parser.on_tag = xml_on_tag;
            parser.on_audio_tag = xml_on_audio_tag;
            parser.on_video_tag = xml_on_video_tag;
            parser.on_metadata_tag = xml_on_metadata_tag;
            parser.on_prev_tag_size = xml_on_prev_tag_size;
            parser.on_stream_end = xml_on_stream_end;
            break;
        case FLVMETA_FORMAT_JSON:
            parser.user_data = NULL;
            parser.on_header = json_on_header;
            parser.on_tag = json_on_tag;
            parser.on_audio_tag = json_on_audio_tag;
            parser.on_video_tag = json_on_video_tag;
            parser.on_metadata_tag = json_on_metadata_tag;
            parser.on_prev_tag_size = json_on_prev_tag_size;
            parser.on_stream_end = json_on_stream_end;
            break;
        case FLVMETA_FORMAT_YAML:
            parser.user_data = NULL;
            parser.on_header = yaml_on_header;
            parser.on_tag = yaml_on_tag;
            parser.on_audio_tag = yaml_on_audio_tag;
            parser.on_video_tag = yaml_on_video_tag;
            parser.on_metadata_tag = yaml_on_metadata_tag;
            parser.on_prev_tag_size = yaml_on_prev_tag_size;
            parser.on_stream_end = yaml_on_stream_end;
            break;
    }

    return flv_parse(options->input_file, &parser);
}
