/*
    $Id: update.c 231 2011-06-27 13:46:19Z marc.noirot $

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
#include "flvmeta.h"
#include "flv.h"
#include "amf.h"
#include "dump.h"
#include "info.h"
#include "update.h"
#include "util.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#define COPY_BUFFER_SIZE 4096

#ifdef WIN32
# include "win32_tmpfile.h"
# define flvmeta_tmpfile win32_tmpfile
#else /* WIN32 */
# define flvmeta_tmpfile tmpfile
#endif /* WIN32 */

/*
    Write the flv output file
*/
static int write_flv(flv_stream * flv_in, FILE * flv_out, const flv_info * info, const flv_metadata * meta, const flvmeta_opts * opts) {
    uint32_be size;
    uint32 on_metadata_name_size;
    uint32 on_metadata_size;
    uint32 prev_timestamp_video;
    uint32 prev_timestamp_audio;
    uint32 prev_timestamp_meta;
    uint8 timestamp_extended_video;
    uint8 timestamp_extended_audio;
    uint8 timestamp_extended_meta;
    byte * copy_buffer;
    flv_tag ft, omft;
    int have_on_last_second;

    if (opts->verbose) {
        fprintf(stdout, "Writing %s...\n", opts->output_file);
    }

    /* write the flv header */
    if (flv_write_header(flv_out, &info->header) != 1) {
        return ERROR_WRITE;
    }

    /* first "previous tag size" */
    size = swap_uint32(0);
    if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
        return ERROR_WRITE;
    }

    /* create the onMetaData tag */
    on_metadata_name_size = (uint32)amf_data_size(meta->on_metadata_name);
    on_metadata_size = (uint32)amf_data_size(meta->on_metadata);

    omft.type = FLV_TAG_TYPE_META;
    omft.body_length = uint32_to_uint24_be(on_metadata_name_size + on_metadata_size);
    flv_tag_set_timestamp(&omft, 0);
    omft.stream_id = uint32_to_uint24_be(0);
    
    /* write the computed onMetaData tag first if it doesn't exist in the input file */
    if (info->on_metadata_size == 0) {
        if (flv_write_tag(flv_out, &omft) != 1
        || amf_data_file_write(meta->on_metadata_name, flv_out) < on_metadata_name_size
        || amf_data_file_write(meta->on_metadata, flv_out) < on_metadata_size) {
            return ERROR_WRITE;
        }

        /* previous tag size */
        size = swap_uint32(FLV_TAG_SIZE + on_metadata_name_size + on_metadata_size);
        if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
            return ERROR_WRITE;
        }
    }

    /* extended timestamp initialization */
    prev_timestamp_video = 0;
    prev_timestamp_audio = 0;
    prev_timestamp_meta = 0;
    timestamp_extended_video = 0;
    timestamp_extended_audio = 0;
    timestamp_extended_meta = 0;

    /* copy the tags verbatim */
    flv_reset(flv_in);

    copy_buffer = (byte *)malloc(info->biggest_tag_body_size + FLV_TAG_SIZE);
    have_on_last_second = 0;
    while (flv_read_tag(flv_in, &ft) == FLV_OK) {
        file_offset_t offset;
        size_t read_body;
        uint32 body_length;
        uint32 timestamp;

        offset = flv_get_current_tag_offset(flv_in);
        body_length = flv_tag_get_body_length(ft);
        timestamp = flv_tag_get_timestamp(ft);

        /* extended timestamp fixing */
        if (ft.type == FLV_TAG_TYPE_META) {
            if (timestamp < prev_timestamp_meta
            && prev_timestamp_meta - timestamp > 0xF00000) {
                ++timestamp_extended_meta;
            }
            prev_timestamp_meta = timestamp;
            if (timestamp_extended_meta > 0) {
                timestamp += timestamp_extended_meta << 24;
            }
        }
        else if (ft.type == FLV_TAG_TYPE_AUDIO) {
            if (timestamp < prev_timestamp_audio
            && prev_timestamp_audio - timestamp > 0xF00000) {
                ++timestamp_extended_audio;
            }
            prev_timestamp_audio = timestamp;
            if (timestamp_extended_audio > 0) {
                timestamp += timestamp_extended_audio << 24;
            }
        }
        else if (ft.type == FLV_TAG_TYPE_VIDEO) {
            if (timestamp < prev_timestamp_video
            && prev_timestamp_video - timestamp > 0xF00000) {
                ++timestamp_extended_video;
            }
            prev_timestamp_video = timestamp;
            if (timestamp_extended_video > 0) {
                timestamp += timestamp_extended_video << 24;
            }
        }

        /* non-zero starting timestamp handling */
        if (opts->reset_timestamps && timestamp > 0) {
            timestamp -= info->first_timestamp;
        }

        flv_tag_set_timestamp(&ft, timestamp);

        /* if we're at the offset of the first onMetaData tag in the input file,
           we write the one we computed instead, discarding the old one */
        if (info->on_metadata_offset == offset) {
            if (flv_write_tag(flv_out, &omft) != 1
            || amf_data_file_write(meta->on_metadata_name, flv_out) < on_metadata_name_size
            || amf_data_file_write(meta->on_metadata, flv_out) < on_metadata_size) {
                free(copy_buffer);
                return ERROR_WRITE;
            }

            /* previous tag size */
            size = swap_uint32(FLV_TAG_SIZE + on_metadata_name_size + on_metadata_size);
            if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
                free(copy_buffer);
                return ERROR_WRITE;
            }
        }
        else {
            /* insert an onLastSecond metadata tag */
            if (opts->insert_onlastsecond && !have_on_last_second && !info->have_on_last_second && (info->last_timestamp - timestamp) <= 1000) {
                flv_tag tag;
                uint32 on_last_second_name_size = (uint32)amf_data_size(meta->on_last_second_name);
                uint32 on_last_second_size = (uint32)amf_data_size(meta->on_last_second);
                tag.type = FLV_TAG_TYPE_META;
                tag.body_length = uint32_to_uint24_be(on_last_second_name_size + on_last_second_size);
                tag.timestamp = ft.timestamp;
                tag.timestamp_extended = ft.timestamp_extended;
                tag.stream_id = uint32_to_uint24_be(0);
                if (flv_write_tag(flv_out, &tag) != 1
                || amf_data_file_write(meta->on_last_second_name, flv_out) < on_last_second_name_size
                || amf_data_file_write(meta->on_last_second, flv_out) < on_last_second_size) {
                    free(copy_buffer);
                    return ERROR_WRITE;
                }

                /* previous tag size */
                size = swap_uint32(FLV_TAG_SIZE + on_last_second_name_size + on_last_second_size);
                if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
                    free(copy_buffer);
                    return ERROR_WRITE;
                }

                have_on_last_second = 1;
            }

            /* if the tag is bigger than expected, it means that
               it's an unknown tag type. In this case, we only
               copy as much data as the copy buffer can contain */
            if (body_length > info->biggest_tag_body_size) {
                body_length = info->biggest_tag_body_size;
            }

            /* copy the tag verbatim */
            read_body = flv_read_tag_body(flv_in, copy_buffer, body_length);
            if (read_body < body_length) {
                /* we have reached end of file on an incomplete tag */
                if (opts->error_handling == FLVMETA_EXIT_ON_ERROR) {
                    free(copy_buffer);
                    return ERROR_EOF;
                }
                else if (opts->error_handling == FLVMETA_FIX_ERRORS) {
                    /* the tag is bogus, just omit it,
                       even though it will make the whole file length
                       calculation wrong, and the metadata inaccurate */
                    /* TODO : fix it by handling that problem in the first pass */
                    free(copy_buffer);
                    return OK;
                }
                else if (opts->error_handling == FLVMETA_IGNORE_ERRORS) {
                    /* just copy the whole tag and exit */
                    flv_write_tag(flv_out, &ft);
                    fwrite(copy_buffer, 1, read_body, flv_out);
                    free(copy_buffer);
                    size = swap_uint32(FLV_TAG_SIZE + read_body);
                    fwrite(&size, sizeof(uint32_be), 1, flv_out);
                    return OK;
                }
            }
            if (flv_write_tag(flv_out, &ft) != 1
            || fwrite(copy_buffer, 1, body_length, flv_out) < body_length) {
                free(copy_buffer);
                return ERROR_WRITE;
            }

            /* previous tag length */
            size = swap_uint32(FLV_TAG_SIZE + body_length);
            if (fwrite(&size, sizeof(uint32_be), 1, flv_out) != 1) {
                free(copy_buffer);
                return ERROR_WRITE;
            }
        }        
    }

    if (opts->verbose) {
        fprintf(stdout, "%s successfully written\n", opts->output_file);
    }

    free(copy_buffer);
    return OK;
}

/* copy a FLV file while adding onMetaData and optionnally onLastSecond events */
int update_metadata(const flvmeta_opts * opts) {
    int res, in_place_update;
    flv_stream * flv_in;
    FILE * flv_out;
    flv_info info;
    flv_metadata meta;

    flv_in = flv_open(opts->input_file);
    if (flv_in == NULL) {
        return ERROR_OPEN_READ;
    }

    /*
        get all necessary information from the flv file
    */
    res = get_flv_info(flv_in, &info, opts);
    if (res != OK) {
        flv_close(flv_in);
        amf_data_free(info.keyframes);
        return res;
    }

    compute_metadata(&info, &meta, opts);

    /*
        open output file
    */
    /* detect whether we have to overwrite the input file */
    if (same_file(opts->input_file, opts->output_file)) {
        in_place_update = 1;
        flv_out = flvmeta_tmpfile();
    }
    else {
        in_place_update = 0;
        flv_out = fopen(opts->output_file, "wb");
    }

    if (flv_out == NULL) {
        flv_close(flv_in);
        amf_data_free(meta.on_last_second_name);
        amf_data_free(meta.on_last_second);
        amf_data_free(meta.on_metadata_name);
        amf_data_free(meta.on_metadata);
        amf_data_free(info.original_on_metadata);
        return ERROR_OPEN_WRITE;
    }

    /*
        write the output file
    */
    res = write_flv(flv_in, flv_out, &info, &meta, opts);

    flv_close(flv_in);
    amf_data_free(meta.on_last_second_name);
    amf_data_free(meta.on_last_second);
    amf_data_free(meta.on_metadata_name);
    amf_data_free(info.original_on_metadata);

    /* copy data into the original file if needed */
    if (in_place_update == 1) {
        FILE * flv_out_real;
        size_t bytes_read;
        byte copy_buffer[COPY_BUFFER_SIZE];

        flv_out_real = fopen(opts->output_file, "wb");
        if (flv_out_real == NULL) {
            amf_data_free(meta.on_metadata);
            return ERROR_OPEN_WRITE;
        }

        /* copy temporary file contents into the final file */
        lfs_fseek(flv_out, 0, SEEK_SET);
        while (!feof(flv_out)) {
            bytes_read = fread(copy_buffer, sizeof(byte), COPY_BUFFER_SIZE, flv_out);
            if (bytes_read > 0) {
                if (fwrite(copy_buffer, sizeof(byte), bytes_read, flv_out_real) < bytes_read) {
                    fclose(flv_out_real);
                    fclose(flv_out);
                    amf_data_free(meta.on_metadata);
                    return ERROR_WRITE;
                }
            }
            else {
                fclose(flv_out_real);
                fclose(flv_out);
                amf_data_free(meta.on_metadata);
                return ERROR_WRITE;
            }
        }
        
        fclose(flv_out_real);
    }
    fclose(flv_out);

    /* dump computed metadata if we have to */
    if (opts->dump_metadata == 1) {
        dump_amf_data(meta.on_metadata, opts);
    }
    
    amf_data_free(meta.on_metadata);
    return res;
}
