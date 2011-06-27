/*
    $Id: dump.c 231 2011-06-27 13:46:19Z marc.noirot $

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/
#include "flvmeta.h"
#include "dump_json.h"
#include "dump_raw.h"
#include "dump_xml.h"
#include "dump_yaml.h"

#include <string.h>

const char * dump_string_get_tag_type(flv_tag * tag) {
    switch (tag->type) {
        case FLV_TAG_TYPE_AUDIO: return "audio";
        case FLV_TAG_TYPE_VIDEO: return "video";
        case FLV_TAG_TYPE_META: return "scriptData";
        default: return "Unknown";
    }
}

const char * dump_string_get_video_codec(flv_video_tag tag) {
    switch (flv_video_tag_codec_id(tag)) {
        case FLV_VIDEO_TAG_CODEC_JPEG: return "JPEG";
        case FLV_VIDEO_TAG_CODEC_SORENSEN_H263: return "Sorenson H.263";
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO: return "Screen video";
        case FLV_VIDEO_TAG_CODEC_ON2_VP6: return "On2 VP6";
        case FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA: return "On2 VP6 with alpha channel";
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2: return "Screen video version 2";
        case FLV_VIDEO_TAG_CODEC_AVC: return "AVC";
        default: return "Unknown";
    }
}

const char * dump_string_get_video_frame_type(flv_video_tag tag) {
    switch (flv_video_tag_frame_type(tag)) {
        case FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME: return "keyframe";
        case FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME: return "inter frame";
        case FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME: return "disposable inter frame";
        case FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME: return "generated keyframe";
        case FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME: return "video info/command frame";
        default: return "Unknown";
    }
}

const char * dump_string_get_sound_type(flv_audio_tag tag) {
    switch (flv_audio_tag_sound_type(tag)) {
        case FLV_AUDIO_TAG_SOUND_TYPE_MONO: return "mono";
        case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: return "stereo";
        default: return "Unknown";
    }
}

const char * dump_string_get_sound_size(flv_audio_tag tag) {
    switch (flv_audio_tag_sound_size(tag)) {
        case FLV_AUDIO_TAG_SOUND_SIZE_8: return "8";
        case FLV_AUDIO_TAG_SOUND_SIZE_16: return "16";
        default: return "Unknown";
    }
}

const char * dump_string_get_sound_rate(flv_audio_tag tag) {
    switch (flv_audio_tag_sound_rate(tag)) {
        case FLV_AUDIO_TAG_SOUND_RATE_5_5: return "5.5";
        case FLV_AUDIO_TAG_SOUND_RATE_11: return "11";
        case FLV_AUDIO_TAG_SOUND_RATE_22: return "22";
        case FLV_AUDIO_TAG_SOUND_RATE_44: return "44";
        default: return "Unknown";
    }
}

const char * dump_string_get_sound_format(flv_audio_tag tag) {
    switch (flv_audio_tag_sound_format(tag)) {
        case FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM: return "Linear PCM, platform endian";
        case FLV_AUDIO_TAG_SOUND_FORMAT_ADPCM: return "ADPCM";
        case FLV_AUDIO_TAG_SOUND_FORMAT_MP3: return "MP3";
        case FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM_LE: return "Linear PCM, little-endian";
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO: return "Nellymoser 16-kHz mono";
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO: return "Nellymoser 8-kHz mono";
        case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER: return "Nellymoser";
        case FLV_AUDIO_TAG_SOUND_FORMAT_G711_A: return "G.711 A-law logarithmic PCM";
        case FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU: return "G.711 mu-law logarithmic PCM";
        case FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED: return "reserved";
        case FLV_AUDIO_TAG_SOUND_FORMAT_AAC: return "AAC";
        case FLV_AUDIO_TAG_SOUND_FORMAT_SPEEX: return "Speex";
        case FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8: return "MP3 8-Khz";
        case FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC: return "Device-specific sound";
        default: return "Unknown";
    }
}

/* dump metadata from a FLV file */
int dump_metadata(const flvmeta_opts * options) {
    int retval;
    flv_parser parser;
    memset(&parser, 0, sizeof(flv_parser));
    parser.user_data = (void*)options;

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
