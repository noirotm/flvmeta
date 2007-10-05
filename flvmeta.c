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
#include <time.h>

#include "flvmeta.h"
#include "flv.h"
#include "amf.h"

#define OK                  0
#define ERROR_OPEN_READ     1
#define ERROR_OPEN_WRITE    2
#define ERROR_NO_FLV        3
#define ERROR_EOF           4
#define ERROR_INVALID_TAG   5
#define ERROR_WRITE         6

typedef struct __flv_info {
    flv_header header;
    uint8 have_video;
    uint8 have_audio;
    uint32 video_width;
    uint32 video_height;
    uint8 video_codec;
    uint32 video_frames_number;
    uint8 audio_codec;
    uint8 audio_size;
    uint8 audio_rate;
    uint8 audio_stereo;
    uint32 video_data_size;
    uint32 audio_data_size;
    uint32 meta_data_size;
    uint32 real_video_data_size;
    uint32 real_audio_data_size;
    uint32 video_first_timestamp;
    uint32 audio_first_timestamp;
    uint8 can_seek_to_end;
    uint8 have_keyframes;
    uint32 last_keyframe_timestamp;
    uint32 on_metadata_size;
    uint32 first_data_tag_offset; /* after onMetaData */
    uint32 biggest_tag_body_size;
    uint32 last_timestamp;
    uint32 video_frame_duration;
    uint32 audio_frame_duration;
    uint32 total_prev_tags_size;
    uint8 have_on_last_second;
    amf_data * keyframes;
    amf_data * timestamps;
    amf_data * filepositions;
} flv_info;

typedef struct __flv_metadata {
    amf_data * on_last_second_name;
    amf_data * on_last_second;
    amf_data * on_metadata_name;
    amf_data * on_metadata;
} flv_metadata;


/*
    compute Sorensen H.263 video size
*/
size_t compute_h263_size(FILE * flv_in, flv_info * info) {
    byte header[9];
    size_t bytes_read = fread(header, 1, 9, flv_in);
    if (bytes_read == 9) {
        uint32 psc = uint24_be_to_uint32(*(uint24_be *)(header)) >> 7;
        if (psc == 1) {
            uint32 psize = ((header[3] & 0x03) << 1) + ((header[4] >> 7) & 0x01);
            switch (psize) {
                case 0:
                    info->video_width  = ((header[4] & 0x7f) << 1) + ((header[5] >> 7) & 0x01);
                    info->video_height = ((header[5] & 0x7f) << 1) + ((header[6] >> 7) & 0x01);
                    break;
                case 1:
                    info->video_width  = ((header[4] & 0x7f) << 9) + (header[5] << 1) + ((header[6] >> 7) & 0x01);
                    info->video_height = ((header[6] & 0x7f) << 9) + (header[7] << 1) + ((header[8] >> 7) & 0x01);
                    break;
                case 2:
                    info->video_width  = 352;
                    info->video_height = 288;
                    break;
                case 3:
                    info->video_width  = 176;
                    info->video_height = 144;
                    break;
                case 4:
                    info->video_width  = 128;
                    info->video_height = 96;
                    break;
                case 5:
                    info->video_width  = 320;
                    info->video_height = 240;
                    break;
                case 6:
                    info->video_width  = 160;
                    info->video_height = 120;
                    break;
                default:
                    break;
            }
        }
    }
    return bytes_read;
}

/*
    compute Screen video size
*/
size_t compute_screen_size(FILE * flv_in, flv_info * info) {
    byte header[4];
    size_t bytes_read = fread(header, 1, 4, flv_in);
    if (bytes_read == 4) {
        info->video_width  = ((header[0] & 0x0f) << 8) + header[1];
        info->video_height = ((header[2] & 0x0f) << 8) + header[3];
    }
    return bytes_read;
}

/*
    compute On2 VP6 video size
*/
size_t compute_vp6_size(FILE * flv_in, flv_info * info) {
    byte header[5];
    size_t bytes_read = fread(header, 1, 5, flv_in);
    if (bytes_read == 5) {
        info->video_width  = (header[4] << 4) - (header[0] >> 4);
	    info->video_height = (header[3] << 4) - (header[0] & 0x0f);
    }
    return bytes_read;
}

/*
    compute On2 VP6 with Alpha video size
*/
size_t compute_vp6_alpha_size(FILE * flv_in, flv_info * info) {
    byte header[8];
    size_t bytes_read = fread(header, 1, 8, flv_in);
    if (bytes_read == 8) {
        info->video_width  = (header[7] << 4) - (header[0] >> 4);
	    info->video_height = (header[6] << 4) - (header[0] & 0x0f);
    }
    return bytes_read;
}

/*
    compute video width and height from the first video frame
*/
size_t compute_video_size(FILE * flv_in, flv_info * info) {
    size_t bytes_read = 0;
    switch (info->video_codec) {
        case FLV_VIDEO_TAG_CODEC_SORENSEN_H263:
            bytes_read = compute_h263_size(flv_in, info);
            break;
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO:
        case FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2:
            bytes_read = compute_screen_size(flv_in, info);
            break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6:
            bytes_read = compute_vp6_size(flv_in, info);
            break;
        case FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA:
            bytes_read = compute_vp6_alpha_size(flv_in, info);
            break;
    }
    return bytes_read;
}

/*
    read the flv file thoroughly to get all necessary information.

    we need to check :
    - timestamp of first audio for audio delay
    - whether we have audio and video
    - first frames codecs (audio, video)
    - total audio and video data sizes
    - keyframe offsets and timestamps
    - whether the last video frame is a keyframe
    - last keyframe timestamp
    - onMetaData tag total size
    - total tags size
    - first tag after onMetaData offset
    - last timestamp
    - real video data size, number of frames, duration to compute framerate and video data rate
    - real audio data size, duration to compute audio data rate
    - video headers to find width and height. (depends on the encoding)
*/
int get_flv_info(FILE * flv_in, flv_info * info) {
    info->have_video = 0;
    info->have_audio = 0;
    info->video_width = 0;
    info->video_height = 0;
    info->video_codec = 0;
    info->video_frames_number = 0;
    info->audio_codec = 0;
    info->audio_size = 0;
    info->audio_rate = 0;
    info->audio_stereo = 0;
    info->video_data_size = 0;
    info->audio_data_size = 0;
    info->meta_data_size = 0;
    info->real_video_data_size = 0;
    info->real_audio_data_size = 0;
    info->video_first_timestamp = 0;
    info->audio_first_timestamp = 0;
    info->can_seek_to_end = 0;
    info->have_keyframes = 0;
    info->last_keyframe_timestamp = 0;
    info->on_metadata_size = 0;
    info->first_data_tag_offset = 0;
    info->biggest_tag_body_size = 0;
    info->last_timestamp = 0;
    info->video_frame_duration = 0;
    info->audio_frame_duration = 0;
    info->total_prev_tags_size = 0;
    info->have_on_last_second = 0;
    info->keyframes = NULL;
    info->timestamps = NULL;
    info->filepositions = NULL;

    /*
        read FLV header
    */

    if (fread(&(info->header), sizeof(flv_header), 1, flv_in) < 1) {
        return ERROR_NO_FLV;
    }

    if (info->header.signature[0] != 'F' || info->header.signature[1] != 'L' || info->header.signature[2] != 'V') {
        return ERROR_NO_FLV;
    }

    info->keyframes = amf_object_new();
    info->timestamps = amf_array_new();
    info->filepositions = amf_array_new();
    amf_object_add(info->keyframes, amf_str("times"), info->timestamps);
    amf_object_add(info->keyframes, amf_str("filepositions"), info->filepositions);

    /* skip first empty previous tag size */
    fseek(flv_in, sizeof(uint32_be), SEEK_CUR);
    info->total_prev_tags_size += sizeof(uint32_be);

    while (!feof(flv_in)) {
        flv_tag ft;
        uint32 offset;
        uint32 body_length;
        uint32 timestamp;

        offset = ftell(flv_in);
        if (fread(&ft, sizeof(flv_tag), 1, flv_in) == 0) {
            break;
        }

        body_length = uint24_be_to_uint32(ft.body_length);
        timestamp = uint24_be_to_uint32(ft.timestamp);

        if (info->biggest_tag_body_size < body_length) {
            info->biggest_tag_body_size = body_length;
        }
        info->last_timestamp = timestamp;

        /* address of first non metadata tag */
        if (ft.type != FLV_TAG_TYPE_META && info->first_data_tag_offset == 0) {
            info->first_data_tag_offset = offset;
        }

        if (ft.type == FLV_TAG_TYPE_META) {
            amf_data * tag_name = amf_data_read(flv_in);
            if (tag_name == NULL) {
                return ERROR_EOF;
            }

            /* just ignore metadata that don't have a proper name */
            if (amf_data_get_type(tag_name) == AMF_TYPE_STRING) {
                char * name = (char *)amf_string_get_bytes(tag_name);
                size_t len = (size_t)amf_string_get_size(tag_name);

                if (!strncmp(name, "onMetaData", len)) {
                    info->on_metadata_size = body_length + sizeof(flv_tag) + sizeof(uint32_be);
                }
                else {
                    if (!strncmp(name, "onLastSecond", len)) {
                        info->have_on_last_second = 1;
                    }
                    info->meta_data_size += (body_length + sizeof(flv_tag));
                    info->total_prev_tags_size += sizeof(uint32_be);
                }
            }
            else {
                info->meta_data_size += (body_length + sizeof(flv_tag));
                info->total_prev_tags_size += sizeof(uint32_be);
            }

            body_length -= (uint32)amf_data_size(tag_name);
            amf_data_free(tag_name);
        }
        else if (ft.type == FLV_TAG_TYPE_VIDEO) {
            flv_video_tag vt;
            size_t bytes_read = 0;

            if (fread(&vt, sizeof(flv_video_tag), 1, flv_in) == 0) {
                return ERROR_EOF;
            }

            if (info->have_video != 1) {
                info->have_video = 1;
                info->video_codec = flv_video_tag_codec_id(vt);
                info->video_first_timestamp = timestamp;

                /* todo: read first video frame to get critical info */
                bytes_read = compute_video_size(flv_in, info);
            }

            /* add keyframe to list */
            if (flv_video_tag_frame_type(vt) == FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME) {
                if (!info->have_keyframes) {
                    info->have_keyframes = 1;
                }
                info->last_keyframe_timestamp = timestamp;
                amf_array_push(info->timestamps, amf_number_new(timestamp / 1000.0));
                amf_array_push(info->filepositions, amf_number_new(offset));

                /* is last frame a key frame ? if so, we can seek to end */
                info->can_seek_to_end = 1;
            }
            else {
                info->can_seek_to_end = 0;
            }

            info->video_frames_number++;

            /*
                we assume all video frames have the same size as the first one: probably bogus
                but only used in case there's no audio in the file
            */
            if (info->video_frame_duration == 0 && timestamp != 0) {
                info->video_frame_duration = timestamp;
            }

            info->video_data_size += (body_length + sizeof(flv_tag));
            info->real_video_data_size += (body_length - 1);

            body_length -= (uint32)(sizeof(flv_video_tag) + bytes_read);
            info->total_prev_tags_size += sizeof(uint32_be);
        }
        else if (ft.type == FLV_TAG_TYPE_AUDIO) {
            flv_audio_tag at;
            
            if (fread(&at, sizeof(flv_audio_tag), 1, flv_in) == 0) {
                return ERROR_EOF;
            }
            
            if (info->have_audio != 1) {
                info->have_audio = 1;
                info->audio_codec = flv_audio_tag_sound_format(at);
                info->audio_rate = flv_audio_tag_sound_rate(at);
                info->audio_size = flv_audio_tag_sound_size(at);
                info->audio_stereo = flv_audio_tag_sound_type(at);
                info->audio_first_timestamp = timestamp;
            }
            /* we assume all audio frames have the same size as the first one */
            if (info->audio_frame_duration == 0 && timestamp != 0) {
                info->audio_frame_duration = timestamp;
            }

            info->audio_data_size += (body_length + sizeof(flv_tag));
            info->real_audio_data_size += (body_length - 1);

            body_length -= sizeof(flv_audio_tag);
            info->total_prev_tags_size += sizeof(uint32_be);
        }
        else {
            return ERROR_INVALID_TAG;
        }

        body_length += sizeof(uint32); /* skip tag size */
        fseek(flv_in, (long)body_length, SEEK_CUR);
    }

    return OK;
}

/*
    compute the metadata
*/
void compute_metadata(const flv_info * info, flv_metadata * meta) {
    meta->on_last_second_name = amf_str("onLastSecond");
    meta->on_last_second = amf_associative_array_new();
    meta->on_metadata_name = amf_str("onMetaData");
    meta->on_metadata = amf_associative_array_new();

    amf_associative_array_add(meta->on_metadata, amf_str("hasMetadata"), amf_boolean_new(1));
    amf_associative_array_add(meta->on_metadata, amf_str("hasVideo"), amf_boolean_new(info->have_video));
    amf_associative_array_add(meta->on_metadata, amf_str("hasAudio"), amf_boolean_new(info->have_audio));

    number64 duration;
    if (info->have_audio) {
        duration = (info->last_timestamp + info->audio_frame_duration) / 1000.0;
    }
    else {
        duration = (info->last_timestamp + info->video_frame_duration) / 1000.0;
    }
    amf_associative_array_add(meta->on_metadata, amf_str("duration"), amf_number_new(duration));

    amf_associative_array_add(meta->on_metadata, amf_str("lasttimestamp"), amf_number_new(info->last_timestamp / 1000.0));
    amf_associative_array_add(meta->on_metadata, amf_str("lastkeyframetimestamp"), amf_number_new(info->last_keyframe_timestamp / 1000.0));
    amf_associative_array_add(meta->on_metadata, amf_str("width"), amf_number_new(info->video_width));
    amf_associative_array_add(meta->on_metadata, amf_str("height"), amf_number_new(info->video_height));

    number64 video_data_rate = ((info->real_video_data_size / 1024.0) * 8.0) / duration;
    amf_associative_array_add(meta->on_metadata, amf_str("videodatarate"), amf_number_new(video_data_rate));
    
    number64 framerate = info->video_frames_number / duration;
    amf_associative_array_add(meta->on_metadata, amf_str("framerate"), amf_number_new(framerate));

    if (info->have_audio) {
        number64 audio_data_rate = ((info->real_audio_data_size / 1024.0) * 8.0) / duration;
        amf_associative_array_add(meta->on_metadata, amf_str("audiodatarate"), amf_number_new(audio_data_rate));
        
        number64 audio_khz = 0.0;
        switch (info->audio_rate) {
            case FLV_AUDIO_TAG_SOUND_RATE_5_5: audio_khz = 5500.0; break;
            case FLV_AUDIO_TAG_SOUND_RATE_11:  audio_khz = 11000.0; break;
            case FLV_AUDIO_TAG_SOUND_RATE_22:  audio_khz = 22000.0; break;
            case FLV_AUDIO_TAG_SOUND_RATE_44:  audio_khz = 44000.0; break;
        }
        amf_associative_array_add(meta->on_metadata, amf_str("audiosamplerate"), amf_number_new(audio_khz));
        number64 audio_sample_rate = 0.0;
        switch (info->audio_size) {
            case FLV_AUDIO_TAG_SOUND_SIZE_8:  audio_sample_rate = 8.0; break;
            case FLV_AUDIO_TAG_SOUND_SIZE_16: audio_sample_rate = 16.0; break;
        }
        amf_associative_array_add(meta->on_metadata, amf_str("audiosamplesize"), amf_number_new(audio_sample_rate));
        amf_associative_array_add(meta->on_metadata, amf_str("stereo"), amf_boolean_new(info->audio_stereo == FLV_AUDIO_TAG_SOUND_TYPE_STEREO));
    }

    /* to be computed later */
    amf_data * amf_total_filesize = amf_number_new(0);
    amf_associative_array_add(meta->on_metadata, amf_str("filesize"), amf_total_filesize);

    if (info->have_video) {
        amf_associative_array_add(meta->on_metadata, amf_str("videosize"), amf_number_new((number64)info->video_data_size));
    }
    if (info->have_audio) {
        amf_associative_array_add(meta->on_metadata, amf_str("audiosize"), amf_number_new((number64)info->audio_data_size));
    }

    /* to be computed later */
    amf_data * amf_total_data_size = amf_number_new(0);
    amf_associative_array_add(meta->on_metadata, amf_str("datasize"), amf_total_data_size);

    amf_associative_array_add(meta->on_metadata, amf_str("metadatacreator"), amf_str(LONG_COPYRIGHT_STR));

    tzset();
    amf_associative_array_add(meta->on_metadata, amf_str("metadatadate"), amf_date_new((number64)(time(NULL)+daylight*3600)*1000, -(sint16)timezone/60));
    if (info->have_audio) {
        amf_associative_array_add(meta->on_metadata, amf_str("audiocodecid"), amf_number_new((number64)info->audio_codec));
    }
    if (info->have_video) {
        amf_associative_array_add(meta->on_metadata, amf_str("videocodecid"), amf_number_new((number64)info->video_codec));
    }
    if (info->have_audio && info->have_video) {
        number64 audio_delay = ((sint32)info->audio_first_timestamp - (sint32)info->video_first_timestamp) / 1000.0;
        amf_associative_array_add(meta->on_metadata, amf_str("audiodelay"), amf_number_new((number64)audio_delay));
    }
    amf_associative_array_add(meta->on_metadata, amf_str("canSeekToEnd"), amf_boolean_new(info->can_seek_to_end));
    amf_associative_array_add(meta->on_metadata, amf_str("hasCuePoints"), amf_boolean_new(0));
    amf_associative_array_add(meta->on_metadata, amf_str("cuePoints"), amf_array_new());
    amf_associative_array_add(meta->on_metadata, amf_str("hasKeyframes"), amf_boolean_new(info->have_keyframes));
    amf_associative_array_add(meta->on_metadata, amf_str("keyframes"), info->keyframes);

    /*
        When we know the final size, we can recompute te offsets for the filepositions, and the final datasize.
    */
    uint32 new_on_metadata_size = sizeof(flv_tag) + sizeof(uint32_be) +
        (uint32)(amf_data_size(meta->on_metadata_name) + amf_data_size(meta->on_metadata));
    uint32 on_last_second_size = (uint32)(amf_data_size(meta->on_last_second_name) + amf_data_size(meta->on_last_second));

    amf_node * node_t = amf_array_first(info->timestamps);
    amf_node * node_f = amf_array_first(info->filepositions);
    while (node_t != NULL || node_f != NULL) {
        amf_data * amf_filepos = amf_array_get(node_f);
        number64 offset = amf_number_get_value(amf_filepos) + new_on_metadata_size - info->on_metadata_size;

        number64 timestamp = amf_number_get_value(amf_array_get(node_t));

        /* after the onLastSecond event we need to take in account the tag size */
        if (!info->have_on_last_second && (duration - timestamp) <= 1.0) {
            offset += (sizeof(flv_tag) + on_last_second_size + sizeof(uint32_be));
        }

        amf_number_set_value(amf_filepos, offset);
        node_t = amf_array_next(node_t);
        node_f = amf_array_next(node_f);
    }

    uint32 total_data_size = info->video_data_size + info->audio_data_size + info->meta_data_size + new_on_metadata_size;
    if (!info->have_on_last_second) {
        total_data_size += (uint32)on_last_second_size;
    }
    amf_number_set_value(amf_total_data_size, (number64)total_data_size);

    uint32 total_filesize = sizeof(flv_header) + total_data_size + info->total_prev_tags_size;
    if (!info->have_on_last_second) {
        /* if we have to add onLastSecond, we must count the header and new prevTagSize we add */
        total_filesize += (sizeof(flv_tag) + sizeof(uint32_be));
    }
    amf_number_set_value(amf_total_filesize, (number64)total_filesize);
}

/*
    Write the flv output file
*/
int write_flv(FILE * flv_in, FILE * flv_out, const flv_info * info, const flv_metadata * meta) {
    /* write the flv header */
    if (fwrite(&info->header, sizeof(flv_header), 1, flv_out) != 1) {
        return ERROR_WRITE;
    }

    /* first "previous tag size" */
    uint32_be size = swap_uint32(0);
    if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
        return ERROR_WRITE;
    }

    /* write the onMetaData tag */
    uint32 on_metadata_name_size = (uint32)amf_data_size(meta->on_metadata_name);
    uint32 on_metadata_size = (uint32)amf_data_size(meta->on_metadata);

    flv_tag ft;
    ft.type = FLV_TAG_TYPE_META;
    ft.body_length = uint32_to_uint24_be(on_metadata_name_size + on_metadata_size);
    ft.timestamp = uint32_to_uint24_be(0);
    ft.timestamp_extended = 0;
    ft.stream_id = uint32_to_uint24_be(0);
    if (fwrite(&ft, sizeof(flv_tag), 1, flv_out) != 1 ||
        amf_data_write(meta->on_metadata_name, flv_out) < on_metadata_name_size ||
        amf_data_write(meta->on_metadata, flv_out) < on_metadata_size
    ) {
        return ERROR_WRITE;
    }

    /* previous tag size */
    size = swap_uint32(sizeof(flv_tag) + on_metadata_name_size + on_metadata_size);
    if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
        return ERROR_WRITE;
    }

    /* copy the rest of the tags verbatim */
    uint32 duration;
    if (info->have_audio) {
        duration = info->last_timestamp + info->audio_frame_duration;
    }
    else {
        duration = info->last_timestamp + info->video_frame_duration;
    }

    fseek(flv_in, info->first_data_tag_offset, SEEK_SET);

    byte * copy_buffer = (byte *)malloc(info->biggest_tag_body_size);
    int have_on_last_second = 0;
    while (!feof(flv_in)) {
        uint32 body_length;
        uint32 timestamp;

        if (fread(&ft, sizeof(ft), 1, flv_in) == 0) {
            break;
        }

        body_length = uint24_be_to_uint32(ft.body_length);
        timestamp = uint24_be_to_uint32(ft.timestamp);

        /* insert an onLastSecond metadata tag */
        if (!have_on_last_second && !info->have_on_last_second && (duration - timestamp) <= 1000) {
            uint32 on_last_second_name_size = (uint32)amf_data_size(meta->on_last_second_name);
            uint32 on_last_second_size = (uint32)amf_data_size(meta->on_last_second);
            flv_tag tag;
            tag.type = FLV_TAG_TYPE_META;
            tag.body_length = uint32_to_uint24_be(on_last_second_name_size + on_last_second_size);
            tag.timestamp = ft.timestamp;
            tag.timestamp_extended = 0;
            tag.stream_id = uint32_to_uint24_be(0);
            if (fwrite(&tag, sizeof(flv_tag), 1, flv_out) != 1 ||
                amf_data_write(meta->on_last_second_name, flv_out) < on_last_second_name_size ||
                amf_data_write(meta->on_last_second, flv_out) < on_last_second_size
            ) {
                free(copy_buffer);
                return ERROR_WRITE;
            }

            /* previous tag size */
            size = swap_uint32(sizeof(flv_tag) + on_last_second_name_size + on_last_second_size);
            if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
                free(copy_buffer);
                return ERROR_WRITE;
            }

            have_on_last_second = 1;
        }

        /* copy the tag verbatim */
        if (fread(copy_buffer, 1, body_length, flv_in) < body_length) {
            free(copy_buffer);
            return ERROR_EOF;
        }
        if (fwrite(&ft, sizeof(flv_tag), 1, flv_out) != 1 ||
            fwrite(copy_buffer, 1, body_length, flv_out) < body_length) {
            free(copy_buffer);
            return ERROR_WRITE;
        }

        /* previous tag length */
        size = swap_uint32(sizeof(flv_tag) + body_length);
        if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
            free(copy_buffer);
            return ERROR_WRITE;
        }

        fseek(flv_in, sizeof(uint32_be), SEEK_CUR);
    }

    free(copy_buffer);
    return OK;
}

/* copy a FLV file while adding onMetaData and onLastSecond events */
int inject_metadata(const char * in_file, const char * out_file) {
    int res;

    FILE * flv_in = fopen(in_file, "rb");
    if (flv_in == NULL) {
        return ERROR_OPEN_READ;
    }

    /*
        open output file
    */
    FILE * flv_out = fopen(out_file, "wb");
    if (flv_out == NULL) {
        fclose(flv_in);
        return ERROR_OPEN_WRITE;
    }

    /*
        get all necessary information from the flv file
    */
    flv_info info;
    res = get_flv_info(flv_in, &info);
    if (res != OK) {
        fclose(flv_in);
        fclose(flv_out);
        amf_data_free(info.keyframes);
        return res;
    }

    flv_metadata meta;
    compute_metadata(&info, &meta);

    /* debug */
    /* amf_data_dump(stderr, meta.on_metadata, 0); */

    /*
        write the output file
    */
    res = write_flv(flv_in, flv_out, &info, &meta);

    fclose(flv_in);
    fclose(flv_out);
    amf_data_free(meta.on_last_second_name);
    amf_data_free(meta.on_last_second);
    amf_data_free(meta.on_metadata_name);
    amf_data_free(meta.on_metadata);

    return res;
}

int main(int argc, char ** argv) {
    if (argc < 3) {
        fprintf(stderr, "%s\n", PACKAGE_STRING);
        fprintf(stderr, "%s\n", COPYRIGHT_STR);
        fprintf(stderr, "This is free software; see the source for copying conditions. There is NO\n"
                        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n\n");
        fprintf(stderr, "Usage: flvmeta in_file out_file\n\n");
        fprintf(stderr, "Report bugs to <%s>\n\n", PACKAGE_BUGREPORT);
        return 1;
    }
    
    if (!strcmp(argv[1], argv[2])) {
    	fprintf(stderr, "Error: input file and output file must be different.\n\n");
    }

    int errcode = inject_metadata(argv[1], argv[2]);
    switch (errcode) {
        case ERROR_OPEN_READ: fprintf(stderr, "Error: cannot open %s for reading.\n", argv[1]); break;
        case ERROR_OPEN_WRITE: fprintf(stderr, "Error: cannot open %s for writing.\n", argv[2]); break;
        case ERROR_NO_FLV: fprintf(stderr, "Error: %s is not a valid FLV file.\n", argv[1]); break;
        case ERROR_EOF: fprintf(stderr, "Error: unexpected end of file.\n"); break;
        case ERROR_INVALID_TAG: fprintf(stderr, "Error: invalid FLV tag.\n"); break;
        case ERROR_WRITE: fprintf(stderr, "Error: unable to write to %s.\n", argv[2]); break;
    }
    return errcode;
}
