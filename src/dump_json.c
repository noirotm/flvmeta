/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2016 Marc Noirot <marc.noirot AT gmail.com>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "dump.h"
#include "dump_json.h"
#include "json.h"

#include <stdio.h>
#include <string.h>

/* JSON metadata dumping */
static void json_amf_data_dump(const amf_data * data, json_emitter * je) {
    if (data != NULL) {
        amf_node * node;
        time_t time;
        struct tm * t;
        char str[128];

        switch (data->type) {
            case AMF_TYPE_NUMBER:
                json_emit_number(je, data->number_data);
                break;
            case AMF_TYPE_BOOLEAN:
                json_emit_boolean(je, data->boolean_data);
                break;
            case AMF_TYPE_STRING:
                json_emit_string(je, (char *)amf_string_get_bytes(data), amf_string_get_size(data));
                break;
            case AMF_TYPE_OBJECT:
                json_emit_object_start(je);
                node = amf_object_first(data);
                while (node != NULL) {
                    json_emit_object_key(je,
                        (char *)amf_string_get_bytes(amf_object_get_name(node)),
                        amf_string_get_size(amf_object_get_name(node))
                    );
                    json_amf_data_dump(amf_object_get_data(node), je);
                    node = amf_object_next(node);
                }
                json_emit_object_end(je);
                break;
            case AMF_TYPE_NULL:
            case AMF_TYPE_UNDEFINED:
                json_emit_null(je);
                break;
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                json_emit_object_start(je);
                node = amf_associative_array_first(data);
                while (node != NULL) {
                    json_emit_object_key(je,
                        (char *)amf_string_get_bytes(amf_associative_array_get_name(node)),
                        amf_string_get_size(amf_associative_array_get_name(node))
                    );
                    json_amf_data_dump(amf_object_get_data(node), je);
                    node = amf_associative_array_next(node);
                }
                json_emit_object_end(je);
                break;
            case AMF_TYPE_ARRAY:
                json_emit_array_start(je);
                node = amf_array_first(data);
                while (node != NULL) {
                    json_amf_data_dump(amf_array_get(node), je);
                    node = amf_array_next(node);
                }
                json_emit_array_end(je);
                break;
            case AMF_TYPE_DATE:
                time = amf_date_to_time_t(data);
                tzset();
                t = localtime(&time);
                strftime(str, sizeof(str), "%Y-%m-%dT%H:%M:%S", t);
                json_emit_string(je, str, strlen(str));
                break;
            case AMF_TYPE_XML: break;
            case AMF_TYPE_CLASS: break;
            default: break;
        }
    }
}

/* JSON FLV file full dump callbacks */

static int json_on_header(flv_header * header, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_start(je);
    json_emit_object_key_z(je, "magic");
    json_emit_string(je, (char*)header->signature, 3);
    json_emit_object_key_z(je, "hasVideo");
    json_emit_boolean(je, flv_header_has_video(*header));
    json_emit_object_key_z(je, "hasAudio");
    json_emit_boolean(je, flv_header_has_audio(*header));
    json_emit_object_key_z(je, "version");
    json_emit_integer(je, header->version);
    json_emit_object_key_z(je, "tags");
    json_emit_array_start(je);

    return OK;
}

static int json_on_tag(flv_tag * tag, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_start(je);
    json_emit_object_key_z(je, "type");
    json_emit_string_z(je, dump_string_get_tag_type(tag));
    json_emit_object_key_z(je, "timestamp");
    json_emit_integer(je, flv_tag_get_timestamp(*tag));
    json_emit_object_key_z(je, "dataSize");
    json_emit_integer(je, flv_tag_get_body_length(*tag));
    json_emit_object_key_z(je, "offset");
    json_emit_file_offset(je, parser->stream->current_tag_offset);

    return OK;
}

static int json_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_key_z(je, "videoData");
    json_emit_object_start(je);
    json_emit_object_key_z(je, "codecID");
    json_emit_string_z(je, dump_string_get_video_codec(vt));
    json_emit_object_key_z(je, "frameType");
    json_emit_string_z(je, dump_string_get_video_frame_type(vt));

    /* if AVC, detect frame type and composition time */
    if (flv_video_tag_codec_id(vt) == FLV_VIDEO_TAG_CODEC_AVC) {
        flv_avc_packet_type type;

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_avc_packet_type)) < sizeof(flv_avc_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        json_emit_object_key_z(je, "AVCData");

        json_emit_object_start(je);
        json_emit_object_key_z(je, "packetType");
        json_emit_string_z(je, dump_string_get_avc_packet_type(type));

        /* composition time */
        if (type == FLV_AVC_PACKET_TYPE_NALU) {
            uint24_be composition_time;

            if (flv_read_tag_body(parser->stream, &composition_time, sizeof(uint24_be)) < sizeof(uint24_be)) {
                return ERROR_INVALID_TAG;
            }

            json_emit_object_key_z(je, "compositionTimeOffset");
            json_emit_integer(je, uint24_be_to_uint32(composition_time));
        }

        json_emit_object_end(je);
    }

    json_emit_object_end(je);

    return OK;
}

static int json_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_key_z(je, "audioData");
    json_emit_object_start(je);
    json_emit_object_key_z(je, "type");
    json_emit_string_z(je, dump_string_get_sound_type(at));
    json_emit_object_key_z(je, "size");
    json_emit_string_z(je, dump_string_get_sound_size(at));
    json_emit_object_key_z(je, "rate");
    json_emit_string_z(je, dump_string_get_sound_rate(at));
    json_emit_object_key_z(je, "format");
    json_emit_string_z(je, dump_string_get_sound_format(at));

    /* if AAC, detect packet type */
    if (flv_audio_tag_sound_format(at) == FLV_AUDIO_TAG_SOUND_FORMAT_AAC) {
        flv_aac_packet_type type;

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_aac_packet_type)) < sizeof(flv_aac_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        json_emit_object_key_z(je, "AACData");

        json_emit_object_start(je);
        json_emit_object_key_z(je, "packetType");

        json_emit_string_z(je, dump_string_get_aac_packet_type(type));

        json_emit_object_end(je);
    }

    json_emit_object_end(je);

    return OK;
}

static int json_on_metadata_tag(flv_tag * tag, char * name, amf_data * data, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_key_z(je, "scriptDataObject");
    json_emit_object_start(je);
    json_emit_object_key_z(je, "name");
    json_emit_string_z(je, name);
    json_emit_object_key_z(je, "metadata");
    json_amf_data_dump(data, je);
    json_emit_object_end(je);

    return OK;
}

static int json_on_prev_tag_size(uint32 size, flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_object_end(je);

    return OK;
}

static int json_on_stream_end(flv_parser * parser) {
    json_emitter * je;
    je = (json_emitter*)parser->user_data;

    json_emit_array_end(je);
    json_emit_object_end(je);

    return OK;
}

/* JSON FLV file metadata dump callback */
static int json_on_metadata_tag_only(flv_tag * tag, char * name, amf_data * data, flv_parser * parser) {
    flvmeta_opts * options = (flvmeta_opts*) parser->user_data;

    if (options->metadata_event == NULL) {
        if (!strcmp(name, "onMetaData")) {
            dump_json_amf_data(data);
            return FLVMETA_DUMP_STOP_OK;
        }
    }
    else {
        if (!strcmp(name, options->metadata_event)) {
            dump_json_amf_data(data);
        }
    }
    return OK;
}

/* setup dumping */

void dump_json_setup_metadata_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_metadata_tag = json_on_metadata_tag_only;
    }
}

int dump_json_file(flv_parser * parser, const flvmeta_opts * options) {
    json_emitter je;

    parser->on_header = json_on_header;
    parser->on_tag = json_on_tag;
    parser->on_audio_tag = json_on_audio_tag;
    parser->on_video_tag = json_on_video_tag;
    parser->on_metadata_tag = json_on_metadata_tag;
    parser->on_prev_tag_size = json_on_prev_tag_size;
    parser->on_stream_end = json_on_stream_end;

    json_emit_init(&je);
    parser->user_data = &je;

    return flv_parse(options->input_file, parser);
}

int dump_json_amf_data(const amf_data * data) {
    json_emitter je;
    json_emit_init(&je);

    /* dump AMF into JSON */
    json_amf_data_dump(data, &je);

    printf("\n");

    return OK;
}
