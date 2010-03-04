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

#include "dump_xml.h"

#include <stdio.h>
#include <string.h>

/* does the given string have XML tag markers ? */
static int has_xml_markers(const char * str, int len) {
    int i;
    for (i = 0; i < len; i++) {
        if (str[i] == '<' || str[i] == '>') {
            return 1;
        }
    }
    return 0;
}

/* XML metadata dumping */
void xml_amf_data_dump(const amf_data * data, int indent_level) {
    if (data != NULL) {
        amf_node * node;
        time_t time;
        struct tm * t;
        char datestr[128];
        int markers;

        /* print indentation spaces */
        printf("%*s", indent_level * 2, "");

        switch (data->type) {
            case AMF_TYPE_NUMBER:
                printf("<amf:number value=\"%.12g\"/>\n", data->number_data);
                break;
            case AMF_TYPE_BOOLEAN:
                printf("<amf:boolean value=\"%s\"/>\n", (data->boolean_data) ? "true" : "false");
                break;
            case AMF_TYPE_STRING:
                printf("<amf:string>");
                /* check whether the string contains xml characters, if so, CDATA it */
                markers = has_xml_markers((char*)amf_string_get_bytes(data), amf_string_get_size(data));
                if (markers) {
                    printf("<![CDATA[");
                }
                /* do not print more than the actual length of string */
                printf("%.*s", (int)amf_string_get_size(data), amf_string_get_bytes(data));
                if (markers) {
                    printf("]]>");
                }
                puts("</amf:string>");
                break;
            case AMF_TYPE_OBJECT:
                puts("<amf:object>");
                node = amf_object_first(data);
                while (node != NULL) {
                    printf("%*s<amf:element name=\"%s\">\n", (indent_level + 1) * 2, "", amf_string_get_bytes(amf_object_get_name(node)));
                    xml_amf_data_dump(amf_object_get_data(node), indent_level + 2);
                    node = amf_object_next(node);
                    printf("%*s</amf:element>\n", (indent_level + 1) * 2, "");
                }
                printf("%*s</amf:object>\n", indent_level * 2, "");
                break;
            case AMF_TYPE_NULL:
                puts("<amf:null/>");
                break;
            case AMF_TYPE_UNDEFINED:
                puts("<amf:undefined/>");
                break;
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                puts("<amf:associativeArray>");
                node = amf_associative_array_first(data);
                while (node != NULL) {
                    printf("%*s<amf:element name=\"%s\">\n", (indent_level + 1) * 2, "", amf_string_get_bytes(amf_associative_array_get_name(node)));
                    xml_amf_data_dump(amf_associative_array_get_data(node), indent_level + 2);
                    node = amf_associative_array_next(node);
                    printf("%*s</amf:element>\n", (indent_level + 1) * 2, "");
                }
                printf("%*s</amf:associativeArray>\n", indent_level * 2, "");
                break;
            case AMF_TYPE_ARRAY:
                puts("<amf:array>");
                node = amf_array_first(data);
                while (node != NULL) {
                    xml_amf_data_dump(amf_array_get(node), indent_level + 1);
                    node = amf_array_next(node);
                }
                printf("%*s</amf:array>\n", indent_level * 2, "");
                break;
            case AMF_TYPE_DATE:
                time = amf_date_to_time_t(data);
                tzset();
                t = localtime(&time);
                strftime(datestr, sizeof(datestr), "%Y-%m-%dT%H:%M:%S", t);
                printf("<amf:date value=\"%s\"/>\n", datestr);
                break;
            case AMF_TYPE_XML: break;
            case AMF_TYPE_CLASS: break;
            default: break;
        }
    }
}

/* XML FLV file full dump callbacks */

static int xml_on_header(flv_header * header, flv_parser * parser) {
    puts("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>");
    printf("<flv xmlns=\"http://schemas.flvmeta.org/FLV/\" xmlns:amf=\"http://schemas.flvmeta.org/AMF0/\" hasVideo=\"%s\" hasAudio=\"%s\" version=\"%i\">\n",
        flv_header_has_video(*header) ? "true" : "false",
        flv_header_has_audio(*header) ? "true" : "false",
        header->version);
    return OK;
}

static int xml_on_tag(flv_tag * tag, flv_parser * parser) {
    char * str;

    switch (tag->type) {
        case FLV_TAG_TYPE_AUDIO: str = "audio"; break;
        case FLV_TAG_TYPE_VIDEO: str = "video"; break;
        case FLV_TAG_TYPE_META: str = "scriptData"; break;
        default: str = "Unknown";
    }

    printf("  <tag type=\"%s\" timestamp=\"%i\" dataSize=\"%i\"",
        str,
        flv_tag_get_timestamp(*tag),
        uint24_be_to_uint32(tag->body_length));
    printf(" offset=\"%" FILE_OFFSET_PRINTF_FORMAT "i\">\n",
        parser->stream->current_tag_offset);

    return OK;
}

static int xml_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
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
    printf("    <videoData codecID=\"%s\"", str);

    switch (flv_video_tag_frame_type(vt)) {
        case FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME: str = "keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME: str = "inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME: str = "disposable inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME: str = "generated keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME: str = "video info/command frame"; break;
        default: str = "Unknown";
    }
    printf(" frameType=\"%s\"/>\n", str);

    return OK;
}

static int xml_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    char * str;

    switch (flv_audio_tag_sound_type(at)) {
        case FLV_AUDIO_TAG_SOUND_TYPE_MONO: str = "mono"; break;
        case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: str = "stereo"; break;
        default: str = "Unknown";
    }
    printf("    <audioData type=\"%s\"", str);

    switch (flv_audio_tag_sound_size(at)) {
        case FLV_AUDIO_TAG_SOUND_SIZE_8: str = "8"; break;
        case FLV_AUDIO_TAG_SOUND_SIZE_16: str = "16"; break;
        default: str = "Unknown";
    }
    printf(" size=\"%s\"", str);

    switch (flv_audio_tag_sound_rate(at)) {
        case FLV_AUDIO_TAG_SOUND_RATE_5_5: str = "5.5"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_11: str = "11"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_22: str = "22"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_44: str = "44"; break;
        default: str = "Unknown";
    }
    printf(" rate=\"%s\"", str);

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
    printf(" format=\"%s\"/>\n", str);

    return OK;
}

static int xml_on_metadata_tag(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    printf("    <scriptDataObject name=\"%s\">\n", amf_string_get_bytes(name));
    /* dump AMF data as XML, we start from level 3, meaning 6 indentations characters */
    xml_amf_data_dump(data, 3);
    puts("    </scriptDataObject>");
    return OK;
}

static int xml_on_prev_tag_size(uint32 size, flv_parser * parser) {
    puts("  </tag>");
    return OK;
}

static int xml_on_stream_end(flv_parser * parser) {
    printf("</flv>");
    return OK;
}

/* XML FLV file metadata dump callbacks */
static int xml_on_metadata_tag_only(flv_tag * tag, amf_data * name, amf_data * data, flv_parser * parser) {
    if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
        dump_xml_amf_data(data);
        return FLVMETA_DUMP_STOP_OK;
    }
    return OK;
}

/* dumping functions */
void dump_xml_setup_metadata_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_metadata_tag = xml_on_metadata_tag_only;
    }
}

void dump_xml_setup_file_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_header = xml_on_header;
        parser->on_tag = xml_on_tag;
        parser->on_audio_tag = xml_on_audio_tag;
        parser->on_video_tag = xml_on_video_tag;
        parser->on_metadata_tag = xml_on_metadata_tag;
        parser->on_prev_tag_size = xml_on_prev_tag_size;
        parser->on_stream_end = xml_on_stream_end;
    }
}

int dump_xml_amf_data(const amf_data * data) {
    puts("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>");
    puts("<scriptDataObject name=\"onMetaData\" xmlns=\"http://schemas.flvmeta.org/FLV/\" xmlns:amf=\"http://schemas.flvmeta.org/AMF0/\">");
    /* dump AMF data as XML, we start from level 3, meaning 6 indentations characters */
    xml_amf_data_dump(data, 1);
    printf("</scriptDataObject>\n");

    return OK;
}
