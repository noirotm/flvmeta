/*
    $Id: check.h 231 2011-06-27 13:46:19Z marc.noirot $

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
#ifndef __CHECK_H__
#define __CHECK_H__

#include "flvmeta.h"

/* message level codes */
#define LEVEL_FATAL     "F"
#define LEVEL_ERROR     "E"
#define LEVEL_WARNING   "W"
#define LEVEL_INFO      "I"

/* message topic codes */
#define TOPIC_GENERAL_FORMAT    "10"
#define TOPIC_HEADER            "11"
#define TOPIC_PREV_TAG_SIZE     "12"
#define TOPIC_TAG_FORMAT        "20"
#define TOPIC_TAG_TYPES         "30"
#define TOPIC_TIMESTAMPS        "40"
#define TOPIC_AUDIO_DATA        "50"
#define TOPIC_AUDIO_CODECS      "51"
#define TOPIC_VIDEO_DATA        "60"
#define TOPIC_VIDEO_CODECS      "61"
#define TOPIC_METADATA          "70"
#define TOPIC_AMF_DATA          "80"
#define TOPIC_KEYFRAMES         "81"
#define TOPIC_CUE_POINTS        "82"

/* info, warning, error codes, sorted by unique identifier */
#define FATAL_HEADER_EOF                    LEVEL_FATAL     TOPIC_HEADER            "001"
#define FATAL_HEADER_NO_SIGNATURE           LEVEL_FATAL     TOPIC_HEADER            "002"
#define ERROR_HEADER_BAD_VERSION            LEVEL_ERROR     TOPIC_HEADER            "003"
#define ERROR_HEADER_NO_STREAMS             LEVEL_ERROR     TOPIC_HEADER            "004"
#define INFO_HEADER_NO_AUDIO                LEVEL_INFO      TOPIC_HEADER            "005"
#define WARNING_HEADER_NO_VIDEO             LEVEL_WARNING   TOPIC_HEADER            "006"
#define ERROR_HEADER_BAD_RESERVED_FLAGS     LEVEL_ERROR     TOPIC_HEADER            "007"
#define ERROR_HEADER_BAD_OFFSET             LEVEL_ERROR     TOPIC_HEADER            "008"
#define FATAL_PREV_TAG_SIZE_EOF             LEVEL_FATAL     TOPIC_PREV_TAG_SIZE     "009"
#define ERROR_PREV_TAG_SIZE_BAD_FIRST       LEVEL_ERROR     TOPIC_PREV_TAG_SIZE     "010"
#define FATAL_GENERAL_NO_TAG                LEVEL_FATAL     TOPIC_GENERAL_FORMAT    "011"
#define FATAL_TAG_EOF                       LEVEL_FATAL     TOPIC_TAG_FORMAT        "012"
#define ERROR_TAG_TYPE_UNKNOWN              LEVEL_ERROR     TOPIC_TAG_TYPES         "013"
#define WARNING_HEADER_UNEXPECTED_VIDEO     LEVEL_WARNING   TOPIC_HEADER            "014"
#define WARNING_HEADER_UNEXPECTED_AUDIO     LEVEL_WARNING   TOPIC_HEADER            "015"
#define FATAL_TAG_BODY_LENGTH_OVERFLOW      LEVEL_FATAL     TOPIC_TAG_FORMAT        "016"
#define WARNING_TAG_BODY_LENGTH_LARGE       LEVEL_WARNING   TOPIC_TAG_FORMAT        "017"
#define WARNING_TAG_BODY_LENGTH_ZERO        LEVEL_WARNING   TOPIC_TAG_FORMAT        "018"
#define ERROR_TIMESTAMP_FIRST_NON_ZERO      LEVEL_ERROR     TOPIC_TIMESTAMPS        "019"
#define ERROR_TIMESTAMP_AUDIO_DECREASE      LEVEL_ERROR     TOPIC_TIMESTAMPS        "020"
#define ERROR_TIMESTAMP_VIDEO_DECREASE      LEVEL_ERROR     TOPIC_TIMESTAMPS        "021"
#define ERROR_TIMESTAMP_OVERFLOW            LEVEL_ERROR     TOPIC_TIMESTAMPS        "022"
#define ERROR_TIMESTAMP_DECREASE            LEVEL_ERROR     TOPIC_TIMESTAMPS        "023"
#define WARNING_TIMESTAMP_DESYNC            LEVEL_WARNING   TOPIC_TIMESTAMPS        "024"
#define ERROR_TAG_STREAM_ID_NON_ZERO        LEVEL_ERROR     TOPIC_TAG_FORMAT        "025"
#define WARNING_AUDIO_FORMAT_CHANGED        LEVEL_WARNING   TOPIC_AUDIO_DATA        "026"
#define WARNING_AUDIO_CODEC_UNKNOWN         LEVEL_WARNING   TOPIC_AUDIO_CODECS      "027"
#define WARNING_AUDIO_CODEC_RESERVED        LEVEL_WARNING   TOPIC_AUDIO_CODECS      "028"
#define WARNING_AUDIO_CODEC_AAC_BAD         LEVEL_WARNING   TOPIC_AUDIO_CODECS      "029"
#define WARNING_AUDIO_CODEC_NELLYMOSER_BAD  LEVEL_WARNING   TOPIC_AUDIO_CODECS      "030"


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* check FLV file validity */
int check_flv_file(const flvmeta_opts * opts);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CHECK_H__ */
