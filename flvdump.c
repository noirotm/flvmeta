/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007 Marc Noirot <marc.noirot AT gmail.com>

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
#include <stdio.h>
#include <string.h>

#include "flvmeta.h"
#include "flv.h"
#include "amf.h"

int main(int argc, char ** argv) {

    if (argc < 2) {
        fprintf(stderr, "FLVDump version %s - Copyright 2007 Marc Noirot\n\n",
            VERSION);
        fprintf(stderr, "Usage: flvdump in_file\n\n");
        fprintf(stderr, "This is free software; see the source for copying conditions. There is NO\n"
                        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
        fprintf(stderr, "Report bugs to <%s>\n\n", PACKAGE_BUGREPORT);
        return 1;
    }

    FILE * flv_in = fopen(argv[1], "rb");

    if (flv_in == NULL) {
        fprintf(stderr, "error opening %s.\n", argv[1]);
        return 2;
    }

    flv_header fh;

    fread(&fh, sizeof(fh), 1, flv_in);

    printf("Magic: %.3s\n", fh.signature);
    printf("Version: %d\n", fh.version);
    printf("Has audio: %s\n", flv_header_has_audio(fh) ? "yes" : "no");
    printf("Has video: %s\n", flv_header_has_video(fh) ? "yes" : "no");
    printf("Offset: %lu\n", swap_uint32(fh.offset));

    /* skip first empty previous tag size */
    fseek(flv_in, sizeof(uint32_be), SEEK_CUR);

    flv_tag ft;
    long offset;
    char * str;
    uint32 n = 1;
    uint32 prev_tag_size;
    while (!feof(flv_in)) {
        offset = ftell(flv_in);
        if (fread(&ft, sizeof(ft), 1, flv_in) == 0)
            break;

        printf("--- Tag #%lu at 0x%lX (%li) ---\n", n++, offset, offset);
        switch (ft.type) {
            case FLV_TAG_TYPE_AUDIO: str = "Audio"; break;
            case FLV_TAG_TYPE_VIDEO: str = "Video"; break;
            case FLV_TAG_TYPE_META: str = "Meta"; break;
            default: str = "Unknown";
        }
        printf("Tag type: %s\n", str);

        uint32 body_length = uint24_be_to_uint32(ft.body_length);
        printf("Body length: %lu\n", body_length);
        printf("Timestamp: %lu\n", uint24_be_to_uint32(ft.timestamp));

        if (ft.type == FLV_TAG_TYPE_AUDIO) {
            flv_audio_tag at;
            fread(&at, sizeof(at), 1, flv_in);

            switch (flv_audio_tag_sound_type(at)) {
                case FLV_AUDIO_TAG_SOUND_TYPE_MONO: str = "Mono"; break;
                case FLV_AUDIO_TAG_SOUND_TYPE_STEREO: str = "Stereo"; break;
                default: str = "Unknown";
            }
            printf("* Sound type: %s\n", str);

            switch (flv_audio_tag_sound_size(at)) {
                case FLV_AUDIO_TAG_SOUND_SIZE_8: str = "8 bits"; break;
                case FLV_AUDIO_TAG_SOUND_SIZE_16: str = "16 bits"; break;
                default: str = "Unknown";
            }
            printf("* Sound size: %s\n", str);

            switch (flv_audio_tag_sound_rate(at)) {
                case FLV_AUDIO_TAG_SOUND_RATE_5_5: str = "5.5 kHz"; break;
                case FLV_AUDIO_TAG_SOUND_RATE_11: str = "11 kHz"; break;
                case FLV_AUDIO_TAG_SOUND_RATE_22: str = "22 kHz"; break;
                case FLV_AUDIO_TAG_SOUND_RATE_44: str = "44 kHz"; break;
                default: str = "Unknown";
            }
            printf("* Sound rate: %s\n", str);

            switch (flv_audio_tag_sound_format(at)) {
                case FLV_AUDIO_TAG_SOUND_FORMAT_UNCOMPRESSED: str = "Uncompressed"; break;
                case FLV_AUDIO_TAG_SOUND_FORMAT_ADPCM: str = "ADPCM"; break;
                case FLV_AUDIO_TAG_SOUND_FORMAT_MP3: str = "MP3"; break;
                case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER8MONO: str = "Nellymoser 8kHz mono"; break;
                case FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER: str = "Nellymoser"; break;
                default: str = "Unknown";
            }
            printf("* Sound format: %s\n", str);

            body_length -= sizeof(at);
        }
        else if (ft.type == FLV_TAG_TYPE_VIDEO) {
            flv_video_tag vt;
            fread(&vt, sizeof(vt), 1, flv_in);

            switch (flv_video_tag_codec_id(vt)) {
                case FLV_VIDEO_TAG_CODEC_SORENSEN_H263: str = "Sorensen H.263"; break;
                case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO: str = "Screen Video"; break;
                case FLV_VIDEO_TAG_CODEC_ON2_VP6: str = "On2 VP6"; break;
                case FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA: str = "On2 VP6 Alpha"; break;
                case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2: str = "Screen Video V2"; break;
                default: str = "Unknown";
            }
            printf("* Video codec: %s\n", str);

            switch (flv_video_tag_frame_type(vt)) {
                case FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME: str = "keyframe"; break;
                case FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME: str = "inter frame"; break;
                case FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME: str = "disposable inter frame"; break;
                default: str = "Unknown";
            }
            printf("* Video frame type: %s\n", str);

            body_length -= sizeof(vt);
        }
        else if (ft.type == FLV_TAG_TYPE_META) {
            amf_data * data = amf_data_read(flv_in);
            printf("* Metadata event name: %s\n", amf_string_get_bytes(data));
            amf_data_free(data);

            data = amf_data_read(flv_in);
            printf("* Metadata contents: ");
            amf_data_dump(stdout, data, 0);
            printf("\n");
            amf_data_free(data);

            body_length = 0;
        }
        fseek(flv_in, (long)body_length, SEEK_CUR);
        if (fread(&prev_tag_size, sizeof(uint32_be), 1, flv_in) == 1) {
            printf("Previous tag size: %lu\n", swap_uint32(prev_tag_size));
        }
    }

    fclose(flv_in);
    return 0;
}
