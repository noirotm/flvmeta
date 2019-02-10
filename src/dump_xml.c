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
static void xml_amf_data_dump(const amf_data * data, int qualified, int indent_level) {
    if (data != NULL) {
        amf_node * node;
        time_t time;
        struct tm * t;
        char datestr[128];
        int markers;
        char * ns;
        char ns_decl[50];

        /* namespace to use whether we're using qualified mode */
        ns = (qualified == 1) ? "amf:" : "";

        /* if indent_level is zero, that means we're at the root of the xml document
           therefore we need to insert the namespace definition */
        if (indent_level == 0) {
            sprintf(ns_decl, " xmlns%s=\"http://schemas.flvmeta.org/AMF0/1.0/\"", ns);
        }
        else {
            strcpy(ns_decl, "");
        }

        /* print indentation spaces */
        printf("%*s", indent_level * 2, "");

        switch (data->type) {
            case AMF_TYPE_NUMBER:
                printf("<%snumber%s value=\"%.12g\"/>\n", ns, ns_decl, data->number_data);
                break;
            case AMF_TYPE_BOOLEAN:
                printf("<%sboolean%s value=\"%s\"/>\n", ns, ns_decl, (data->boolean_data) ? "true" : "false");
                break;
            case AMF_TYPE_STRING:
                if (amf_string_get_size(data) > 0) {
                    printf("<%sstring%s>", ns, ns_decl);
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
                    printf("</%sstring>\n", ns);
                }
                else {
                    /* simplify empty xml element into a more compact form */
                    printf("<%sstring%s/>\n", ns, ns_decl);
                }
                break;
            case AMF_TYPE_OBJECT:
                if (amf_object_size(data) > 0) {
                    printf("<%sobject%s>\n", ns, ns_decl);
                    node = amf_object_first(data);
                    while (node != NULL) {
                        printf("%*s<%sentry name=\"%s\">\n", (indent_level + 1) * 2, "", ns, amf_string_get_bytes(amf_object_get_name(node)));
                        xml_amf_data_dump(amf_object_get_data(node), qualified, indent_level + 2);
                        node = amf_object_next(node);
                        printf("%*s</%sentry>\n", (indent_level + 1) * 2, "", ns);
                    }
                    printf("%*s</%sobject>\n", indent_level * 2, "", ns);
                }
                else {
                    /* simplify empty xml element into a more compact form */
                    printf("<%sobject%s/>\n", ns, ns_decl);
                }
                break;
            case AMF_TYPE_NULL:
                printf("<%snull%s/>\n", ns, ns_decl);
                break;
            case AMF_TYPE_UNDEFINED:
                printf("<%sundefined%s/>\n", ns, ns_decl);
                break;
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                if (amf_associative_array_size(data) > 0) {
                    printf("<%sassociativeArray%s>\n", ns, ns_decl);
                    node = amf_associative_array_first(data);
                    while (node != NULL) {
                        printf("%*s<%sentry name=\"%s\">\n", (indent_level + 1) * 2, "", ns, amf_string_get_bytes(amf_associative_array_get_name(node)));
                        xml_amf_data_dump(amf_associative_array_get_data(node), qualified, indent_level + 2);
                        node = amf_associative_array_next(node);
                        printf("%*s</%sentry>\n", (indent_level + 1) * 2, "", ns);
                    }
                    printf("%*s</%sassociativeArray>\n", indent_level * 2, "", ns);
                }
                else {
                    /* simplify empty xml element into a more compact form */
                    printf("<%sassociativeArray%s/>\n", ns, ns_decl);
                }
                break;
            case AMF_TYPE_ARRAY:
                if (amf_array_size(data) > 0) {
                    printf("<%sarray%s>\n", ns, ns_decl);
                    node = amf_array_first(data);
                    while (node != NULL) {
                        xml_amf_data_dump(amf_array_get(node), qualified, indent_level + 1);
                        node = amf_array_next(node);
                    }
                    printf("%*s</%sarray>\n", indent_level * 2, "", ns);
                }
                else {
                    /* simplify empty xml element into a more compact form */
                    printf("<%sarray%s/>\n", ns, ns_decl);
                }
                break;
            case AMF_TYPE_DATE:
                time = amf_date_to_time_t(data);
                tzset();
                t = localtime(&time);
                strftime(datestr, sizeof(datestr), "%Y-%m-%dT%H:%M:%S", t);
                printf("<%sdate%s value=\"%s\"/>\n", ns, ns_decl, datestr);
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
    printf("<flv xmlns=\"http://schemas.flvmeta.org/FLV/1.0/\" xmlns:amf=\"http://schemas.flvmeta.org/AMF0/1.0/\" hasVideo=\"%s\" hasAudio=\"%s\" version=\"%" PRI_BYTE "u\">\n",
        flv_header_has_video(*header) ? "true" : "false",
        flv_header_has_audio(*header) ? "true" : "false",
        header->version);
    return OK;
}

static int xml_on_tag(flv_tag * tag, flv_parser * parser) {
    printf("  <tag type=\"%s\" timestamp=\"%i\" dataSize=\"%i\"",
        dump_string_get_tag_type(tag),
        flv_tag_get_timestamp(*tag),
        flv_tag_get_body_length(*tag));
    printf(" offset=\"%" FILE_OFFSET_PRINTF_FORMAT "u\">\n",
        FILE_OFFSET_PRINTF_TYPE(parser->stream->current_tag_offset));

    return OK;
}

static int xml_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
    printf("    <videoData codecID=\"%s\"", dump_string_get_video_codec(vt));
    printf(" frameType=\"%s\"", dump_string_get_video_frame_type(vt));

    /* if AVC, detect frame type and composition time */
    if (flv_video_tag_codec_id(vt) == FLV_VIDEO_TAG_CODEC_AVC) {
        flv_avc_packet_type type;

        printf(">\n");

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_avc_packet_type)) < sizeof(flv_avc_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        printf("        <AVCData packetType=\"%s\"", dump_string_get_avc_packet_type(type));

        /* composition time */
        if (type == FLV_AVC_PACKET_TYPE_NALU) {
            uint24_be composition_time;

            if (flv_read_tag_body(parser->stream, &composition_time, sizeof(uint24_be)) < sizeof(uint24_be)) {
                return ERROR_INVALID_TAG;
            }

            printf(" compositionTimeOffset=\"%i\"", uint24_be_to_uint32(composition_time));
        }

        printf("/>\n");
        printf("    </videoData>\n");
    }
    else {
        printf("/>\n");
    }

    return OK;
}

static int xml_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    printf("    <audioData type=\"%s\"", dump_string_get_sound_type(at));
    printf(" size=\"%s\"", dump_string_get_sound_size(at));
    printf(" rate=\"%s\"", dump_string_get_sound_rate(at));
    printf(" format=\"%s\"", dump_string_get_sound_format(at));

    /* if AAC, detect packet type */
    if (flv_audio_tag_sound_format(at) == FLV_AUDIO_TAG_SOUND_FORMAT_AAC) {
        flv_aac_packet_type type;

        printf(">\n");

        /* packet type */
        if (flv_read_tag_body(parser->stream, &type, sizeof(flv_aac_packet_type)) < sizeof(flv_aac_packet_type)) {
            return ERROR_INVALID_TAG;
        }

        printf("        <AACData packetType=\"%s\"/>\n", dump_string_get_aac_packet_type(type));
        printf("    </audioData>\n");
    }
    else {
        printf("/>\n");
    }

    return OK;
}

static int xml_on_metadata_tag(flv_tag * tag, char * name, amf_data * data, flv_parser * parser) {
    printf("    <scriptDataObject name=\"%s\">\n", name);
    /* dump AMF data as XML, we start from level 3, meaning 6 indentations characters */
    xml_amf_data_dump(data, 1, 3);
    puts("    </scriptDataObject>");
    return OK;
}

static int xml_on_prev_tag_size(uint32 size, flv_parser * parser) {
    puts("  </tag>");
    return OK;
}

static int xml_on_stream_end(flv_parser * parser) {
    puts("</flv>");
    return OK;
}

/* XML FLV file metadata dump callbacks */
static int xml_on_metadata_tag_only(flv_tag * tag, char * name, amf_data * data, flv_parser * parser) {
    flvmeta_opts * options = (flvmeta_opts*) parser->user_data;

    if (options->metadata_event == NULL) {
        if (!strcmp(name, "onMetaData")) {
            dump_xml_amf_data(data);
            return FLVMETA_DUMP_STOP_OK;
        }
    }
    else {
        if (!strcmp(name, options->metadata_event)) {
            dump_xml_amf_data(data);
        }
    }
    return OK;
}

/* dumping functions */
void dump_xml_setup_metadata_dump(flv_parser * parser) {
    if (parser != NULL) {
        parser->on_metadata_tag = xml_on_metadata_tag_only;
    }
}

int dump_xml_file(flv_parser * parser, const flvmeta_opts * options) {
    parser->on_header = xml_on_header;
    parser->on_tag = xml_on_tag;
    parser->on_audio_tag = xml_on_audio_tag;
    parser->on_video_tag = xml_on_video_tag;
    parser->on_metadata_tag = xml_on_metadata_tag;
    parser->on_prev_tag_size = xml_on_prev_tag_size;
    parser->on_stream_end = xml_on_stream_end;

    return flv_parse(options->input_file, parser);
}

int dump_xml_amf_data(const amf_data * data) {
    puts("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>");
    xml_amf_data_dump(data, 0, 0);
    return OK;
}
