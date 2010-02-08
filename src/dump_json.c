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

#include "dump_json.h"
#include "json.h"

#include <stdio.h>
#include <string.h>

/* JSON metadata dumping */
static void amf_to_json(amf_data * data, json_t ** object) {
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

int json_on_header(flv_header * header, flv_parser * parser) {
    printf("{\"magic\":\"%c%c%c\",\"hasVideo\":%s,\"hasAudio\":%s,\"version\":%i,\"tags\":[",
        header->signature[0], header->signature[1], header->signature[2],
        flv_header_has_video(*header) ? "true" : "false",
        flv_header_has_audio(*header) ? "true" : "false",
        header->version);
    return OK;
}

int json_on_tag(flv_tag * tag, flv_parser * parser) {
    char * str;
    
    if (parser->user_data != NULL) {
        printf(",");
    }
    else {
        parser->user_data = tag;
    }

    switch (tag->type) {
        case FLV_TAG_TYPE_AUDIO: str = "audio"; break;
        case FLV_TAG_TYPE_VIDEO: str = "video"; break;
        case FLV_TAG_TYPE_META: str = "scriptData"; break;
        default: str = "Unknown";
    }

    printf("{\"type\":\"%s\",\"timestamp\":%i,\"dataSize\":%i",
        str,
        flv_tag_get_timestamp(*tag),
        uint24_be_to_uint32(tag->body_length));
    printf(",\"offset\":%" FILE_OFFSET_PRINTF_FORMAT "i,",
        parser->stream->current_tag_offset);

    return OK;
}

int json_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    char * str;

    switch (flv_video_tag_codec_id(vt)) {
        case FLV_VIDEO_TAG_CODEC_JPEG: str = "JPEG"; break;
        case FLV_VIDEO_TAG_CODEC_SORENSEN_H263: str = "Sorenson H.263"; break;
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO: str = "Screen video"; break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6: str = "On2 VP6"; break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA: str = "On2 VP6 with alpha channel"; break;
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2: str = "Screen video version 2"; break;
        case FLV_VIDEO_TAG_CODEC_AVC: str = "AVC"; break;
        default: str = "Unknown";
    }
    printf("\"videoData\":{\"codecID\":\"%s\"", str);

    switch (flv_video_tag_frame_type(vt)) {
        case FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME: str = "keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME: str = "inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME: str = "disposable inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME: str = "generated keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME: str = "video info/command frame"; break;
        default: str = "Unknown";
    }
    printf(",\"frameType\":\"%s\"}", str);

    return OK;
}

int json_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    char * str;

    switch (flv_audio_tag_sound_type(at)) {
        case FLV_AUDIO_TAG_SOUND_TYPE_MONO: str = "mono"; break;
        case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: str = "stereo"; break;
        default: str = "Unknown";
    }
    printf("\"audioData\":{\"type\":\"%s\"", str);

    switch (flv_audio_tag_sound_size(at)) {
        case FLV_AUDIO_TAG_SOUND_SIZE_8: str = "8"; break;
        case FLV_AUDIO_TAG_SOUND_SIZE_16: str = "16"; break;
        default: str = "Unknown";
    }
    printf(",\"size\":\"%s\"", str);

    switch (flv_audio_tag_sound_rate(at)) {
        case FLV_AUDIO_TAG_SOUND_RATE_5_5: str = "5.5"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_11: str = "11"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_22: str = "22"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_44: str = "44"; break;
        default: str = "Unknown";
    }
    printf(",\"rate\":\"%s\"", str);

    switch (flv_audio_tag_sound_format(at)) {
        case FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM: str = "Linear PCM, platform endian"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_ADPCM: str = "ADPCM"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_MP3: str = "MP3"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM_LE: str = "Linear PCM, little-endian"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO: str = "Nellymoser 16-kHz mono"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO: str = "Nellymoser 8-kHz mono"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER: str = "Nellymoser"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_G711_A: str = "G.711 A-law logarithmic PCM"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU: str = "G.711 mu-law logarithmic PCM"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED: str = "reserved"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_AAC: str = "AAC"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_SPEEX: str = "Speex"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8: str = "MP3 8-Khz"; break;
        case FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC: str = "Device-specific sound"; break;
        default: str = "Unknown";
    }
    printf(",\"format\":\"%s\"}", str);

    return OK;
}

int json_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
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

int json_on_prev_tag_size(uint32 size, flv_parser * parser) {
    printf("}");
    return OK;
}

int json_on_stream_end(flv_parser * parser) {
    printf("]}");
    return OK;
}

/* JSON FLV file metadata dump callback */
int json_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    json_t * root;

    if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
        root = NULL;
        /* dump AMF into JSON */
        amf_to_json(data, &root);
        /* print data */
        json_stream_output(stdout, root);
        /* cleanup */
	    json_free_value(&root);
    }
    return FLVMETA_DUMP_STOP_OK;
}
