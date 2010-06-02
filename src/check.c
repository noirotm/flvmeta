/*
    $Id: check.c 176 2010-03-05 14:54:21Z marc.noirot $

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
#include "check.h"

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_ACCEPTABLE_TAG_BODY_LENGTH 50000

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


/* check FLV file validity */
int check_flv_file(const flvmeta_opts * opts) {
    flv_stream * flv_in;
    flv_header header;
    int errors, warnings;
    int result;
    char message[256];
    uint32 prev_tag_size, tag_number;
    struct stat file_stats;

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
    tag_number = 0;
    while (flv_get_offset(flv_in) < file_stats.st_size) {
        flv_tag tag;
        flv_audio_tag at;
        flv_video_tag vt;
        file_offset_t offset;
        uint32 body_length, timestamp, stream_id;

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

        /* check body length */
        if (body_length > (file_stats.st_size - flv_get_offset(flv_in))) {
            sprintf(message, "tag body length (%d bytes) exceeds file size", body_length);
            print_fatal("F20014", offset + 1, message);
            goto end;
        }
        else if (body_length > MAX_ACCEPTABLE_TAG_BODY_LENGTH) {
            sprintf(message, "tag body length (%d bytes) is abnormally large", body_length);
            print_warning("W20015", offset + 1, message);
        }
        else if (body_length == 0) {
            sprintf(message, "tag body length is zero", body_length);
            print_warning("W20016", offset + 1, message);
        }

        /** check timestamp **/

        /* check whether first timestamp is zero */
        if (tag_number == 1 && timestamp != 0) {
            sprintf(message, "first timestamp should be zero, %d found instead", timestamp);
            print_warning("W40017", offset + 4, message);
        }


        /* stream id must be zero */
        if (stream_id != 0) {
            sprintf(message, "tag stream id must be zero, %d found instead", stream_id);
            print_error("E20018", offset + 8, message);
        }

        /* check tag body contents only if not empty */
        if (body_length > 0) {

            /** check audio info **/
            if (tag.type == FLV_TAG_TYPE_AUDIO) {
                uint8_bitmask audio_format;

                result = flv_read_audio_tag(flv_in, &at);
                if (result == FLV_ERROR_EOF) {
                    print_fatal("F20012", offset + 11, "unexpected end of file in tag");
                    goto end;
                }

                /* check format */
                audio_format = flv_audio_tag_sound_format(at);
                if (audio_format == 12 || audio_format == 13) {
                    sprintf(message, "unknown audio format %d in tag", audio_format);
                    print_warning("W51019", offset + 11, message);
                }
                else if (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_A
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_G711_MU
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_RESERVED
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_MP3_8
                    || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_DEVICE_SPECIFIC
                ) {
                    sprintf(message, "audio format %d is reserved for internal use", audio_format);
                    print_warning("W51020", offset + 11, message);
                }

                /* check consistency, see flash video spec */
                if (flv_audio_tag_sound_rate(at) != FLV_AUDIO_TAG_SOUND_RATE_44
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning("W51021", offset + 11, "audio data in AAC format should have a 44KHz rate, field will be ignored");
                }

                if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_STEREO
                    && (audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_16_MONO
                        || audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_NELLYMOSER_8_MONO)
                ) {
                    print_warning("W51022", offset + 11, "audio data in Nellymoser format cannot be stereo, field will be ignored");
                }

                else if (flv_audio_tag_sound_type(at) == FLV_AUDIO_TAG_SOUND_TYPE_MONO
                    && audio_format == FLV_AUDIO_TAG_SOUND_FORMAT_AAC
                ) {
                    print_warning("W51023", offset + 11, "audio data in AAC format should be stereo, field will be ignored");
                }
            }
            /** check video info **/
            else if (tag.type == FLV_TAG_TYPE_VIDEO) {
            }
            /** check script data info **/
            else if (tag.type == FLV_TAG_TYPE_META) {

            }
        }

        /* check body length against previous tag size */
        result = flv_read_prev_tag_size(flv_in, &prev_tag_size);
        if (result != FLV_OK) {
            print_fatal("F12024", flv_get_offset(flv_in), "unexpected end of file after tag");
            goto end;
        }

        if (prev_tag_size != FLV_TAG_SIZE + body_length) {
            sprintf(message, "previous tag size should be %d, %d found instead", FLV_TAG_SIZE + body_length, prev_tag_size);
            print_error("E12025", flv_get_offset(flv_in), message);
            goto end;
        }
    }

end:
    report_end(opts, errors, warnings);
    
    flv_close(flv_in);
    
    return (errors > 0) ? ERROR_INVALID_FLV_FILE : OK;
}
