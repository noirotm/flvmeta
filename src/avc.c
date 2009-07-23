/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008, 2009 Marc Noirot <marc.noirot AT gmail.com>

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
#include <stdlib.h>

#include "avc.h"

/**
    bit buffer handling
*/
typedef struct __bit_buffer {
    byte * start;
    size_t size;
    byte * current;
    uint8 read_bits;
} bit_buffer;

void skip_bits(bit_buffer * bb, size_t nbits) {
    bb->current = bb->current + ((nbits + bb->read_bits) / 8);
    bb->read_bits = (uint8)((bb->read_bits + nbits) % 8);
}

uint8 get_bit(bit_buffer * bb) {
    uint8 ret;
    ret = (*(bb->current) >> (7 - bb->read_bits)) & 0x1;
    if (bb->read_bits == 7) {
        bb->read_bits = 0;
        bb->current++;
    }
    else {
        bb->read_bits++;
    }
    return ret;
}

uint32 get_bits(bit_buffer * bb, size_t nbits) {
    uint32 i, ret;
    ret = 0;
    for (i = 0; i < nbits; i++) {
        ret = (ret << 1) + get_bit(bb);
    }
    return ret;
}

uint32 exp_golomb_ue(bit_buffer * bb) {
    uint8 bit, significant_bits;
    significant_bits = 0;
    bit = get_bit(bb);
    while (bit == 0) {
        significant_bits++;
        bit = get_bit(bb);
    }
    return (1 << significant_bits) + get_bits(bb, significant_bits) - 1;
}

sint32 exp_golomb_se(bit_buffer * bb) {
    sint32 ret;
    ret = exp_golomb_ue(bb);
    if ((ret & 0x1) == 0) {
        return -(ret >> 1);
    }
    else {
        return (ret + 1) >> 1;
    }
}

/* AVC type definitions */
#pragma pack(push, 1)

#define AVC_SEQUENCE_HEADER 0
#define AVC_NALU            1
#define AVC_END_OF_SEQUENCE 2

typedef struct __AVCDecoderConfigurationRecord {
    uint8 configurationVersion;
    uint8 AVCProfileIndication;
    uint8 profile_compatibility;
    uint8 AVCLevelIndication;
    uint8 lengthSizeMinusOne;
    uint8 numOfSequenceParameterSets;
} AVCDecoderConfigurationRecord;

#pragma pack(pop)

static void parse_scaling_list(uint32 size, bit_buffer * bb) {
    uint32 last_scale, next_scale, i;
    sint32 delta_scale;
    last_scale = 8;
    next_scale = 8;
    for (i = 0; i < size; i++) {
        if (next_scale != 0) {
            delta_scale = exp_golomb_se(bb);
            next_scale = (last_scale + delta_scale + 256) % 256;
        }
        if (next_scale != 0) {
            last_scale = next_scale;
        }
    }
}

/**
    Parses a SPS NALU to retrieve video width and height
*/
static void parse_sps(byte * sps, size_t sps_size, uint32 * width, uint32 * height) {
    bit_buffer bb;
    uint32 profile, pic_order_cnt_type, height_map;
    uint32 i, size;

    bb.start = sps;
    bb.size = sps_size;
    bb.current = sps;
    bb.read_bits = 0;

    /* skip first byte, since we already know we're parsing a SPS */
    skip_bits(&bb, 8);
    /* get profile */
    profile = get_bits(&bb, 8);
    /* skip 4 bits + 4 zeroed bits + 8 bits = 32 bits = 4 bytes */
    skip_bits(&bb, 16);

    /* read sps id, first exp-golomb encoded value */
    exp_golomb_ue(&bb);

    if (profile == 100 || profile == 110 || profile == 122 || profile == 144) {
        /* chroma format idx */
        if (exp_golomb_ue(&bb) == 3) {
            skip_bits(&bb, 1);
        }
        /* bit depth luma minus8 */
        exp_golomb_ue(&bb);
        /* bit depth chroma minus8 */
        exp_golomb_ue(&bb);
        /* Qpprime Y Zero Transform Bypass flag */
        skip_bits(&bb, 1);
        /* Seq Scaling Matrix Present Flag */
        if (get_bit(&bb)) {
            for (i = 0; i < 8; i++) {
                /* Seq Scaling List Present Flag */
                if (get_bit(&bb)) {
                    parse_scaling_list(i < 6 ? 16 : 64, &bb);
                }
            }
        }
    }
    /* log2_max_frame_num_minus4 */
    exp_golomb_ue(&bb);
    /* pic_order_cnt_type */
    pic_order_cnt_type = exp_golomb_ue(&bb);
    if (pic_order_cnt_type == 0) {
        /* log2_max_pic_order_cnt_lsb_minus4 */
        exp_golomb_ue(&bb);
    }
    else if (pic_order_cnt_type == 1) {
        /* delta_pic_order_always_zero_flag */
        skip_bits(&bb, 1);
        /* offset_for_non_ref_pic */
        exp_golomb_se(&bb);
        /* offset_for_top_to_bottom_field */
        exp_golomb_se(&bb);
        size = exp_golomb_ue(&bb);
        for (i = 0; i < size; i++) {
            /* offset_for_ref_frame */
            exp_golomb_se(&bb);
        }
    }
    /* num_ref_frames */
    exp_golomb_ue(&bb);
    /* gaps_in_frame_num_value_allowed_flag */
    skip_bits(&bb, 1);
    /* width */
    *width = (exp_golomb_ue(&bb) + 1) * 16;
    /* height */
    height_map = exp_golomb_ue(&bb) + 1;
    *height = (2 - get_bit(&bb)) * height_map * 16;
}

/**
    Tries to read the resolution of the current video packet.
    We assume to be at the first byte of the video data.
*/
size_t read_avc_resolution(FILE * f, uint32 body_length, uint32 * width, uint32 * height) {
    size_t bytes_read;
    byte avc_packet_type;
    uint24 composition_time;
    AVCDecoderConfigurationRecord adcr;
    uint16 sps_size;
    byte * sps_buffer;

    /* make sure we have enough bytes to read in the current tag */
    if (body_length < sizeof(byte) + sizeof(uint24) + sizeof(AVCDecoderConfigurationRecord)) {
        return 0;
    }

    /* determine whether we're reading an AVCDecoderConfigurationRecord */
    bytes_read = fread(&avc_packet_type, 1, 1, f);
    if (bytes_read == 0 || avc_packet_type != AVC_SEQUENCE_HEADER) {
        return bytes_read;
    }

    /* read the composition time */
    if (fread(&composition_time, sizeof(uint24), 1, f) == 0) {
        return bytes_read;
    }
    bytes_read += sizeof(uint24);

    /* we need to read an AVCDecoderConfigurationRecord */
    if (fread(&adcr, sizeof(AVCDecoderConfigurationRecord), 1, f) == 0) {
        return bytes_read;
    }
    bytes_read += sizeof(AVCDecoderConfigurationRecord);

    /* number of SequenceParameterSets */
    if ((adcr.numOfSequenceParameterSets & 0x1F) == 0) {
        /* no SPS, return */
        return bytes_read;
    }

    /** read the first SequenceParameterSet found */
    /* SPS size */
    if (fread(&sps_size, sizeof(uint16), 1, f) == 0) {
        return bytes_read;
    }
    bytes_read += sizeof(uint16);
    sps_size = swap_uint16(sps_size);
    
    /* read the SPS entirely */
    sps_buffer = (byte *) malloc((size_t)sps_size);
    if (sps_buffer == NULL) {
        return bytes_read;
    }
    if (fread(sps_buffer, (size_t)sps_size, 1, f) == 0) {
        free(sps_buffer);
        return bytes_read;
    }
    bytes_read += (size_t)sps_size;

    /* parse SPS to determine video resolution */
    parse_sps(sps_buffer, (size_t)sps_size, width, height);
    
    free(sps_buffer);
    return bytes_read;
}
