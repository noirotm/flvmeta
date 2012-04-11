/*
    $Id: check.c 236 2011-08-10 15:30:04Z marc.noirot@gmail.com $

    FLV Metadata updater

    Copyright (C) 2007-2012 Marc Noirot <marc.noirot AT gmail.com>

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
#include "check.h"
#include "dump.h"
#include "info.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_ACCEPTABLE_TAG_BODY_LENGTH 1000000

/* start the report */
static void report_start(const flvmeta_opts * opts) {
    if (opts->quiet)
        return;

    if (opts->check_xml_report) {
        time_t now;
        struct tm * t;
        char datestr[128];

        puts("<?xml version=\"1.0\" encoding=\"utf-8\"?>");
        puts("<report xmlns=\"http://schemas.flvmeta.org/report/1.0/\">");
        puts("  <metadata>");
        printf("    <filename>%s</filename>\n", opts->input_file);

        now = time(NULL);
        tzset();
        t = localtime(&now);
        strftime(datestr, sizeof(datestr), "%Y-%m-%dT%H:%M:%S", t);
        printf("    <creation-date>%s</creation-date>\n", datestr);

        printf("    <generator>%s</generator>\n", PACKAGE_STRING);

        puts("  </metadata>");
        puts("  <messages>");
    }
}

/* end the report */
static void report_end(const flvmeta_opts * opts, int errors, int warnings) {
    if (opts->quiet)
        return;

    if (opts->check_xml_report) {
        puts("  </messages>");
        puts("</report>");
    }
    else {
        printf("%d error(s), %d warning(s)\n", errors, warnings);
    }
}

/* report an error to stdout according to the current format */
static void report_print_message(
    int level,
    const char * code,
    file_offset_t offset,
    const char * message,
    const flvmeta_opts * opts
) {
    if (opts->quiet)
        return;

    if (level >= opts->check_level) {
        char * levelstr;

        switch (level) {
            case FLVMETA_CHECK_LEVEL_INFO: levelstr = "info"; break;
            case FLVMETA_CHECK_LEVEL_WARNING: levelstr = "warning"; break;
            case FLVMETA_CHECK_LEVEL_ERROR: levelstr = "error"; break;
            case FLVMETA_CHECK_LEVEL_FATAL: levelstr = "fatal"; break;
            default: levelstr = "unknown";
        }

        if (opts->check_xml_report) {
            /* XML report entry */
            printf("    <message level=\"%s\" code=\"%s\"", levelstr, code);
            printf(" offset=\"%" FILE_OFFSET_PRINTF_FORMAT  "i\">", offset);
            printf("%s</message>\n", message);
        }
        else {
            /* raw report entry */
            /*printf("%s:", opts->input_file);*/
            printf("0x%.8" FILE_OFFSET_PRINTF_FORMAT "x: ", offset);
            printf("%s %s: %s\n", levelstr, code, message);
        }
    }
}

/* convenience macros */
#define print_info(code, offset, message) \
    report_print_message(FLVMETA_CHECK_LEVEL_INFO, code, offset, message, opts)
#define print_warning(code, offset, message) \
    if (opts->check_level <= FLVMETA_CHECK_LEVEL_WARNING) ++warnings; \
    report_print_message(FLVMETA_CHECK_LEVEL_WARNING, code, offset, message, opts)
#define print_error(code, offset, message) \
    if (opts->check_level <= FLVMETA_CHECK_LEVEL_ERROR) ++errors; \
    report_print_message(FLVMETA_CHECK_LEVEL_ERROR, code, offset, message, opts)
#define print_fatal(code, offset, message) \
    if (opts->check_level <= FLVMETA_CHECK_LEVEL_FATAL) ++errors; \
    report_print_message(FLVMETA_CHECK_LEVEL_FATAL, code, offset, message, opts)

/* get string representing given AMF type */
const char * get_amf_type_string(byte type) {
    switch (type) {
        case AMF_TYPE_NUMBER: return "Number";
        case AMF_TYPE_BOOLEAN: return "Boolean";
        case AMF_TYPE_STRING: return "String";
        case AMF_TYPE_NULL: return "Null";
        case AMF_TYPE_UNDEFINED: return "Undefined";
        /*case AMF_TYPE_REFERENCE:*/
        case AMF_TYPE_OBJECT: return "Object";
        case AMF_TYPE_ASSOCIATIVE_ARRAY: return "Associative array";
        case AMF_TYPE_ARRAY: return "Array";
        case AMF_TYPE_DATE: return "Date";
        /*case AMF_TYPE_SIMPLEOBJECT:*/
        case AMF_TYPE_XML: return "XML";
        case AMF_TYPE_CLASS: return "Class";
        default: return "Unknown type";
    }
}

/* check FLV file validity */
int check_flv_file(const flvmeta_opts * opts) {
    flv_stream * flv_in;
    flv_header header;
    int errors, warnings;
    int result;
    char message[256];
    uint32 prev_tag_size, tag_number;
    uint32 last_timestamp, last_video_timestamp, last_audio_timestamp;
    struct stat file_stats;
    int have_audio, have_video;
    flvmeta_opts opts_loc;
    flv_info info;
    int have_desync;
    int have_on_metadata;
    file_offset_t on_metadata_offset;
    amf_data * on_metadata, * on_metadata_name;
    int have_on_last_second;
    uint32 on_last_second_timestamp;

    int have_prev_audio_tag;
    flv_audio_tag prev_audio_tag;
    int have_prev_video_tag;
    flv_video_tag prev_video_tag;

    int video_frames_number, keyframes_number;

    prev_audio_tag = 0;
    prev_video_tag = 0;
    
    have_audio = have_video = 0;
    tag_number = 0;
    last_timestamp = last_video_timestamp = last_audio_timestamp = 0;
    have_desync = 0;
    have_prev_audio_tag = have_prev_video_tag = 0;
    video_frames_number = keyframes_number = 0;
    have_on_metadata = 0;
    on_metadata_offset = 0;
    on_metadata = on_metadata_name = NULL;
    have_on_last_second = 0;
    on_last_second_timestamp = 0;

    /* file stats */
    if (stat(opts->input_file, &file_stats) != 0) {
        return ERROR_OPEN_READ;
    }

    /* open file for reading */
    flv_in = flv_open(opts->input_file);
    if (flv_in == NULL) {
        return ERROR_OPEN_READ;
    }
    
    errors = warnings = 0;

    report_start(opts);

    /** check header **/

    /* check signature */
    result = flv_read_header(flv_in, &header);
    if (result == FLV_ERROR_EOF) {
        print_fatal(FATAL_HEADER_EOF, 0, "unexpected end of file in header");
        goto end;
    }
    else if (result == FLV_ERROR_NO_FLV) {
        print_fatal(FATAL_HEADER_NO_SIGNATURE, 0, "FLV signature not found in header");
        goto end;
    }

    /* version */
    if (header.version != FLV_VERSION) {
        sprintf(message, "header version should be 1, %d found instead", header.version);
        print_error(ERROR_HEADER_BAD_VERSION, 3, message);
    }

    /* video and audio flags */
    if (!flv_header_has_audio(header) && !flv_header_has_video(header)) {
        print_error(ERROR_HEADER_NO_STREAMS, 4, "header signals the file does not contain video tags or audio tags");
    }
    else if (!flv_header_has_audio(header)) {
        print_info(INFO_HEADER_NO_AUDIO, 4, "header signals the file does not contain audio tags");
    }
    else if (!flv_header_has_video(header)) {
        print_warning(WARNING_HEADER_NO_VIDEO, 4, "header signals the file does not contain video tags");
    }

    /* reserved flags */
    if (header.flags & 0xFA) {
        print_error(ERROR_HEADER_BAD_RESERVED_FLAGS, 4, "header reserved flags are not zero");
    }

    /* offset */
    if (flv_header_get_offset(header) != 9) {
        sprintf(message, "header offset should be 9, %d found instead", flv_header_get_offset(header));
        print_error(ERROR_HEADER_BAD_OFFSET, 5, message);
    }

    /** check first previous tag size **/

    result = flv_read_prev_tag_size(flv_in, &prev_tag_size);
    if (result == FLV_ERROR_EOF) {
        print_fatal(FATAL_PREV_TAG_SIZE_EOF, 9, "unexpected end of file in previous tag size");
        goto end;
    }
    else if (prev_tag_size != 0) {
        sprintf(message, "first previous tag size should be 0, %d found instead", prev_tag_size);
        print_error(ERROR_PREV_TAG_SIZE_BAD_FIRST, 9, message);
    }

    /* we reached the end of file: no tags in file */
    if (flv_get_offset(flv_in) == file_stats.st_size) {
        print_fatal(FATAL_GENERAL_NO_TAG, 13, "file does not contain tags");
        goto end;
    }

    /** read tags **/
    while (flv_get_offset(flv_in) < file_stats.st_size) {
        flv_tag tag;
        file_offset_t offset;
        uint32 body_length, timestamp, stream_id;
        int decr_timestamp_signaled;

        result = flv_read_tag(flv_in, &tag);
        if (result != FLV_OK) {
            print_fatal(FATAL_TAG_EOF, flv_get_offset(flv_in), "unexpected end of file in tag");
            goto end;
        }

        ++tag_number;

        offset = flv_get_current_tag_offset(flv_in);
        body_length = flv_tag_get_body_length(tag);
        timestamp = flv_tag_get_timestamp(tag);
        stream_id = flv_tag_get_stream_id(tag);

        /* check tag type */
        if (tag.type != FLV_TAG_TYPE_AUDIO
            && tag.type != FLV_TAG_TYPE_VIDEO
            && tag.type != FLV_TAG_TYPE_META
        ) {
            sprintf(message, "unknown tag type %hhd", tag.type);
            print_error(ERROR_TAG_TYPE_UNKNOWN, offset, message);
        }

        /* check consistency with global header */
        if (!have_video && tag.type == FLV_TAG_TYPE_VIDEO) {
            if (!flv_header_has_video(header)) {
                print_warning(WARNING_HEADER_UNEXPECTED_VIDEO, offset, "video tag found despite header signaling the file contains no video");
            }
            have_video = 1;
        }
        if (!have_audio && tag.type == FLV_TAG_TYPE_AUDIO) {
            if (!flv_header_has_audio(header)) {
                print_warning(WARNING_HEADER_UNEXPECTED_AUDIO, offset, "audio tag found despite header signaling the file contains no audio");
            }
            have_audio = 1;
        }

        /* check body length */
        if (body_length > (file_stats.st_size - flv_get_offset(flv_in))) {
            sprintf(message, "tag body length (%d bytes) exceeds file size", body_length);
            print_fatal(FATAL_TAG_BODY_LENGTH_OVERFLOW, offset + 1, message);
            goto end;
        }
        else if (body_length > MAX_ACCEPTABLE_TAG_BODY_LENGTH) {
            sprintf(message, "tag body length (%d bytes) is abnormally large", body_length);
            print_warning(WARNING_TAG_BODY_LENGTH_LARGE, offset + 1, message);
        }
        else if (body_length == 0) {
            print_warning(WARNING_TAG_BODY_LENGTH_ZERO, offset + 1, "tag body length is zero");
        }

        /** check timestamp **/
        decr_timestamp_signaled = 0;

        /* check whether first timestamp is zero */
        if (tag_number == 1 && timestamp != 0) {
            sprintf(message, "first timestamp should be zero, %d found instead", timestamp);
            print_error(ERROR_TIMESTAMP_FIRST_NON_ZERO, offset + 4, message);
        }

        /* check whether timestamps decrease in a given stream */
        if (tag.type == FLV_TAG_TYPE_AUDIO) {
            if (last_audio_timestamp > timestamp) {
                sprintf(message, "audio tag timestamps are decreasing from %d to %d", last_audio_timestamp, timestamp);
                print_error(ERROR_TIMESTAMP_AUDIO_DECREASE, offset + 4, message);
            }
            last_audio_timestamp = timestamp;
            decr_timestamp_signaled = 1;
        }
        if (tag.type == FLV_TAG_TYPE_VIDEO) {
            if (last_video_timestamp > timestamp) {
                sprintf(message, "video tag timestamps are decreasing from %d to %d", last_video_timestamp, timestamp);
                print_error(ERROR_TIMESTAMP_VIDEO_DECREASE, offset + 4, message);
            }
            last_video_timestamp = timestamp;
            decr_timestamp_signaled = 1;
        }

        /* check for overflow error */
        if (last_timestamp > timestamp && last_timestamp - timestamp > 0xF00000) {
            print_error(ERROR_TIMESTAMP_OVERFLOW, offset + 4, "extended bits not used after timestamp overflow");
        }

        /* check whether timestamps decrease globally */
        else if (!decr_timestamp_signaled && last_timestamp > timestamp && last_timestamp - timestamp >= 1000) {
            sprintf(message, "timestamps are decreasing from %d to %d", last_timestamp, timestamp);
            print_error(ERROR_TIMESTAMP_DECREASE, offset + 4, message);
        }

        last_timestamp = timestamp;

        /* check for desyncs between audio and video: one second or more is suspicious */
        if (have_video && have_audio && !have_desync && labs(last_video_timestamp - last_audio_timestamp) >= 1000) {
            sprintf(message, "audio and video streams are desynchronized by %ld ms",
                labs(last_video_timestamp - last_audio_timestamp));
            print_warning(WARNING_TIMESTAMP_DESYNC, offset + 4, message);
            have_desync = 1; /* do not repeat */
        }

        /** stream id must be zero **/
        if (stream_id != 0) {
            sprintf(message, "tag stream id must be zero, %d found instead", stream_id);
            print_error(ERROR_TAG_STREAM_ID_NON_ZERO, offset + 8, message);
        }

        /* check tag body contents only if not empty */
        if (body_length > 0) {

            /** check audio info **/
            if (tag.type == FLV_TAG_TYPE_AUDIO) {
                flv_audio_tag at;
                uint8_bitmask audio_format;

                result = flv_read_audio_tag(flv_in, &at);
                if (result == FLV_ERROR_EOF) {
                    print_fatal(FATAL_TAG_EOF, offset + 11, "unexpected end of file in tag");
                    goto end;
                }

                /* check whether the format varies between tags */
                if (have_prev_audio_tag && prev_audio_tag != at) {
                    print_warning(WARNING_AUDIO_FORMAT_CHANGED, offset + 11, "audio format changed since last tag");
                }

                /* check format */
                audio_format = flv_audio_tag_sound_format(at);
                if (audio_format == 12 || audio_format == 13) {
                    sprintf(message, "unknown audio format %d", audio_format);
                    print_warning(WARNING_AUDIO_CODEC_UNKNOWN, offset + 11, message);
                }
                else if (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_A
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC
                ) {
                    sprintf(message, "audio format %d is reserved for internal use", audio_format);
                    print_warning(WARNING_AUDIO_CODEC_RESERVED, offset + 11, message);
                }

                /* check consistency, see flash video spec */
                if (flv_audio_tag_sound_rate(at) != FLV_AUDIO_TAG_SOUND_RATE_44
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning(WARNING_AUDIO_CODEC_AAC_BAD, offset + 11, "audio data in AAC format should have a 44KHz rate, field will be ignored");
                }

                if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_STEREO
                    && (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO)
                ) {
                    print_warning(WARNING_AUDIO_CODEC_NELLYMOSER_BAD, offset + 11, "audio data in Nellymoser format cannot be stereo, field will be ignored");
                }

                else if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_MONO
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning(WARNING_AUDIO_CODEC_AAC_MONO, offset + 11, "audio data in AAC format should be stereo, field will be ignored");
                }

                else if (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM) {
                    print_warning(WARNING_AUDIO_CODEC_LINEAR_PCM, offset + 11, "audio data in Linear PCM, platform endian format should not be used because of non-portability");
                }

                prev_audio_tag = at;
                have_prev_audio_tag = 1;
            }
            /** check video info **/
            else if (tag.type == FLV_TAG_TYPE_VIDEO) {
                flv_video_tag vt;
                uint8_bitmask video_frame_type, video_codec;

                video_frames_number++;

                result = flv_read_video_tag(flv_in, &vt);
                if (result == FLV_ERROR_EOF) {
                    print_fatal(FATAL_TAG_EOF, offset + 11, "unexpected end of file in tag");
                    goto end;
                }

                /* check whether the format varies between tags */
                if (have_prev_video_tag && flv_video_tag_codec_id(prev_video_tag) != flv_video_tag_codec_id(vt)) {
                    print_warning(WARNING_VIDEO_FORMAT_CHANGED, offset + 11, "video format changed since last tag");
                }

                /* check video frame type */
                video_frame_type = flv_video_tag_frame_type(vt);
                if (video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME
                    && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_INTERFRAME
                    && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_DISPOSABLE_INTERFRAME
                    && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_GENERATED_KEYFRAME
                    && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_COMMAND_FRAME
                ) {
                    sprintf(message, "unknown video frame type %d", video_frame_type);
                    print_error(ERROR_VIDEO_FRAME_TYPE_UNKNOWN, offset + 11, message);
                }

                if (video_frame_type == FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME) {
                    keyframes_number++;
                }

                /* check whether first frame is a keyframe */
                if (!have_prev_video_tag && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME) {
                    print_warning(WARNING_VIDEO_NO_FIRST_KEYFRAME, offset + 11, "first video frame is not a keyframe, playback will suffer");
                }

                /* check video codec */
                video_codec = flv_video_tag_codec_id(vt);
                if (video_codec != FLV_VIDEO_TAG_CODEC_JPEG
                    && video_codec != FLV_VIDEO_TAG_CODEC_SORENSEN_H263
                    && video_codec != FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO
                    && video_codec != FLV_VIDEO_TAG_CODEC_ON2_VP6
                    && video_codec != FLV_VIDEO_TAG_CODEC_ON2_VP6_ALPHA
                    && video_codec != FLV_VIDEO_TAG_CODEC_SCREEN_VIDEO_V2
                    && video_codec != FLV_VIDEO_TAG_CODEC_AVC
                ) {
                    sprintf(message, "unknown video codec id %d", video_codec);
                    print_error(ERROR_VIDEO_CODEC_UNKNOWN, offset + 11, message);
                }

                /* according to spec, JPEG codec is not currently used */
                if (video_codec == FLV_VIDEO_TAG_CODEC_JPEG) {
                    print_warning(WARNING_VIDEO_CODEC_JPEG, offset + 11, "JPEG codec not currently used");
                }

                prev_video_tag = vt;
                have_prev_video_tag = 1;
            }
            /** check script data info **/
            else if (tag.type == FLV_TAG_TYPE_META) {
                amf_data * name;
                amf_data * data;

                name = NULL;
                data = NULL;
                result = flv_read_metadata(flv_in, &name, &data);

                if (result == FLV_ERROR_EOF) {
                    print_fatal(FATAL_TAG_EOF, offset + 11, "unexpected end of file in tag");
                    amf_data_free(name);
                    amf_data_free(data);
                    goto end;
                }
                else if (result == FLV_ERROR_EMPTY_TAG) {
                    print_warning(WARNING_METADATA_EMPTY, offset + 11, "empty metadata tag");
                }
                else if (result == FLV_ERROR_INVALID_METADATA_NAME) {
                    print_error(ERROR_METADATA_NAME_INVALID, offset + 11, "invalid metadata name");
                }
                else if (result == FLV_ERROR_INVALID_METADATA) {
                    print_error(ERROR_METADATA_DATA_INVALID, offset + 11, "invalid metadata");
                }
                else if (amf_data_get_type(name) != AMF_TYPE_STRING) {
                    /* name type checking */
                    sprintf(message, "invalid metadata name type: %d, should be a string (2)", amf_data_get_type(name));
                    print_error(ERROR_METADATA_NAME_INVALID_TYPE, offset, message);
                }
                else {
                    /* empty name checking */
                    if (amf_string_get_size(name) == 0) {
                        print_warning(WARNING_METADATA_NAME_EMPTY, offset, "empty metadata name");
                    }

                    /* check whether all body size has been read */
                    if (flv_in->current_tag_body_length > 0) {
                        sprintf(message, "%d bytes not read in tag body after metadata end", body_length - flv_in->current_tag_body_length);
                        print_warning(WARNING_METADATA_DATA_REMAINING, flv_get_offset(flv_in), message);
                    }
                    else if (flv_in->current_tag_body_overflow > 0) {
                        sprintf(message, "%d bytes missing from tag body after metadata end", flv_in->current_tag_body_overflow);
                        print_warning(WARNING_METADATA_DATA_MISSING, flv_get_offset(flv_in), message);
                    }

                    /* onLastSecond checking */
                    if (!strcmp((char*)amf_string_get_bytes(name), "onLastSecond")) {
                        if (have_on_last_second == 0) {
                            have_on_last_second = 1;
                            on_last_second_timestamp = timestamp;
                        }
                        else {
                            print_warning(WARNING_METADATA_LAST_SECOND_DUP, offset, "duplicate onLastSecond event");
                        }
                    }

                    /* onMetaData checking */
                    if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
                        if (have_on_metadata == 0) {
                            have_on_metadata = 1;
                            on_metadata_offset = offset;
                            on_metadata = amf_data_clone(data);
                            on_metadata_name = amf_data_clone(name);

                            /* check onMetadata type */
                            if (amf_data_get_type(on_metadata) != AMF_TYPE_ASSOCIATIVE_ARRAY) {
                                sprintf(message, "invalid onMetaData data type: %d, should be an associative array (8)", amf_data_get_type(on_metadata));
                                print_error(ERROR_METADATA_DATA_INVALID_TYPE, offset, message);
                            }

                            /* onMetaData must be the first tag at 0 timestamp */
                            if (tag_number != 1) {
                                print_warning(WARNING_METADATA_BAD_TAG, offset, "onMetadata event found after the first tag");
                            }
                            if (timestamp != 0) {
                                print_warning(WARNING_METADATA_BAD_TIMESTAMP, offset, "onMetadata event found after timestamp zero");
                            }
                        }
                        else {
                            print_warning(WARNING_METADATA_DUPLICATE, offset, "duplicate onMetaData event");
                        }
                    }

                    /* unknown metadata name */
                    if (strcmp((char*)amf_string_get_bytes(name), "onMetaData")
                    && strcmp((char*)amf_string_get_bytes(name), "onCuePoint")
                    && strcmp((char*)amf_string_get_bytes(name), "onLastSecond")) {
                        sprintf(message, "unknown metadata event name: '%s'", (char*)amf_string_get_bytes(name));
                        print_info(INFO_METADATA_NAME_UNKNOWN, flv_get_offset(flv_in), message);
                    }
                }

                amf_data_free(name);
                amf_data_free(data);
            }
        }

        /* check body length against previous tag size */
        result = flv_read_prev_tag_size(flv_in, &prev_tag_size);
        if (result != FLV_OK) {
            print_fatal(FATAL_PREV_TAG_SIZE_EOF, flv_get_offset(flv_in), "unexpected end of file in previous tag size");
            goto end;
        }

        if (prev_tag_size != FLV_TAG_SIZE + body_length) {
            sprintf(message, "previous tag size should be %d, %d found instead", FLV_TAG_SIZE + body_length, prev_tag_size);
            print_error(ERROR_PREV_TAG_SIZE_BAD, flv_get_offset(flv_in), message);
        }
    }

    /** final checks */

    /* check consistency with global header */
    if (!have_video && flv_header_has_video(header)) {
        print_warning(WARNING_HEADER_VIDEO_NOT_FOUND, 4, "no video tag found despite header signaling the file contains video");
    }
    if (!have_audio && flv_header_has_audio(header)) {
        print_warning(WARNING_HEADER_AUDIO_NOT_FOUND, 4, "no audio tag found despite header signaling the file contains audio");
    }

    /* check last timestamps */
    if (have_video && have_audio && labs(last_audio_timestamp - last_video_timestamp) >= 1000) {
        if (last_audio_timestamp > last_video_timestamp) {
            sprintf(message, "video stops %d ms before audio", last_audio_timestamp - last_video_timestamp);
            print_warning(WARNING_TIMESTAMP_VIDEO_ENDS_FIRST, file_stats.st_size, message);
        }
        else {
            sprintf(message, "audio stops %d ms before video", last_video_timestamp - last_audio_timestamp);
            print_warning(WARNING_TIMESTAMP_AUDIO_ENDS_FIRST, file_stats.st_size, message);
        }
    }

    /* check video keyframes */
    if (have_video && keyframes_number == 0) {
        print_warning(WARNING_VIDEO_NO_KEYFRAME, file_stats.st_size, "no keyframe detected, file is probably broken or incomplete");
    }
    if (have_video && keyframes_number == video_frames_number) {
        print_warning(WARNING_VIDEO_ONLY_KEYFRAMES, file_stats.st_size, "only keyframes detected, probably inefficient compression scheme used");
    }

    /* only keyframes + onLastSecond bug */
    if (have_video && have_on_last_second && keyframes_number == video_frames_number) {
        print_warning(WARNING_VIDEO_ONLY_KF_LAST_SEC, file_stats.st_size, "only keyframes detected and onLastSecond event present, file is probably not playable");
    }

    /* check onLastSecond timestamp */
    if (have_on_last_second && (last_timestamp - on_last_second_timestamp) >= 2000) {
        sprintf(message, "onLastSecond event located %d ms before the last tag", last_timestamp - on_last_second_timestamp);
        print_warning(WARNING_METADATA_LAST_SECOND_BAD, file_stats.st_size, message);
    }

    /* check onMetaData presence */
    if (!have_on_metadata) {
        print_warning(WARNING_METADATA_NOT_PRESENT, file_stats.st_size, "onMetaData event not found, file might not be playable");
    }
    else {
        amf_node * n;
        int have_width, have_height;

        have_width = 0;
        have_height = 0;

        /* compute metadata, with a sensible set of unobstrusive options */
        opts_loc.verbose = 0;
        opts_loc.reset_timestamps = 0;
        opts_loc.preserve_metadata = 0;
        opts_loc.all_keyframes = 0;
        opts_loc.error_handling = FLVMETA_IGNORE_ERRORS;
        opts_loc.insert_onlastsecond = 0;

        flv_reset(flv_in);
        if (get_flv_info(flv_in, &info, &opts_loc) != OK) {
            print_fatal(FATAL_INFO_COMPUTATION_ERROR, 0, "unable to compute file information");
            goto end;
        }

        /* delete useless info data */
        amf_data_free(info.original_on_metadata);

        /* more metadata checks */
        for (n = amf_associative_array_first(on_metadata); n != NULL; n = amf_associative_array_next(n)) {
            byte * name;
            amf_data * data;
            byte type;
            number64 duration;

            if (info.have_audio) {
                duration = (info.last_timestamp - info.first_timestamp + info.audio_frame_duration) / 1000.0;
            }
            else {
                duration = (info.last_timestamp - info.first_timestamp + info.video_frame_duration) / 1000.0;
            }

            name = amf_string_get_bytes(amf_associative_array_get_name(n));
            data = amf_associative_array_get_data(n);
            type = amf_data_get_type(data);

            /* TODO: check UTF-8 strings, in key, and value if string type */

            /* hasMetadata (bool): true */
            if (!strcmp((char*)name, "hasMetadata")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) == 0) {
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, "hasMetadata should be set to true");
                    }
                }
                else {
                    sprintf(message, "invalid type for hasMetadata: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* hasVideo (bool) */
            if (!strcmp((char*)name, "hasVideo")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.have_video) {
                        sprintf(message, "hasVideo should be set to %s", info.have_video ? "true" : "false");
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for hasVideo: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* hasAudio (bool) */
            if (!strcmp((char*)name, "hasAudio")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.have_audio) {
                        sprintf(message, "hasAudio should be set to %s", info.have_audio ? "true" : "false");
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for hasAudio: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* duration (number) */
            if (!strcmp((char*)name, "duration")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 file_duration;
                    file_duration = amf_number_get_value(data);

                    if (fabs(file_duration - duration) >= 1.0) {
                        sprintf(message, "duration should be %.12g, got %.12g", duration, file_duration);
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for duration: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* lasttimestamp: (number) */
            if (!strcmp((char*)name, "lasttimestamp")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 lasttimestamp, file_lasttimestamp;
                    lasttimestamp = info.last_timestamp / 1000.0;
                    file_lasttimestamp = amf_number_get_value(data);

                    if (fabs(file_lasttimestamp - lasttimestamp) >= 1.0) {
                        sprintf(message, "lasttimestamp should be %.12g, got %.12g", lasttimestamp, file_lasttimestamp);
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for lasttimestamp: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* lastkeyframetimestamp: (number) */
            if (!strcmp((char*)name, "lastkeyframetimestamp")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 lastkeyframetimestamp, file_lastkeyframetimestamp;
                    lastkeyframetimestamp = info.last_keyframe_timestamp / 1000.0;
                    file_lastkeyframetimestamp = amf_number_get_value(data);

                    if (fabs(file_lastkeyframetimestamp - lastkeyframetimestamp) >= 1.0) {
                        sprintf(message, "lastkeyframetimestamp should be %.12g, got %.12g",
                            lastkeyframetimestamp,
                            file_lastkeyframetimestamp);
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for lastkeyframetimestamp: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* width: (number) */
            if (!strcmp((char*)name, "width")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 width, file_width;
                        width = info.video_width;
                        file_width = amf_number_get_value(data);

                        if (fabs(file_width - width) >= 1.0) {
                            sprintf(message, "width should be %.12g, got %.12g", width, file_width);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                        have_width = 1;
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "width metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for width: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* height: (number) */
            if (!strcmp((char*)name, "height")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 height, file_height;
                        height = info.video_height;
                        file_height = amf_number_get_value(data);

                        if (fabs(file_height - height) >= 1.0) {
                            sprintf(message, "height should be %.12g, got %.12g", height, file_height);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                        have_height = 1;
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "height metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for height: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* videodatarate: (number)*/
            if (!strcmp((char*)name, "videodatarate")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 videodatarate, file_videodatarate;
                        videodatarate = ((info.real_video_data_size / 1024.0) * 8.0) / duration;
                        file_videodatarate = amf_number_get_value(data);

                        if (fabs(file_videodatarate - videodatarate) >= 1.0) {
                            sprintf(message, "videodatarate should be %.12g, got %.12g", videodatarate, file_videodatarate);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "videodatarate metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for videodatarate: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* framerate: (number) */
            if (!strcmp((char*)name, "framerate")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 framerate, file_framerate;
                        framerate = info.video_frames_number / duration;
                        file_framerate = amf_number_get_value(data);

                        if (fabs(file_framerate - framerate) >= 1.0) {
                            sprintf(message, "framerate should be %.12g, got %.12g", framerate, file_framerate);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "framerate metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for framerate: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiodatarate: (number) */
            if (!strcmp((char*)name, "audiodatarate")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio) {
                        number64 audiodatarate, file_audiodatarate;
                        audiodatarate = ((info.real_audio_data_size / 1024.0) * 8.0) / duration;
                        file_audiodatarate = amf_number_get_value(data);

                        if (fabs(file_audiodatarate - audiodatarate) >= 1.0) {
                            sprintf(message, "audiodatarate should be %.12g, got %.12g", audiodatarate, file_audiodatarate);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "audiodatarate metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiodatarate: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiosamplerate: (number) */
            if (!strcmp((char*)name, "audiosamplerate")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio) {
                        number64 audiosamplerate, file_audiosamplerate;
                        audiosamplerate = 0.0;
                        switch (info.audio_rate) {
                            case FLV_AUDIO_TAG_SOUND_RATE_5_5: audiosamplerate = 5500.0; break;
                            case FLV_AUDIO_TAG_SOUND_RATE_11:  audiosamplerate = 11000.0; break;
                            case FLV_AUDIO_TAG_SOUND_RATE_22:  audiosamplerate = 22050.0; break;
                            case FLV_AUDIO_TAG_SOUND_RATE_44:  audiosamplerate = 44100.0; break;
                        }
                        file_audiosamplerate = amf_number_get_value(data);

                        /* 100 tolerance, since 44000 is sometimes used instead of 44100 */
                        if (fabs(file_audiosamplerate - audiosamplerate) > 100.0) {
                            sprintf(message, "audiosamplerate should be %.12g, got %.12g", audiosamplerate, file_audiosamplerate);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "audiosamplerate metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiosamplerate: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiosamplesize: (number) */
            if (!strcmp((char*)name, "audiosamplesize")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio) {
                        number64 audiosamplesize, file_audiosamplesize;
                        audiosamplesize = 0.0;
                        switch (info.audio_size) {
                            case FLV_AUDIO_TAG_SOUND_SIZE_8:  audiosamplesize = 8.0; break;
                            case FLV_AUDIO_TAG_SOUND_SIZE_16: audiosamplesize = 16.0; break;
                        }
                        file_audiosamplesize = amf_number_get_value(data);

                        if (fabs(file_audiosamplesize - audiosamplesize) >= 1.0) {
                            sprintf(message, "audiosamplesize should be %.12g, got %.12g", audiosamplesize, file_audiosamplesize);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "audiosamplesize metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiosamplesize: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* stereo: (boolean) */
            if (!strcmp((char*)name, "stereo")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (info.have_audio) {
                        uint8 stereo, file_stereo;
                        stereo = (info.audio_stereo == FLV_AUDIO_TAG_SOUND_TYPE_STEREO);
                        file_stereo = amf_boolean_get_value(data);

                        if (file_stereo != stereo) {
                            sprintf(message, "stereo should be %s", stereo ? "true" : "false");
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "stereo metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for stereo: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* filesize: (number) */
            if (!strcmp((char*)name, "filesize")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 filesize, file_filesize;

                    filesize = (number64)(file_stats.st_size);
                    file_filesize = amf_number_get_value(data);

                    if (fabs(file_filesize - filesize) >= 1.0) {
                        sprintf(message, "filesize should be %.12g, got %.12g", filesize, file_filesize);
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for filesize: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* videosize: (number) */
            if (!strcmp((char*)name, "videosize")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 videosize, file_videosize;
                        videosize = (number64)(info.video_data_size);
                        file_videosize = amf_number_get_value(data);

                        if (fabs(file_videosize - videosize) >= 1.0) {
                            sprintf(message, "videosize should be %.12g, got %.12g", videosize, file_videosize);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "videosize metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for videosize: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiosize: (number) */
            if (!strcmp((char*)name, "audiosize")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio) {
                        number64 audiosize, file_audiosize;
                        audiosize = (number64)(info.audio_data_size);
                        file_audiosize = amf_number_get_value(data);

                        if (fabs(file_audiosize - audiosize) >= 1.0) {
                            sprintf(message, "audiosize should be %.12g, got %.12g", audiosize, file_audiosize);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "audiosize metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiosize: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* datasize: (number) */
            if (!strcmp((char*)name, "datasize")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 datasize, file_datasize;
                    uint32 on_metadata_size;
                    
                    on_metadata_size = FLV_TAG_SIZE +
                        (uint32)(amf_data_size(on_metadata_name) + amf_data_size(on_metadata));
                    datasize = (number64)(info.meta_data_size + on_metadata_size);
                    file_datasize = amf_number_get_value(data);

                    if (fabs(file_datasize - datasize) >= 1.0) {
                        sprintf(message, "datasize should be %.12g, got %.12g", datasize, file_datasize);
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for datasize: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiocodecid: (number) */
            if (!strcmp((char*)name, "audiocodecid")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio) {
                        number64 audiocodecid, file_audiocodecid;
                        audiocodecid = (number64)info.audio_codec;
                        file_audiocodecid = amf_number_get_value(data);

                        if (fabs(file_audiocodecid - audiocodecid) >= 1.0) {
                            sprintf(message, "audiocodecid should be %.12g, got %.12g", audiocodecid, file_audiocodecid);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_NEEDED, on_metadata_offset, "audiocodecid metadata present without audio data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiocodecid: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* videocodecid: (number) */
            if (!strcmp((char*)name, "videocodecid")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 videocodecid, file_videocodecid;
                        videocodecid = (number64)info.video_codec;
                        file_videocodecid = amf_number_get_value(data);

                        if (fabs(file_videocodecid - videocodecid) >= 1.0) {
                            sprintf(message, "videocodecid should be %.12g, got %.12g", videocodecid, file_videocodecid);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_VIDEO_NEEDED, on_metadata_offset, "videocodecid metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for videocodecid: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* audiodelay: (number) */
            if (!strcmp((char*)name, "audiodelay")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_audio && info.have_video) {
                        number64 audiodelay, file_audiodelay;
                        audiodelay = ((sint32)info.audio_first_timestamp - (sint32)info.video_first_timestamp) / 1000.0;
                        file_audiodelay = amf_number_get_value(data);

                        if (fabs(file_audiodelay - audiodelay) >= 1.0) {
                            sprintf(message, "audiodelay should be %.12g, got %.12g", audiodelay, file_audiodelay);
                            print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning(WARNING_AMF_DATA_AUDIO_VIDEO_NEEDED, on_metadata_offset, "audiodelay metadata present without audio and video data");
                    }
                }
                else {
                    sprintf(message, "invalid type for audiodelay: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* canSeekToEnd: (boolean) */
            if (!strcmp((char*)name, "canSeekToEnd")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.can_seek_to_end) {
                        sprintf(message, "canSeekToEnd should be set to %s", info.can_seek_to_end ? "true" : "false");
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for canSeekToEnd: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* hasKeyframes: (boolean) */
            if (!strcmp((char*)name, "hasKeyframes")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.have_keyframes) {
                        sprintf(message, "hasKeyframes should be set to %s", info.have_keyframes ? "true" : "false");
                        print_warning(WARNING_AMF_DATA_INVALID_VALUE, on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "invalid type for hasKeyframes: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }

            /* keyframes: (object) */
            if (!strcmp((char*)name, "keyframes")) {
                if (type == AMF_TYPE_OBJECT) {
                    amf_data * file_times, * file_filepositions;
                    
                    file_times = amf_object_get(data, "times");
                    file_filepositions = amf_object_get(data, "filepositions");

                    /* check sub-arrays' presence */
                    if (file_times == NULL) {
                        print_warning(WARNING_KEYFRAMES_TIMES_MISSING, on_metadata_offset, "Missing times metadata");
                    }
                    if (file_filepositions == NULL) {
                        print_warning(WARNING_KEYFRAMES_FILEPOS_MISSING, on_metadata_offset, "Missing filepositions metadata");
                    }

                    if (file_times != NULL && file_filepositions != NULL) {
                        /* check types */
                        uint8 times_type, fp_type;

                        times_type = amf_data_get_type(file_times);
                        if (times_type != AMF_TYPE_ARRAY) {
                            sprintf(message, "invalid type for times: expected %s, got %s",
                                get_amf_type_string(AMF_TYPE_ARRAY),
                                get_amf_type_string(times_type));
                            print_warning(WARNING_KEYFRAMES_TIMES_TYPE_BAD, on_metadata_offset, message);
                        }

                        fp_type = amf_data_get_type(file_filepositions);
                        if (fp_type != AMF_TYPE_ARRAY) {
                            sprintf(message, "invalid type for filepositions: expected %s, got %s",
                                get_amf_type_string(AMF_TYPE_ARRAY),
                                get_amf_type_string(fp_type));
                            print_warning(WARNING_KEYFRAMES_FILEPOS_TYPE_BAD, on_metadata_offset, message);
                        }

                        if (times_type == AMF_TYPE_ARRAY && fp_type == AMF_TYPE_ARRAY) {
                            /* check array sizes */
                            if (amf_array_size(info.times) != amf_array_size(file_times) ||
                                amf_array_size(info.filepositions) != amf_array_size(file_filepositions) ||
                                amf_array_size(file_filepositions) != amf_array_size(file_times)) {
                                print_warning(WARNING_KEYFRAMES_ARRAY_LENGTH_BAD, on_metadata_offset, "invalid keyframes arrays length");
                            }
                            else {
                                number64 last_file_time;
                                int have_last_time;
                                amf_node * ff_node, * ft_node, * f_node, * t_node;

                                /* iterate in parallel, report diffs */
                                last_file_time = 0;
                                have_last_time = 0;

                                t_node = amf_array_first(info.times);
                                f_node = amf_array_first(info.filepositions);
                                ft_node = amf_array_first(file_times);
                                ff_node = amf_array_first(file_filepositions);

                                while (t_node != NULL && f_node != NULL && ft_node != NULL && ff_node != NULL) {
                                    number64 time, f_time, position, f_position;
                                    time = amf_number_get_value(amf_array_get(t_node));
                                    position = amf_number_get_value(amf_array_get(f_node));

                                    /* time */
                                    if (amf_data_get_type(amf_array_get(ft_node)) != AMF_TYPE_NUMBER) {
                                        sprintf(message, "invalid type for time: expected %s, got %s",
                                            get_amf_type_string(AMF_TYPE_NUMBER),
                                            get_amf_type_string(type));
                                        print_warning(WARNING_KEYFRAMES_TIME_TYPE_BAD, on_metadata_offset, message);
                                    }
                                    else {
                                        f_time = amf_number_get_value(amf_array_get(ft_node));

                                        if (fabs(time - f_time) >= 1.0) {
                                            sprintf(message, "invalid keyframe time: expected %.12g, got %.12g",
                                                time, f_time);
                                            print_warning(WARNING_KEYFRAMES_TIME_BAD, on_metadata_offset, message);
                                        }
                                        
                                        /* check for duplicate time, can happen in H.264 files */
                                        if (have_last_time && last_file_time == f_time) {
                                            sprintf(message, "Duplicate keyframe time: %.12g", f_time);
                                            print_warning(WARNING_KEYFRAMES_TIME_DUPLICATE, on_metadata_offset, message);
                                        }
                                        have_last_time = 1;
                                        last_file_time = f_time;
                                    }

                                    /* position */
                                    if (amf_data_get_type(amf_array_get(ff_node)) != AMF_TYPE_NUMBER) {
                                        sprintf(message, "invalid type for file position: expected %s, got %s",
                                            get_amf_type_string(AMF_TYPE_NUMBER),
                                            get_amf_type_string(type));
                                        print_warning(WARNING_KEYFRAMES_POS_TYPE_BAD, on_metadata_offset, message);
                                    }
                                    else {
                                        f_position = amf_number_get_value(amf_array_get(ff_node));

                                        if (fabs(position - f_position) >= 1.0) {
                                            sprintf(message, "invalid keyframe file position: expected %.12g, got %.12g",
                                                position, f_position);
                                            print_warning(WARNING_KEYFRAMES_POS_BAD, on_metadata_offset, message);
                                        }
                                    }

                                    /* next entry */
                                    t_node = amf_array_next(t_node);
                                    f_node = amf_array_next(f_node);
                                    ft_node = amf_array_next(ft_node);
                                    ff_node = amf_array_next(ff_node);
                                }
                            }
                        }
                    }
                }
                else {
                    sprintf(message, "invalid type for keyframes: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning(WARNING_AMF_DATA_INVALID_TYPE, on_metadata_offset, message);
                }
            }
        }

        /* we need to release the info.keyframes pointer, because
           as opposed to update.c, these amf data do not get added
           into another object, therefore keep memory ownership */
        amf_data_free(info.keyframes);

        /* missing width or height can cause size problem in various players */
        if (info.have_video) {
            if (!have_width) {
                print_error(ERROR_VIDEO_WIDTH_MISSING, on_metadata_offset, "width information not found in metadata, problems might occur in some players");
            }
            if (!have_height) {
                print_error(ERROR_VIDEO_HEIGHT_MISSING, on_metadata_offset, "height information not found in metadata, problems might occur in some players");
            }
        }
    }

    /* could we compute video resolution ? */
    if (info.video_width == 0 && info.video_height == 0) {
        print_warning(WARNING_VIDEO_SIZE_ERROR, file_stats.st_size, "unable to determine video resolution");
    }

    /* global info */

    if (info.have_video) {
        /* video codec */
        sprintf(message, "video codec is %s", dump_string_get_video_codec(prev_video_tag));
        print_info(INFO_VIDEO_CODEC, 0, message);

    }

    if (info.have_audio) {
        /* audio info */
        sprintf(message, "audio format is %s (%s, %s-bit, %s kHz)",
            dump_string_get_sound_format(prev_audio_tag),
            dump_string_get_sound_type(prev_audio_tag),
            dump_string_get_sound_size(prev_audio_tag),
            dump_string_get_sound_rate(prev_audio_tag)
        );
        print_info(INFO_AUDIO_FORMAT, 0, message);
    }

    /* does the file use extended timestamps ? */
    if (last_timestamp > 0x00FFFFFF) {
        print_info(INFO_TIMESTAMP_USE_EXTENDED, 0, "extended timestamps used in the file");
    }

    /* is the file larger than 4GB ? */
    if (file_stats.st_size > 0xFFFFFFFFULL) {
        print_info(INFO_GENERAL_LARGE_FILE, 0, "file is larger than 4 GB");
    }

end:
    report_end(opts, errors, warnings);
    
    amf_data_free(on_metadata);
    amf_data_free(on_metadata_name);
    flv_close(flv_in);
    
    return (errors > 0) ? ERROR_INVALID_FLV_FILE : OK;
}
