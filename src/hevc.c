/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2023 Marc Noirot <marc.noirot AT gmail.com>

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

#include "hevc.h"
#include "bitstream.h"

#define HEVC_NAL_SPS 33
#define AV_INPUT_BUFFER_PADDING_SIZE 64

typedef struct H2645NAL {
    const uint8_t *data;
    int size;

    /**
     * Size, in bits, of just the data, excluding the stop bit and any trailing
     * padding. I.e. what HEVC calls SODB.
     */
    int size_bits;

    int raw_size;
    const uint8_t *raw_data;

    bit_buffer gb;
} H2645NAL;

// Simplified version based on https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/h2645_parse.c */

int ff_h2645_extract_rbsp(const uint8_t *src, int length,
                          uint8_t *rbsp, H2645NAL *nal, int small_padding)
{
    int i, si, di;
    uint8_t *dst;

/* Emulation prevention byte 0x000003 check based on H.264/H.265 spec */

/* FFMPEG uses a lot of optimizations depending on architectures, for now we use
the slowest */

#define STARTCODE_TEST                                                  \
        if (i + 2 < length && src[i + 1] == 0 && src[i + 2] <= 3) {     \
            if (src[i + 2] != 3 && src[i + 2] != 0) {                   \
                /* startcode, so we must be past the end */             \
                length = i;                                             \
            }                                                           \
            break;                                                      \
        }
    for (i = 0; i + 1 < length; i += 2) {
        if (src[i])
            continue;
        if (i > 0 && src[i - 1] == 0)
            i--;
        STARTCODE_TEST;
    }

    if (i >= length - 1 && small_padding) { // no escaped 0
        nal->data     =
        nal->raw_data = src;
        nal->size     =
        nal->raw_size = length;
        return length;
    } else if (i > length)
        i = length;

    dst = rbsp;

    memcpy(dst, src, i);
    si = di = i;
    while (si + 2 < length) {
        // remove escapes (very rare 1:2^22)
        if (src[si + 2] > 3) {
            dst[di++] = src[si++];
            dst[di++] = src[si++];
        } else if (src[si] == 0 && src[si + 1] == 0 && src[si + 2] != 0) {
            if (src[si + 2] == 3) { // escape
                dst[di++] = 0;
                dst[di++] = 0;
                si       += 3;
                continue;
            } else // next start code
                goto nsc;
        }

        dst[di++] = src[si++];
    }
    while (si < length)
        dst[di++] = src[si++];

nsc:
    memset(dst + di, 0, AV_INPUT_BUFFER_PADDING_SIZE);

    nal->data = dst;
    nal->size = di;
    nal->raw_data = src;
    nal->raw_size = si;

    return si;
}

/* Based on https://github.com/FFmpeg/FFmpeg/blob/42982b5a5d461530a792e69b3e8abdd9d6d67052/libavcodec/hevc_ps.c#L243*/

static int decode_profile_tier_level(bit_buffer *bb) {
    uint32_t profile_space;
    uint32_t tier_flag;
    uint32_t profile_idc;
    uint32_t profile_compatibility_flag[32];
    int i;

    get_bits(bb, 2, &profile_space);
    get_bits(bb, 1, &tier_flag);
    get_bits(bb, 5, &profile_idc);

    for (i = 0; i < 32; i++) {
        get_bits(bb, 1, &profile_compatibility_flag[i]);
        if (profile_idc == 0 && i > 0 && profile_compatibility_flag[i])
            profile_idc = i;
    }

    skip_bits(bb, 4);  // flags

    #define check_profile_idc(idc) \
        profile_idc == idc || profile_compatibility_flag[idc]

    if (check_profile_idc(4) || check_profile_idc(5) || check_profile_idc(6) ||
        check_profile_idc(7) || check_profile_idc(8) || check_profile_idc(9) ||
        check_profile_idc(10)) {

        skip_bits(bb, 9);
        // ptl->max_12bit_constraint_flag        = get_bits1(gb);
        // ptl->max_10bit_constraint_flag        = get_bits1(gb);
        // ptl->max_8bit_constraint_flag         = get_bits1(gb);
        // ptl->max_422chroma_constraint_flag    = get_bits1(gb);
        // ptl->max_420chroma_constraint_flag    = get_bits1(gb);
        // ptl->max_monochrome_constraint_flag   = get_bits1(gb);
        // ptl->intra_constraint_flag            = get_bits1(gb);
        // ptl->one_picture_only_constraint_flag = get_bits1(gb);
        // ptl->lower_bit_rate_constraint_flag   = get_bits1(gb);

        if (check_profile_idc(5) || check_profile_idc(9) || check_profile_idc(10)) {
            //ptl->max_14bit_constraint_flag    = get_bits1(gb);
            skip_bits(bb, 1);
            skip_bits(bb, 33); // XXX_reserved_zero_33bits[0..32]
        } else {
            skip_bits(bb, 34);// XXX_reserved_zero_34bits[0..33]
        }
    } else if (check_profile_idc(2)) {
        skip_bits(bb, 7);
        skip_bits(bb, 1); // one_picture_only_constraint_flag
        skip_bits(bb, 35); // XXX_reserved_zero_35bits[0..34]
    } else {
        skip_bits(bb, 43); // XXX_reserved_zero_43bits[0..42]
    }

    if (check_profile_idc(1) || check_profile_idc(2) || check_profile_idc(3) ||
        check_profile_idc(4) || check_profile_idc(5) || check_profile_idc(9))
        skip_bits(bb, 1);
    else
        skip_bits(bb, 1);
#undef check_profile_idc
}

/* Based on https://github.com/FFmpeg/FFmpeg/blob/42982b5a5d461530a792e69b3e8abdd9d6d67052/libavcodec/hevc_ps.c#L319 */
static int parse_ptl(bit_buffer *bb, int max_num_sub_layers) {
    uint8_t sub_layer_profile_present_flag[HEVC_MAX_SUB_LAYERS];
    uint8_t sub_layer_level_present_flag[HEVC_MAX_SUB_LAYERS];
    int i;

    decode_profile_tier_level(bb);
    skip_bits(bb, 8); //general_ptl.level_idc

    for (i = 0; i < max_num_sub_layers - 1; i++) {
        get_bits(bb, 1, &sub_layer_profile_present_flag[i]);
        get_bits(bb, 1, &sub_layer_level_present_flag[i]);
    }

    if (max_num_sub_layers - 1 > 0) {
        for (i = max_num_sub_layers - 1; i < 8; i++)
            skip_bits(bb, 2); // reserved_zero_2bits[i]
    }

    for (i = 0; i < max_num_sub_layers - 1; i++) {
        if (sub_layer_profile_present_flag[i]) {
            decode_profile_tier_level(bb);
        }

        if (sub_layer_level_present_flag[i])
            skip_bits(bb, 8);
    }
}

static void hvcc_parse_sps(uint8_t * sps, size_t sps_size, uint32 * width, uint32 * height) {
    bit_buffer bb;
    uint8_t temporalIdNested;
    uint8_t chromaFormat;
    uint16_t pic_width_in_luma_samples;
    uint16_t pic_length_in_luma_samples;

    bb.start = sps;
    bb.size = sps_size;
    bb.current = sps;
    bb.read_bits = 0;

    uint32_t sps_max_sub_layers;

    /* SPS header (2 bytes) */
    skip_bits(&bb, 16);

    /* Parse SPS PTL */
    skip_bits(&bb, 4); // vps_id

    get_bits(&bb, 3, &sps_max_sub_layers);
    sps_max_sub_layers += 1;

    skip_bits(&bb, 1); // temporal_id_nesting_flag
    parse_ptl(&bb, sps_max_sub_layers);
    exp_golomb_ue(&bb); // sps_id

    if (exp_golomb_ue(&bb) == 3) { // chromaFormat
        skip_bits(&bb, 1);
    }

    /* Approximately 124 bits using HEVC main proifle*/
    *width = exp_golomb_ue(&bb);
    *height = exp_golomb_ue(&bb);
}

typedef struct __HEVCDecoderConfigurationRecord {
    byte data[23];
} HEVCDecoderConfigurationRecord;

int read_hevc_resolution(flv_video_tag * vt, flv_stream * f, uint32 body_length, uint32 * width, uint32 * height) {
    int packet_type;
    int num_of_arrays;
    int i, j, nal_count;
    uint8 nal_type, nal_unit_type;
    uint16_be num_nalus, nal_unit_length;
    byte *buf;

    HEVCDecoderConfigurationRecord hdcr;

    if(!flv_video_tag_is_ext_header(vt)) {
        return FLV_ERROR_EOF;
    }

    packet_type = flv_video_tag_ext_packet_type(vt);

    /* Based on Extended RTMP spec */
    if (packet_type != FLV_VIDEO_TAG_PACKET_TYPE_SEQUENCE_START) {
      return FLV_OK;
    }

    /* make sure we have enough bytes to read in the current tag */
    if (body_length < sizeof(byte) + sizeof(uint24) + sizeof(hdcr)) {
        return FLV_OK;
    }

    /* Read over the HEVCDecoderConfigurationRecord */
    if (flv_read_tag_body(f, &hdcr, sizeof(hdcr)) < sizeof(hdcr)) {
        return FLV_ERROR_EOF;
    }

    /* Parse the VPS, SPS, NALUs that are part of the hvcc atom */
    num_of_arrays = hdcr.data[22];

    for (i = 0; i < num_of_arrays; i++) {

        if (flv_read_tag_body(f, &nal_unit_type, 1) < 1) {
            return FLV_ERROR_EOF;
        }
        if (flv_read_tag_body(f, &num_nalus, 2) < 2) {
            return FLV_ERROR_EOF;
        }

        nal_type = nal_unit_type & 0x3F;
        nal_count = swap_uint16(num_nalus);

        // https://github.com/FFmpeg/FFmpeg/blob/master/libavcodec/hevc_parse.c#L80
        for (j = 0; j < nal_count; j++) {

            // Read the unit length.
            if (flv_read_tag_body(f, &nal_unit_length, 2) < 2) {
                return FLV_ERROR_EOF;
            }
            nal_unit_length = swap_uint16(nal_unit_length);

            // Read the data next.
            buf = malloc(nal_unit_length);

            if (flv_read_tag_body(f, buf, nal_unit_length) < nal_unit_length) {
                return FLV_ERROR_EOF;
            }

            /* Use the SPS to determine video resolution. But first, we must get rid of the
            start emulation byte 0x00003. */

            if (nal_type == HEVC_NAL_SPS) {
                H2645NAL h2645nal;
                uint8_t *rbsp_buffer;
                rbsp_buffer = malloc(nal_unit_length);

                int consumed = ff_h2645_extract_rbsp(buf, nal_unit_length, rbsp_buffer, &h2645nal, 1);

                hvcc_parse_sps(h2645nal.data, nal_unit_length, width, height);
                free(rbsp_buffer);
                free(buf);
                return FLV_OK;
            }
            free(buf);
        }
    }
    return FLV_OK;
}

