/*
    $Id: dump_raw.c 170 2010-02-12 16:05:27Z marc.noirot $

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
    char * str;
    int * n;
    
    switch (tag->type) {
        case FLV_TAG_TYPE_AUDIO: str = "audio"; break;
        case FLV_TAG_TYPE_VIDEO: str = "video"; break;
        case FLV_TAG_TYPE_META: str = "scriptData"; break;
        default: str = "Unknown";
    }

    n = ((int*)parser->user_data);
    ++(*n);

    printf("--- Tag #%u at 0x%" FILE_OFFSET_PRINTF_FORMAT "X", *n, parser->stream->current_tag_offset);
    printf(" (%" FILE_OFFSET_PRINTF_FORMAT "i) ---\n", parser->stream->current_tag_offset);
    printf("Tag type: %s\n", str);
    printf("Body length: %u\n", uint24_be_to_uint32(tag->body_length));
    printf("Timestamp: %u\n", flv_tag_get_timestamp(*tag));

    return OK;
}

static int raw_on_video_tag(flv_tag * tag, flv_video_tag vt, flv_parser * parser) {
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
    printf("* Video codec: %s\n", str);

    switch (flv_video_tag_frame_type(vt)) {
        case FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME: str = "keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME: str = "inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME: str = "disposable inter frame"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME: str = "generated keyframe"; break;
        case FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME: str = "video info/command frame"; break;
        default: str = "Unknown";
    }
    printf("* Video frame type: %s\n", str);

    return OK;
}

static int raw_on_audio_tag(flv_tag * tag, flv_audio_tag at, flv_parser * parser) {
    char * str;

    switch (flv_audio_tag_sound_type(at)) {
        case FLV_AUDIO_TAG_SOUND_TYPE_MONO: str = "mono"; break;
        case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: str = "stereo"; break;
        default: str = "Unknown";
    }
    printf("* Sound type: %s\n", str);

    switch (flv_audio_tag_sound_size(at)) {
        case FLV_AUDIO_TAG_SOUND_SIZE_8: str = "8"; break;
        case FLV_AUDIO_TAG_SOUND_SIZE_16: str = "16"; break;
        default: str = "Unknown";
    }
    printf("* Sound size: %s\n", str);

    switch (flv_audio_tag_sound_rate(at)) {
        case FLV_AUDIO_TAG_SOUND_RATE_5_5: str = "5.5"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_11: str = "11"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_22: str = "22"; break;
        case FLV_AUDIO_TAG_SOUND_RATE_44: str = "44"; break;
        default: str = "Unknown";
    }
    printf("* Sound rate: %s\n", str);

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
    printf("* Sound format: %s\n", str);

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
        amf_data_dump(stdout, data, 0);
        printf("\n");
    }
    return FLVMETA_DUMP_STOP_OK;
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
