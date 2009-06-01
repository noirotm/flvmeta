/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008 Marc Noirot <marc.noirot AT gmail.com>

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
#include "flv.h"
#include "amf.h"

#include <stdio.h>
#include <string.h>

/* XML metadata dumping */
typedef struct __xml_dump_status {
    uint8 level;
} xml_dump_status;


/* XML FLV file full dump callbacks */

static int xml_on_header(flv_header * header, flv_parser * parser) {
    puts("<?xml version=\"1.0\" encoding=\"utf-8\" standalone=\"yes\"?>");
    printf("<flv hasVideo=\"%s\" hasAudio=\"%s\" version=\"%i\">\n",
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
        case FLV_TAG_TYPE_META: str = "metadata"; break;
        default: str = "Unknown";
    }

    printf("  <tag type=\"%s\" timestamp=\"%i\" bodyLength=\"%i\"",
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
        case FLV_VIDEO_TAG_CODEC_SORENSEN_H263: str = "Sorensen H.263"; break;
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO: str = "Screen Video"; break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6: str = "On2 VP6"; break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA: str = "On2 VP6 Alpha"; break;
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2: str = "Screen Video V2"; break;
        case FLV_VIDEO_TAG_CODEC_AVC: str = "AVC"; break;
        default: str = "Unknown";
    }
    printf("    <videoData codec=\"%s\"", str);

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
        case FLV_AUDIO_TAG_SOUND_TYPE_MONO: str = "Mono"; break;
        case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: str = "Stereo"; break;
        default: str = "Unknown";
    }
    printf("    <audioData type=\"%s\"", str);

    switch (flv_audio_tag_sound_size(at)) {
        case FLV_AUDIO_TAG_SOUND_SIZE_8: str = "8 bits"; break;
        case FLV_AUDIO_TAG_SOUND_SIZE_16: str = "16 bits"; break;
        default: str = "Unknown";
    }
    printf(" size=\"%s\"", str);

    switch (flv_audio_tag_sound_rate(at)) {
        case FLV_AUDIO_TAG_SOUND_RATE_5_5: str = "5.5 kHz"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_11: str = "11 kHz"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_22: str = "22 kHz"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_44: str = "44 kHz"; break;
        default: str = "Unknown";
    }
    printf(" rate=\"%s\"", str);

    switch (flv_audio_tag_sound_format(at)) {
        case FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM: str = "Linear PCM"; break;
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


/* dump metadata from a FLV file */
int dump_metadata(const flvmeta_opts * options) {
    /*flv_parser parser;
    
    switch (options->dump_format) {

    }*/

    return OK;
}

/* dump the full contents of an FLV file */
int dump_flv_file(const flvmeta_opts * options) {
    flv_parser parser;

    switch (options->dump_format) {
        case FLVMETA_FORMAT_XML:
            memset(&parser, 0, sizeof(flv_parser));
            parser.on_header = xml_on_header;
            parser.on_tag = xml_on_tag;
            parser.on_audio_tag = xml_on_audio_tag;
            parser.on_video_tag = xml_on_video_tag;
            parser.on_metadata_tag = xml_on_metadata_tag;
            parser.on_prev_tag_size = xml_on_prev_tag_size;
            parser.on_stream_end = xml_on_stream_end;
            break;
    }

    return flv_parse(options->input_file, &parser);
}
