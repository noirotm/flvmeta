/*
    $Id: check.c 236 2011-08-10 15:30:04Z marc.noirot@gmail.com $

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
#include "check.h"
#include "info.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_ACCEPTABLE_TAG_BODY_LENGTH 100000

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
    amf_data * on_metadata;
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
    on_metadata = NULL;
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
        print_fatal("F11001", 0, "unexpected end of file in header");
        goto end;
    }
    else if (result == FLV_ERROR_NO_FLV) {
        print_fatal("F11002", 0, "FLV signature not found in header");
        goto end;
    }

    /* version */
    if (header.version != FLV_VERSION) {
        sprintf(message, "header version should be 1, %d found instead", header.version);
        print_error("E11003", 3, message);
    }

    /* video and audio flags */
    if (!flv_header_has_audio(header) && !flv_header_has_video(header)) {
        print_error("E11004", 4, "header signals the file does not contain video tags or audio tags");
    }
    else if (!flv_header_has_audio(header)) {
        print_info("I11005", 4, "header signals the file does not contain audio tags");
    }
    else if (!flv_header_has_video(header)) {
        print_warning("W11006", 4, "header signals the file does not contain video tags");
    }

    /* reserved flags */
    if (header.flags & 0xFA) {
        print_error("E11007", 4, "header reserved flags are not zero");
    }

    /* offset */
    if (flv_header_get_offset(header) != 9) {
        sprintf(message, "header offset should be 9, %d found instead", flv_header_get_offset(header));
        print_error("E11008", 5, message);
    }

    /** check first previous tag size **/

    result = flv_read_prev_tag_size(flv_in, &prev_tag_size);
    if (result == FLV_ERROR_EOF) {
        print_fatal("F12009", 9, "unexpected end of file in previous tag size");
        goto end;
    }
    else if (prev_tag_size != 0) {
        sprintf(message, "first previous tag size should be 0, %d found instead", prev_tag_size);
        print_error("E12010", 9, message);
    }

    /* we reached the end of file: no tags in file */
    if (flv_get_offset(flv_in) == file_stats.st_size) {
        print_fatal("F10011", 13, "file does not contain tags");
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
            print_fatal("F20012", flv_get_offset(flv_in), "unexpected end of file in tag");
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
            print_error("E30013", offset, message);
        }

        /* check consistency with global header */
        if (!have_video && tag.type == FLV_TAG_TYPE_VIDEO) {
            if (!flv_header_has_video(header)) {
                print_warning("W11014", offset, "video tag found despite header signaling the file contains no video");
            }
            have_video = 1;
        }
        if (!have_audio && tag.type == FLV_TAG_TYPE_AUDIO) {
            if (!flv_header_has_audio(header)) {
                print_warning("W11015", offset, "audio tag found despite header signaling the file contains no audio");
            }
            have_audio = 1;
        }

        /* check body length */
        if (body_length > (file_stats.st_size - flv_get_offset(flv_in))) {
            sprintf(message, "tag body length (%d bytes) exceeds file size", body_length);
            print_fatal("F20016", offset + 1, message);
            goto end;
        }
        else if (body_length > MAX_ACCEPTABLE_TAG_BODY_LENGTH) {
            sprintf(message, "tag body length (%d bytes) is abnormally large", body_length);
            print_warning("W20017", offset + 1, message);
        }
        else if (body_length == 0) {
            print_warning("W20018", offset + 1, "tag body length is zero");
        }

        /** check timestamp **/
        decr_timestamp_signaled = 0;

        /* check whether first timestamp is zero */
        if (tag_number == 1 && timestamp != 0) {
            sprintf(message, "first timestamp should be zero, %d found instead", timestamp);
            print_error("E40019", offset + 4, message);
        }

        /* check whether timestamps decrease in a given stream */
        if (tag.type == FLV_TAG_TYPE_AUDIO) {
            if (last_audio_timestamp > timestamp) {
                sprintf(message, "audio tag timestamps are decreasing from %d to %d", last_audio_timestamp, timestamp);
                print_error("E40020", offset + 4, message);
            }
            last_audio_timestamp = timestamp;
            decr_timestamp_signaled = 1;
        }
        if (tag.type == FLV_TAG_TYPE_VIDEO) {
            if (last_video_timestamp > timestamp) {
                sprintf(message, "video tag timestamps are decreasing from %d to %d", last_video_timestamp, timestamp);
                print_error("E40021", offset + 4, message);
            }
            last_video_timestamp = timestamp;
            decr_timestamp_signaled = 1;
        }

        /* check for overflow error */
        if (last_timestamp > timestamp && last_timestamp - timestamp > 0xF00000) {
            print_error("E40022", offset + 4, "extended bits not used after timestamp overflow");
        }

        /* check whether timestamps decrease globally */
        else if (!decr_timestamp_signaled && last_timestamp > timestamp && last_timestamp - timestamp >= 1000) {
            sprintf(message, "timestamps are decreasing from %d to %d", last_timestamp, timestamp);
            print_error("E40023", offset + 4, message);
        }

        last_timestamp = timestamp;

        /* check for desyncs between audio and video: one second or more is suspicious */
        if (have_video && have_audio && !have_desync && labs(last_video_timestamp - last_audio_timestamp) >= 1000) {
            sprintf(message, "audio and video streams are desynchronized by %ld ms",
                labs(last_video_timestamp - last_audio_timestamp));
            print_warning("W40024", offset + 4, message);
            have_desync = 1; /* do not repeat */
        }

        /** stream id must be zero **/
        if (stream_id != 0) {
            sprintf(message, "tag stream id must be zero, %d found instead", stream_id);
            print_error("E20025", offset + 8, message);
        }

        /* check tag body contents only if not empty */
        if (body_length > 0) {

            /** check audio info **/
            if (tag.type == FLV_TAG_TYPE_AUDIO) {
                flv_audio_tag at;
                uint8_bitmask audio_format;

                result = flv_read_audio_tag(flv_in, &at);
                if (result == FLV_ERROR_EOF) {
                    print_fatal("F20012", offset + 11, "unexpected end of file in tag");
                    goto end;
                }

                /* check whether the format varies between tags */
                if (have_prev_audio_tag && prev_audio_tag != at) {
                    print_warning("W51026", offset + 11, "audio format changed since last tag");
                }

                /* check format */
                audio_format = flv_audio_tag_sound_format(at);
                if (audio_format == 12 || audio_format == 13) {
                    sprintf(message, "unknown audio format %d", audio_format);
                    print_warning("W51027", offset + 11, message);
                }
                else if (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_A
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC
                ) {
                    sprintf(message, "audio format %d is reserved for internal use", audio_format);
                    print_warning("W51028", offset + 11, message);
                }

                /* check consistency, see flash video spec */
                if (flv_audio_tag_sound_rate(at) != FLV_AUDIO_TAG_SOUND_RATE_44
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning("W51029", offset + 11, "audio data in AAC format should have a 44KHz rate, field will be ignored");
                }

                if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_STEREO
                    && (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO)
                ) {
                    print_warning("W51030", offset + 11, "audio data in Nellymoser format cannot be stereo, field will be ignored");
                }

                else if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_MONO
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning("W51031", offset + 11, "audio data in AAC format should be stereo, field will be ignored");
                }

                else if (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_LINEAR_PCM) {
                    print_warning("W51032", offset + 11, "audio data in Linear PCM, platform endian format should not be used because of non-portability");
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
                    print_fatal("F20012", offset + 11, "unexpected end of file in tag");
                    goto end;
                }

                /* check whether the format varies between tags */
                if (have_prev_video_tag && flv_video_tag_codec_id(prev_video_tag) != flv_video_tag_codec_id(vt)) {
                    print_warning("W60033", offset + 11, "video format changed since last tag");
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
                    print_error("E60034", offset + 11, message);
                }

                if (video_frame_type == FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME) {
                    keyframes_number++;
                }

                /* check whether first frame is a keyframe */
                if (!have_prev_video_tag && video_frame_type != FLV_VIDEO_TAG_FRAME_TYPE_KEYFRAME) {
                    print_warning("W60035", offset + 11, "first video frame is not a keyframe, playback will suffer");
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
                    print_error("E61034", offset + 11, message);
                }

                /* according to spec, JPEG codec is not currently used */
                if (video_codec == FLV_VIDEO_TAG_CODEC_JPEG) {
                    print_warning("W61035", offset + 11, "JPEG codec not currently used");
                }

                prev_video_tag = vt;
                have_prev_video_tag = 1;
            }
            /** check script data info **/
            else if (tag.type == FLV_TAG_TYPE_META) {
                amf_data * name;
                amf_data * data;

                result = flv_read_metadata(flv_in, &name, &data);

                if (result == FLV_ERROR_EOF) {
                    print_fatal("F20012", offset + 11, "unexpected end of file in tag");
                    amf_data_free(name);
                    amf_data_free(data);
                    goto end;
                }
                else if (result == FLV_ERROR_EMPTY_TAG) {
                    print_warning("W70038", offset + 11, "empty metadata tag");
                }
                else if (result == FLV_ERROR_INVALID_METADATA_NAME) {
                    print_error("E70039", offset + 11, "invalid metadata name");
                }
                else if (result == FLV_ERROR_INVALID_METADATA) {
                    print_error("E70039", offset + 11, "invalid metadata");
                }
                else if (amf_data_get_type(name) != AMF_TYPE_STRING) {
                    /* name type checking */
                    sprintf(message, "invalid metadata name type: %d, should be a string (2)", amf_data_get_type(name));
                    print_error("E70038", offset, message);
                }
                else {
                    /* empty name checking */
                    if (amf_string_get_size(name) == 0) {
                        print_warning("W70038", offset, "empty metadata name");
                    }

                    /* check whether all body size has been read */
                    if (flv_in->current_tag_body_length > 0) {
                        sprintf(message, "%d bytes not read in tag body after metadata end", body_length - flv_in->current_tag_body_length);
                        print_warning("W70040", flv_get_offset(flv_in), message);
                    }
                    else if (flv_in->current_tag_body_length < 0) {
                        sprintf(message, "%d bytes missing from tag body after metadata end", flv_in->current_tag_body_length - body_length);
                        print_warning("W70041", flv_get_offset(flv_in), message);
                    }

                    /* onLastSecond checking */
                    if (!strcmp((char*)amf_string_get_bytes(name), "onLastSecond")) {
                        if (have_on_last_second == 0) {
                            have_on_last_second = 1;
                            on_last_second_timestamp = timestamp;
                        }
                        else {
                            print_warning("W70038", offset, "duplicate onLastSecond event");
                        }
                    }

                    /* onMetaData checking */
                    if (!strcmp((char*)amf_string_get_bytes(name), "onMetaData")) {
                        if (have_on_metadata == 0) {
                            have_on_metadata = 1;
                            on_metadata_offset = offset;
                            on_metadata = amf_data_clone(data);

                            /* check onMetadata type */
                            if (amf_data_get_type(on_metadata) != AMF_TYPE_ASSOCIATIVE_ARRAY) {
                                sprintf(message, "invalid onMetaData data type: %d, should be an associative array (8)", amf_data_get_type(on_metadata));
                                print_error("E70038", offset, message);
                            }

                            /* onMetaData must be the first tag at 0 timestamp */
                            if (tag_number != 1) {
                                print_warning("W70038", offset, "onMetadata event found after the first tag");
                            }
                            if (timestamp != 0) {
                                print_warning("W70038", offset, "onMetadata event found after timestamp zero");
                            }
                        }
                        else {
                            print_warning("W70038", offset, "duplicate onMetaData event");
                        }
                    }

                    /* unknown metadata name */
                    if (strcmp((char*)amf_string_get_bytes(name), "onMetaData")
                    && strcmp((char*)amf_string_get_bytes(name), "onCuePoint")
                    && strcmp((char*)amf_string_get_bytes(name), "onLastSecond")) {
                        sprintf(message, "unknown metadata event name: '%s'", (char*)amf_string_get_bytes(name));
                        print_info("I70039", flv_get_offset(flv_in), message);
                    }
                }

                amf_data_free(name);
                amf_data_free(data);
            }
        }

        /* check body length against previous tag size */
        result = flv_read_prev_tag_size(flv_in, &prev_tag_size);
        if (result != FLV_OK) {
            print_fatal("F12036", flv_get_offset(flv_in), "unexpected end of file after tag");
            goto end;
        }

        if (prev_tag_size != FLV_TAG_SIZE + body_length) {
            sprintf(message, "previous tag size should be %d, %d found instead", FLV_TAG_SIZE + body_length, prev_tag_size);
            print_error("E12037", flv_get_offset(flv_in), message);
            goto end;
        }
    }

    /** final checks */

    /* check consistency with global header */
    if (!have_video && flv_header_has_video(header)) {
        print_warning("W11038", 4, "no video tag found despite header signaling the file contains video");
    }
    if (!have_audio && flv_header_has_audio(header)) {
        print_warning("W11039", 4, "no audio tag found despite header signaling the file contains audio");
    }

    /* check last timestamps */
    if (have_video && have_audio && labs(last_audio_timestamp - last_video_timestamp) >= 1000) {
        if (last_audio_timestamp > last_video_timestamp) {
            sprintf(message, "video stops %d ms before audio", last_audio_timestamp - last_video_timestamp);
            print_warning("W40040", file_stats.st_size, message);
        }
        else {
            sprintf(message, "audio stops %d ms before video", last_video_timestamp - last_audio_timestamp);
            print_warning("W40041", file_stats.st_size, message);
        }
    }

    /* check video keyframes */
    if (have_video && keyframes_number == 0) {
        print_warning("W60042", file_stats.st_size, "no keyframe detected, file is probably broken or incomplete");
    }
    if (have_video && keyframes_number == video_frames_number) {
        print_warning("W60043", file_stats.st_size, "only keyframes detected, probably inefficient compression scheme used");
    }

    /* only keyframes + onLastSecond bug */
    if (have_video && have_on_last_second && keyframes_number == video_frames_number) {
        print_warning("W60044", file_stats.st_size, "only keyframes detected and onLastSecond event present, file is probably not playable");
    }

    /* check onLastSecond timestamp */
    if (have_on_last_second && (last_timestamp - on_last_second_timestamp) >= 2000) {
        sprintf(message, "onLastSecond event located %d ms before the last tag", last_timestamp - on_last_second_timestamp);
        print_warning("W70050", file_stats.st_size, message);
    }

    /* check onMetaData presence */
    if (!have_on_metadata) {
        print_warning("W70044", file_stats.st_size, "onMetaData event not found, file might not be playable");
    }
    else {
        amf_node * n;
        int have_width, have_height;

        have_width = 0;
        have_height = 0;

        /* compute metadata */
        opts_loc.verbose = 0;
        opts_loc.reset_timestamps = 0;
        opts_loc.preserve_metadata = 0;
        opts_loc.all_keyframes = 0;
        opts_loc.error_handling = FLVMETA_IGNORE_ERRORS;

        flv_reset(flv_in);
        if (get_flv_info(flv_in, &info, &opts_loc) != OK) {
            print_fatal("F10042", 0, "unable to compute metadata");
            goto end;
        }

        /* more metadata checks */
        for (n = amf_associative_array_first(on_metadata); n != NULL; n = amf_associative_array_next(n)) {
            byte * name;
            amf_data * data;
            byte type;

            name = amf_string_get_bytes(amf_associative_array_get_name(n));
            data = amf_associative_array_get_data(n);
            type = amf_data_get_type(data);

            /* hasMetadata (bool): true */
            if (!strcmp((char*)name, "hasMetadata")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) == 0) {
                        print_warning("W70045", on_metadata_offset, "hasMetadata should be set to true");
                    }
                }
                else {
                    sprintf(message, "Invalid type for hasMetadata: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* hasVideo (bool) */
            if (!strcmp((char*)name, "hasVideo")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.have_video) {
                        sprintf(message, "hasVideo should be set to %s", info.have_video ? "true" : "false");
                        print_warning("W70045", on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "Invalid type for hasVideo: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* hasAudio (bool) */
            if (!strcmp((char*)name, "hasAudio")) {
                if (type == AMF_TYPE_BOOLEAN) {
                    if (amf_boolean_get_value(data) != info.have_audio) {
                        sprintf(message, "hasAudio should be set to %s", info.have_audio ? "true" : "false");
                        print_warning("W70045", on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "Invalid type for hasAudio: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_BOOLEAN),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* duration (number) */
            if (!strcmp((char*)name, "duration")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 duration, file_duration;
                    if (info.have_audio) {
                        duration = (info.last_timestamp - info.first_timestamp + info.audio_frame_duration) / 1000.0;
                    }
                    else {
                        duration = (info.last_timestamp - info.first_timestamp + info.video_frame_duration) / 1000.0;
                    }
                    file_duration = amf_number_get_value(data);

                    if (fabs(file_duration - duration) >= 1.0) {
                        sprintf(message, "duration should be %.12g, got %.12g", duration, file_duration);
                        print_warning("W70045", on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "Invalid type for duration: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* lasttimestamp: (number) */
            if (!strcmp((char*)name, "lasttimestamp")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 last_timestamp, file_last_timestamp;
                    last_timestamp = info.last_timestamp;
                    file_last_timestamp = amf_number_get_value(data);

                    if (fabs(file_last_timestamp - last_timestamp) >= 1.0) {
                        sprintf(message, "lasttimestamp should be %.12g, got %.12g", last_timestamp, file_last_timestamp);
                        print_warning("W70045", on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "Invalid type for lasttimestamp: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* lastkeyframetimestamp: (number) */
            if (!strcmp((char*)name, "lastkeyframetimestamp")) {
                if (type == AMF_TYPE_NUMBER) {
                    number64 last_kftimestamp, file_last_kftimestamp;
                    last_kftimestamp = info.last_keyframe_timestamp;
                    file_last_kftimestamp = amf_number_get_value(data);

                    if (fabs(file_last_kftimestamp - last_kftimestamp) >= 1.0) {
                        sprintf(message, "lastkeyframetimestamp should be %.12g, got %.12g", last_timestamp, file_last_kftimestamp);
                        print_warning("W70045", on_metadata_offset, message);
                    }
                }
                else {
                    sprintf(message, "Invalid type for lastkeyframetimestamp: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
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
                            print_warning("W70045", on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning("W70047", on_metadata_offset, "width metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "Invalid type for width: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
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
                            print_warning("W70045", on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning("W70047", on_metadata_offset, "height metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "Invalid type for height: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* videodatarate: (number)*/
            if (!strcmp((char*)name, "videodatarate")) {
                if (type == AMF_TYPE_NUMBER) {
                    if (info.have_video) {
                        number64 videodatarate, file_videodatarate, duration;
                        duration = (info.last_timestamp - info.first_timestamp + info.audio_frame_duration) / 1000.0;
                        videodatarate = ((info.real_video_data_size / 1024.0) * 8.0) / duration;
                        file_videodatarate = amf_number_get_value(data);

                        if (fabs(file_videodatarate - videodatarate) >= 1.0) {
                            sprintf(message, "videodatarate should be %.12g, got %.12g", videodatarate, file_videodatarate);
                            print_warning("W70045", on_metadata_offset, message);
                        }
                    }
                    else {
                        print_warning("W70047", on_metadata_offset, "videodatarate metadata present without video data");
                    }
                }
                else {
                    sprintf(message, "Invalid type for videodatarate: expected %s, got %s",
                        get_amf_type_string(AMF_TYPE_NUMBER),
                        get_amf_type_string(type));
                    print_warning("W70046", on_metadata_offset, message);
                }
            }

            /* framerate: (number) */
            /* audiodatarate: (number) */
            /* audiosamplerate: (number) */
            /* audiosamplesize: (number) */
            /* stereo: (boolean) */
            /* filesize: (number) */
            /* videosize: (number) */
            /* audiosize: (number) */
            /* datasize: (number) */
            /* audiocodecid: (number) */
            /* videocodecid: (number) */
            /* audiodelay: (number) */
            /* canSeekToEnd: (boolean) */
            /* hasKeyframes: (boolean) */
            /* keyframes: (object) */
        }

        /* missing width or height can cause size problem in various players */
        if (info.have_video) {
            if (!have_width) {
                print_error("E60047", on_metadata_offset, "width information not found in metadata, problems might occur in some players");
            }
            if (!have_height) {
                print_error("E60047", on_metadata_offset, "height information not found in metadata, problems might occur in some players");
            }
        }
    }

    /* could we compute video resolution ? */
    if (info.video_width == 0 && info.video_height == 0) {
        print_warning("W60044", file_stats.st_size, "unable to determine video resolution");
    }

end:
    report_end(opts, errors, warnings);
    
    amf_data_free(on_metadata);
    flv_close(flv_in);
    
    return (errors > 0) ? ERROR_INVALID_FLV_FILE : OK;
}
