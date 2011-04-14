/*
    $Id: dump_json.c 222 2011-04-14 13:53:57Z marc.noirot $

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
#include "dump_json.h"
#include "json.h"

#include <stdio.h>
#include <string.h>

/* JSON metadata dumping */
static void amf_to_json(const amf_data * data, json_t ** object) {
    if (data != NULL) {
        json_t * value;
        amf_node * node;
        time_t time;
        struct tm * t;
        char str[128];
        char * escaped_str;

        switch (data->type) {
            case AMF_TYPE_NUMBER:
                sprintf(str, "%.12g", data->number_data);
                *object = json_new_number(str);
                break;
            case AMF_TYPE_BOOLEAN:
                *object = (data->boolean_data) ? json_new_true() : json_new_false();
                break;
            case AMF_TYPE_STRING:
                escaped_str = json_escape((char *)amf_string_get_bytes(data));
                *object = json_new_string(escaped_str);
                free(escaped_str);
                break;
            case AMF_TYPE_OBJECT:
                *object = json_new_object();
                node = amf_object_first(data);
                while (node != NULL) {
                    amf_to_json(amf_object_get_data(node), &value);
                    escaped_str = json_escape((char *)amf_string_get_bytes(amf_object_get_name(node)));
                    json_insert_pair_into_object(*object, escaped_str, value);
                    free(escaped_str);
                    node = amf_object_next(node);
                }
                break;
            case AMF_TYPE_NULL:
            case AMF_TYPE_UNDEFINED:
                *object = json_new_null();
                break;
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                *object = json_new_object();
                node = amf_associative_array_first(data);
                while (node != NULL) {
                    amf_to_json(amf_associative_array_get_data(node), &value);
                    json_insert_pair_into_object(*object, (const char *)amf_string_get_bytes(amf_associative_array_get_name(node)), value);
                    node = amf_associative_array_next(node);
                }
                break;
            case AMF_TYPE_ARRAY:
                *object = json_new_array();
                node = amf_array_first(data);
                while (node != NULL) {
                    amf_to_json(amf_array_get(node), &value);
                    json_insert_child(*object, value);
                    node = amf_array_next(node);
                }
                break;
            case AMF_TYPE_DATE:
                time = amf_date_to_time_t(data);
                tzset();
                t = localtime(&time);
                strftime(str, sizeof(str), "%Y-%m-%dT%H:%M:%S", t);
                *object = json_new_string(str);
                break;
            case AMF_TYPE_XML: break;
            case AMF_TYPE_CLASS: break;
            default: break;
        }
    }
}

/* JSON FLV file full dump callbacks */

static int json_on_header(flv_header * header, flv_parser * parser) {
    printf("{\"magic\":\"%.3s\",\"hasVideo\":%s,\"hasAudio\":%s,\"version\":%i,\"tags\":[",
        header->signature,
        flv_header_has_video(*header) ? "true" : "false",
        flv_header_has_audio(*header) ? "true" : "false",
        header->version);
    return OK;
}

static int json_on_tag(flv_tag * tag, flv_parser * parser) {
    if (parser->user_data != NULL) {
        printf(",");
    }
    else {
        parser->user_data = tag;
    }

    printf("{\"type\":\"%s\",\"timestamp\":%i,\"dataSize\":%i",
        dump_string_get_tag_type(tag),
        flv_tag_get_timestamp(*tag),
        flv_tag_get_body_length(*tag));
    printf(",\"offset\":%" FILE_OFFSET_PRINTF_FORMAT "i,",
        parser->stream->current_tag_offset);

    return OK;
}

static int json_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    printf("\"videoData\":{\"codecID\":\"%s\"", dump_string_get_video_codec(vt));
    printf(",\"frameType\":\"%s\"}", dump_string_get_video_frame_type(vt));
    return OK;
}

static int json_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    printf("\"audioData\":{\"type\":\"%s\"", dump_string_get_sound_type(at));
    printf(",\"size\":\"%s\"", dump_string_get_sound_size(at));
    printf(",\"rate\":\"%s\"", dump_string_get_sound_rate(at));
    printf(",\"format\":\"%s\"}", dump_string_get_sound_format(at));
    return OK;
}

static int json_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    json_t * root;

    printf("\"scriptDataObject\":{\"name\":\"%s\",\"metadata\":", amf_string_get_bytes(name));
    root = NULL;
    /* dump AMF into JSON */
    amf_to_json(data, &root);
    /* print data */
    json_stream_output(stdout, root);
    /* cleanup */
    json_free_value(&root);
    printf("}");
    return OK;
}

static int json_on_prev_tag_size(uint32 size, flv_parser * parser) {
    printf("}");
    return OK;
}

static int json_on_stream_end(flv_parser * parser) {
    printf("]}");
    return OK;
}

/* JSON FLV file metadata dump callback */
static int json_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    flvmeta_opts * options = (flvmeta_opts*) parser->user_data;

    if (options->metadata_event == NULL) {
        if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
            dump_json_amf_data(data);
            return FLVMETA_DUMP_STOP_OK;
        }
    }
    else {
        if (!strcmp((char*)amf_string_get_bytes(name), options->metadata_event)) {
            dump_json_amf_data(data);
            return FLVMETA_DUMP_STOP_OK;
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

void dump_json_setup_file_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_header = json_on_header;
        parser->on_tag = json_on_tag;
        parser->on_audio_tag = json_on_audio_tag;
        parser->on_video_tag = json_on_video_tag;
        parser->on_metadata_tag = json_on_metadata_tag;
        parser->on_prev_tag_size = json_on_prev_tag_size;
        parser->on_stream_end = json_on_stream_end;
    }
}

int dump_json_amf_data(const amf_data * data) {
    json_t * root;

    root = NULL;
    /* dump AMF into JSON */
    amf_to_json(data, &root);
    /* print data */
    json_stream_output(stdout, root);
    /* cleanup */
    json_free_value(&root);

    printf("\n");

    return OK;
}
