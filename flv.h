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
#ifndef __FLV_H__
#define __FLV_H__

#include "types.h"

/* flv file format structure and definitions */
#pragma pack(push, 1)

/* FLV file header */
#define FLV_SIGNATURE       "FLV"
#define FLV_VERSION         ((uint8)0x01)

#define FLV_FLAG_VIDEO      ((uint8)0x01)
#define FLV_FLAG_AUDIO      ((uint8)0x04)

typedef struct __flv_header {
    byte            signature[3]; /* always "FLV" */
    uint8           version; /* should be 1 */
    uint8_bitmask   flags;
    uint32_be       offset; /* always 9 */
} flv_header;

#define flv_header_has_video(header)    ((header).flags & FLV_FLAG_VIDEO)
#define flv_header_has_audio(header)    ((header).flags & FLV_FLAG_AUDIO)

/* FLV tag */
#define FLV_TAG_TYPE_AUDIO  ((uint8)0x08)
#define FLV_TAG_TYPE_VIDEO  ((uint8)0x09)
#define FLV_TAG_TYPE_META   ((uint8)0x12)

typedef struct __flv_tag {
    uint8       type;
    uint24_be   body_length; /* in bytes, total tag size minus 11 */
    uint24_be   timestamp; /* milli-seconds */
    uint8       timestamp_extended; /* timestamp extension */
    uint24_be   stream_id; /* reserved, must be "\0\0\0" */
    /* body comes next */
} flv_tag;

#define flv_tag_get_timestamp(tag) \
    (uint24_be_to_uint32((tag).timestamp) + ((tag).timestamp_extended << 24))

/* audio tag */
#define FLV_AUDIO_TAG_SOUND_TYPE_MONO    0
#define FLV_AUDIO_TAG_SOUND_TYPE_STEREO  1

#define FLV_AUDIO_TAG_SOUND_SIZE_8       0
#define FLV_AUDIO_TAG_SOUND_SIZE_16      1

#define FLV_AUDIO_TAG_SOUND_RATE_5_5     0
#define FLV_AUDIO_TAG_SOUND_RATE_11      1
#define FLV_AUDIO_TAG_SOUND_RATE_22      2
#define FLV_AUDIO_TAG_SOUND_RATE_44      3

#define FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM          0
#define FLV_AUDIO_TAG_SOUND_FORMAT_ADPCM               1
#define FLV_AUDIO_TAG_SOUND_FORMAT_MP3                 2
#define FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM_LE       3
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO  4
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO   5
#define FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER          6
#define FLV_AUDIO_TAG_SOUND_FORMAT_G711_A              7
#define FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU             8
#define FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED            9
#define FLV_AUDIO_TAG_SOUND_FORMAT_AAC                 10
#define FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8               14
#define FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC     15

typedef byte flv_audio_tag;

#define flv_audio_tag_sound_type(tag)   (((tag) & 0x01) >> 0)
#define flv_audio_tag_sound_size(tag)   (((tag) & 0x02) >> 1)
#define flv_audio_tag_sound_rate(tag)   (((tag) & 0x0C) >> 2)
#define flv_audio_tag_sound_format(tag) (((tag) & 0xF0) >> 4)

/* video tag */
#define FLV_VIDEO_TAG_CODEC_JPEG            1
#define FLV_VIDEO_TAG_CODEC_SORENSEN_H263   2
#define FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO    3
#define FLV_VIDEO_TAG_CODEC_ON2_VP6         4
#define FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA   5
#define FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2 6
#define FLV_VIDEO_TAG_CODEC_AVC             7

#define FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME               1
#define FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME             2
#define FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME  3

typedef byte flv_video_tag;

#define flv_video_tag_codec_id(tag)     (((tag) & 0x0F) >> 0)
#define flv_video_tag_frame_type(tag)   (((tag) & 0xF0) >> 4)

#pragma pack(pop)

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* FLV functions */
void flv_tag_set_timestamp(flv_tag * tag, uint32 timestamp);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FLV_H__ */
