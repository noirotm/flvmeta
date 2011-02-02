/*
    $Id: dump_raw.c 170 2010-02-12 16:05:27Z marc.noirot $

    FLV Metadata updater

    Copyright (C) 2007-2011 Marc Noirot <marc.noirot AT gmail.com>

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
#include "dump.h"
#include "dump_raw.h"

#include <stdio.h>
#include <string.h>

/* raw FLV file full dump callbacks */

static int raw_on_header(flv_header * header, flv_parser * parser) {
    int * n;
    
    n = (int*) malloc(sizeof(uint32));
    if (n == NULL) {
        return ERROR_MEMORY;
    }

    parser->user_data = n;
    *n = 0;

    printf("Magic: %.3s\n", header->signature);
    printf("Version: %d\n", header->version);
    printf("Has audio: %s\n", flv_header_has_audio(*header) ? "yes" : "no");
    printf("Has video: %s\n", flv_header_has_video(*header) ? "yes" : "no");
    printf("Offset: %u\n", swap_uint32(header->offset));
    return OK;
}

static int raw_on_tag(flv_tag * tag, flv_parser * parser) {
    int * n;

    /* increment current tag number */
    n = ((int*)parser->user_data);
    ++(*n);

    printf("--- Tag #%u at 0x%" FILE_OFFSET_PRINTF_FORMAT "X", *n, parser->stream->current_tag_offset);
    printf(" (%" FILE_OFFSET_PRINTF_FORMAT "i) ---\n", parser->stream->current_tag_offset);
    printf("Tag type: %s\n", dump_string_get_tag_type(tag));
    printf("Body length: %u\n", flv_tag_get_body_length(*tag));
    printf("Timestamp: %u\n", flv_tag_get_timestamp(*tag));

    return OK;
}

static int raw_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    printf("* Video codec: %s\n", dump_string_get_video_codec(vt));
    printf("* Video frame type: %s\n", dump_string_get_video_frame_type(vt));
    return OK;
}

static int raw_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    printf("* Sound type: %s\n", dump_string_get_sound_type(at));
    printf("* Sound size: %s\n", dump_string_get_sound_size(at));
    printf("* Sound rate: %s\n", dump_string_get_sound_rate(at));
    printf("* Sound format: %s\n", dump_string_get_sound_format(at));
    return OK;
}

static int raw_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    printf("* Metadata event name: %s\n", amf_string_get_bytes(name));
    printf("* Metadata contents: ");
    amf_data_dump(stdout, data, 0);
    printf("\n");
    return OK;
}

static int raw_on_prev_tag_size(uint32 size, flv_parser * parser) {
    printf("Previous tag size: %u\n", size);
    return OK;
}

static int raw_on_stream_end(flv_parser * parser) {
    free(parser->user_data);
    return OK;
}

/* raw FLV file metadata dump callback */
static int raw_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
        dump_raw_amf_data(data);
        return FLVMETA_DUMP_STOP_OK;
    }
    return OK;
}

/* setup dumping */

void dump_raw_setup_metadata_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_metadata_tag = raw_on_metadata_tag_only;
    }
}

void dump_raw_setup_file_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_header = raw_on_header;
        parser->on_tag = raw_on_tag;
        parser->on_audio_tag = raw_on_audio_tag;
        parser->on_video_tag = raw_on_video_tag;
        parser->on_metadata_tag = raw_on_metadata_tag;
        parser->on_prev_tag_size = raw_on_prev_tag_size;
        parser->on_stream_end = raw_on_stream_end;
    }
}

int dump_raw_amf_data(const amf_data * data) {
    amf_data_dump(stdout, data, 0);
    printf("\n");
    return OK;
}
