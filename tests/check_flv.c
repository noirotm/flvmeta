/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2016 Marc Noirot <marc.noirot AT gmail.com>

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
#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "src/flv.h"

/**
    FLV types
*/

START_TEST(test_swap_uint16) {
    uint16 ile;
    uint16_be ibe;

    ile = 0x1122U;
    ibe = swap_uint16(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, 0x2211U);
#endif
    ck_assert_int_eq(swap_uint16(ibe), ile);
}
END_TEST

START_TEST(test_swap_uint16_neg) {
    uint16 ile;
    uint16_be ibe;

    ile = 0xFFFEU;
    ibe = swap_uint16(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, 0xFEFFU);
#endif
    ck_assert_int_eq(swap_uint16(ibe), ile);
}
END_TEST

START_TEST(test_swap_sint16) {
    sint16 ile;
    sint16_be ibe;

    ile = 0x1122;
    ibe = swap_sint16(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, 0x2211);
#endif
    ck_assert_int_eq(swap_sint16(ibe), ile);
}
END_TEST

START_TEST(test_swap_sint16_neg) {
    sint16 ile;
    sint16_be ibe;

    ile = 0xFF00;
    ibe = swap_sint16(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, (0x00FF));
#endif
    ck_assert_int_eq(swap_sint16(ibe), ile);
}
END_TEST

START_TEST(test_swap_uint32) {
    uint32 ile;
    uint32_be ibe;

    ile = 0x11223344U;
    ibe = swap_uint32(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, 0x44332211U);
#endif
    ck_assert_int_eq(swap_uint32(ibe), ile);
}
END_TEST

START_TEST(test_swap_uint32_neg) {
    uint32 ile;
    uint32_be ibe;

    ile = 0xFFFEFDFCU;
    ibe = swap_uint32(ile);

#ifndef WORDS_BIGENDIAN
    ck_assert_int_eq(ibe, 0xFCFDFEFFU);
#endif
    ck_assert_int_eq(swap_uint32(ibe), ile);
}
END_TEST

START_TEST(test_swap_number64) {
    number64 ile;
    number64_be ibe;

    ile = 3.14159;
    ibe = swap_number64(ile);

    ck_assert_double_eq(swap_number64(ibe), ile);
}
END_TEST

START_TEST(test_swap_number64_neg) {
    number64 ile;
    number64_be ibe;

    ile = -3.14159;
    ibe = swap_number64(ile);

    ck_assert_double_eq(swap_number64(ibe), ile);
}
END_TEST

START_TEST(test_uint24_be_to_uint32) {
    uint24_be ile;
    uint32 ibe;

    ile.b[2] = 0x33;
    ile.b[1] = 0x22;
    ile.b[0] = 0x11;
    ibe = uint24_be_to_uint32(ile);

    ck_assert_int_eq(ibe, 0x00112233);
}
END_TEST

START_TEST(test_uint32_to_uint24_be) {
    uint32 ile;
    uint24_be ibe;

    ile = 0x00112233;
    ibe = uint32_to_uint24_be(ile);

    ck_assert_int_eq(ibe.b[0], 0x11);
    ck_assert_int_eq(ibe.b[1], 0x22);
    ck_assert_int_eq(ibe.b[2], 0x33);
}
END_TEST

START_TEST(test_uint32_to_uint24_be_truncate) {
    uint32 ile;
    uint24_be ibe;

    ile = 0x11223344;
    ibe = uint32_to_uint24_be(ile);

    ck_assert_int_eq(ibe.b[0], 0x22);
    ck_assert_int_eq(ibe.b[1], 0x33);
    ck_assert_int_eq(ibe.b[2], 0x44);
}
END_TEST

/**
    FLV tags
*/
START_TEST(test_flv_tag_get_timestamp_short) {
    flv_tag tag;
    uint32 val;

    tag.timestamp = uint32_to_uint24_be(0x00332211);
    tag.timestamp_extended = 0x00;
    val = flv_tag_get_timestamp(tag);

    ck_assert_int_eq(val, 0x00332211);
}
END_TEST

START_TEST(test_flv_tag_get_timestamp_extended) {
    flv_tag tag;
    uint32 val;

    tag.timestamp = uint32_to_uint24_be(0x00332211);
    tag.timestamp_extended = 0x44;
    val = flv_tag_get_timestamp(tag);

    ck_assert_int_eq(val, 0x44332211);
}
END_TEST

START_TEST(test_flv_tag_set_timestamp_short) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x00112233);
    val = uint24_be_to_uint32(tag.timestamp);

    ck_assert_int_eq(val, 0x00112233);
    ck_assert_int_eq(tag.timestamp_extended, 0x00);
}
END_TEST

START_TEST(test_flv_tag_set_timestamp_extended) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x44332211);
    val = uint24_be_to_uint32(tag.timestamp);

    ck_assert_int_eq(val, 0x00332211);
    ck_assert_int_eq(tag.timestamp_extended, 0x44);
}
END_TEST


void write_flv_header(FILE *file) {
    flv_header header;
    header.signature[0] = 'F';
    header.signature[1] = 'L';
    header.signature[2] = 'V';
    header.version = 1;
    header.flags = 0;
    header.offset = FLV_HEADER_SIZE;  // Size of the FLV header

    // Write the FLV header to the output file
    fwrite(&header, sizeof(flv_header), 1, file);
    char byte = 0xFF;

    // write prev tag item
    fwrite(&byte, 1, 1, file);
}

void write_flv_video_tag(FILE *file, uint8_t frame_type, byte fourcc[FLV_VIDEO_FOURCC_SIZE]) {

    flv_tag tag;
    flv_video_tag video_tag;

    tag.type = FLV_TAG_TYPE_VIDEO;
    tag.body_length = uint32_to_uint24_be(FLV_TAG_SIZE);
    tag.timestamp = uint32_to_uint24_be(0);
    tag.timestamp_extended = 0;
    tag.stream_id = uint32_to_uint24_be(0);

    fwrite(&tag, sizeof(flv_tag), 1, file);

    // Write the video tag data
    fwrite(&frame_type, sizeof(uint8_t), 1, file);

    // Only write if the extended header bit is set.
    video_tag.video_tag = frame_type;

    if (flv_video_tag_is_ext_header(&video_tag)) {
       fwrite(fourcc, FLV_VIDEO_FOURCC_SIZE, 1, file);
    }
}

FILE *create_temp_file(char *template) {
    int fd = mkstemp(template);

    if(fd == -1) {
        perror("Could not create temporary file");
        exit(1);
    }
    return fdopen(fd, "wb");
}

START_TEST(test_flv_reader_no_extended) {
    flv_header header;
    flv_parser parser;
    flv_tag tag;
    flv_video_tag vt;
    vt.fourcc = 101;     /* set to random value */

    byte FLV_VIDEO_TAG_FOURCC_AV1[] = "av01";
    char template[] = "/tmp/mytempfileXXXXXX";

    FILE *file = create_temp_file(template);
    write_flv_header(file);
    write_flv_video_tag(file, 0x07, FLV_VIDEO_TAG_FOURCC_AV1);
    fclose(file);

    flv_stream* stream = flv_open(template);

    int result = flv_read_header(stream, &header);
    ck_assert_int_eq(result, FLV_OK);
    int retval = flv_read_tag(stream, &tag);
    ck_assert_int_eq(retval, FLV_OK);
    ck_assert_int_eq(tag.type, FLV_TAG_TYPE_VIDEO);

    retval = flv_read_video_tag(stream, &vt);
    ck_assert_int_eq(retval, FLV_OK);

    /* Confirm codec was not set */
    ck_assert_int_eq(vt.fourcc, 101);
}

START_TEST(test_flv_reader_av1) {
    flv_header header;
    flv_parser parser;
    flv_tag tag;
    flv_video_tag vt;
    char template[] = "/tmp/mytempfileXXXXXX";
    byte FLV_VIDEO_TAG_FOURCC_AV1[] = "av01";

    FILE *file = create_temp_file(template);

    write_flv_header(file);
    write_flv_video_tag(file, 0x87, FLV_VIDEO_TAG_FOURCC_AV1);
    fclose(file);

    flv_stream* stream = flv_open(template);

    int result = flv_read_header(stream, &header);
    ck_assert_int_eq(result, FLV_OK);
    int retval = flv_read_tag(stream, &tag);
    ck_assert_int_eq(retval, FLV_OK);
    ck_assert_int_eq(tag.type, FLV_TAG_TYPE_VIDEO);

    retval = flv_read_video_tag(stream, &vt);
    ck_assert_int_eq(vt.fourcc, FLV_VIDEO_FOURCC_AV1);
}

START_TEST(test_flv_reader_hevc) {
    flv_header header;
    flv_parser parser;
    flv_tag tag;
    flv_video_tag vt;

    char template[] = "/tmp/mytempfileXXXXXX";
    byte FLV_VIDEO_TAG_FOURCC_HEVC[] = "hvc1";

    FILE *file = create_temp_file(template);
    write_flv_header(file);
    write_flv_video_tag(file, 0x87, FLV_VIDEO_TAG_FOURCC_HEVC);
    fclose(file);

    flv_stream* stream = flv_open(template);

    int result = flv_read_header(stream, &header);
    ck_assert_int_eq(result, FLV_OK);
    int retval = flv_read_tag(stream, &tag);
    ck_assert_int_eq(retval, FLV_OK);
    ck_assert_int_eq(tag.type, FLV_TAG_TYPE_VIDEO);

    retval = flv_read_video_tag(stream, &vt);
    ck_assert_int_eq(vt.fourcc, FLV_VIDEO_FOURCC_HEVC);
}

/**
    FLV Suite
*/
Suite * flv_suite(void) {
    Suite * s = suite_create("FLV Format");

    /* FLV data types tests */
    TCase * tc_flv_types = tcase_create("FLV types");
    tcase_add_test(tc_flv_types, test_swap_uint16);
    tcase_add_test(tc_flv_types, test_swap_uint16_neg);
    tcase_add_test(tc_flv_types, test_swap_sint16);
    tcase_add_test(tc_flv_types, test_swap_sint16_neg);
    tcase_add_test(tc_flv_types, test_swap_uint32);
    tcase_add_test(tc_flv_types, test_swap_uint32_neg);
    tcase_add_test(tc_flv_types, test_swap_number64);
    tcase_add_test(tc_flv_types, test_swap_number64_neg);
    tcase_add_test(tc_flv_types, test_uint24_be_to_uint32);
    tcase_add_test(tc_flv_types, test_uint32_to_uint24_be);
    tcase_add_test(tc_flv_types, test_uint32_to_uint24_be_truncate);
    suite_add_tcase(s, tc_flv_types);

    /* FLV tag tests */
    TCase * tc_flv_tag = tcase_create("FLV tags");
    tcase_add_test(tc_flv_tag, test_flv_tag_get_timestamp_short);
    tcase_add_test(tc_flv_tag, test_flv_tag_get_timestamp_extended);
    tcase_add_test(tc_flv_tag, test_flv_tag_set_timestamp_short);
    tcase_add_test(tc_flv_tag, test_flv_tag_set_timestamp_extended);
    tcase_add_test(tc_flv_tag, test_flv_reader_no_extended);
    tcase_add_test(tc_flv_tag, test_flv_reader_av1);
    tcase_add_test(tc_flv_tag, test_flv_reader_hevc);
    suite_add_tcase(s, tc_flv_tag);
    return s;
}
