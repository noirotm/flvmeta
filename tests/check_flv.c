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
#include "unity.h"
#include <stdio.h>
#include <stdlib.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif
#include "src/flv.h"

#ifndef FLVMETA_TEST_TMP_DIR
#define FLVMETA_TEST_TMP_DIR "."
#endif

#define FLVMETA_TEST_PATH_SIZE 1024

static unsigned long get_test_process_id(void) {
#if defined(_WIN32)
    return (unsigned long)_getpid();
#else
    return (unsigned long)getpid();
#endif
}

static void make_temp_path(char * path, size_t path_size, const char * filename) {
    int written = snprintf(
        path,
        path_size,
        "%s/flvmeta-test-%lu-%s",
        FLVMETA_TEST_TMP_DIR,
        get_test_process_id(),
        filename
    );

    TEST_ASSERT_TRUE(written > 0);
    TEST_ASSERT_TRUE((size_t)written < path_size);
}

static FILE * create_temp_file(const char * filename, char * path, size_t path_size) {
    FILE * file;

    make_temp_path(path, path_size, filename);
    file = fopen(path, "wb");
    TEST_ASSERT_NOT_NULL(file);

    return file;
}

/**
    FLV types
*/

static void test_swap_uint16(void) {
    uint16 ile;
    uint16_be ibe;

    ile = 0x1122U;
    ibe = swap_uint16(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_UINT16(0x2211U, ibe);
#endif
    TEST_ASSERT_EQUAL_UINT16(ile, swap_uint16(ibe));
}

static void test_swap_uint16_neg(void) {
    uint16 ile;
    uint16_be ibe;

    ile = 0xFFFEU;
    ibe = swap_uint16(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_UINT16(0xFEFFU, ibe);
#endif
    TEST_ASSERT_EQUAL_UINT16(ile, swap_uint16(ibe));
}

static void test_swap_sint16(void) {
    sint16 ile;
    sint16_be ibe;

    ile = 0x1122;
    ibe = swap_sint16(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_INT16(0x2211, ibe);
#endif
    TEST_ASSERT_EQUAL_INT16(ile, swap_sint16(ibe));
}

static void test_swap_sint16_neg(void) {
    sint16 ile;
    sint16_be ibe;

    ile = (sint16)0xFF00;
    ibe = swap_sint16(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_INT16((sint16)0x00FF, ibe);
#endif
    TEST_ASSERT_EQUAL_INT16(ile, swap_sint16(ibe));
}

static void test_swap_uint32(void) {
    uint32 ile;
    uint32_be ibe;

    ile = 0x11223344U;
    ibe = swap_uint32(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_UINT32(0x44332211U, ibe);
#endif
    TEST_ASSERT_EQUAL_UINT32(ile, swap_uint32(ibe));
}

static void test_swap_uint32_neg(void) {
    uint32 ile;
    uint32_be ibe;

    ile = 0xFFFEFDFCU;
    ibe = swap_uint32(ile);

#ifndef WORDS_BIGENDIAN
    TEST_ASSERT_EQUAL_UINT32(0xFCFDFEFFU, ibe);
#endif
    TEST_ASSERT_EQUAL_UINT32(ile, swap_uint32(ibe));
}

static void test_swap_number64(void) {
    number64 ile;
    number64_be ibe;

    ile = 3.14159;
    ibe = swap_number64(ile);

    TEST_ASSERT_EQUAL_DOUBLE(ile, swap_number64(ibe));
}

static void test_swap_number64_neg(void) {
    number64 ile;
    number64_be ibe;

    ile = -3.14159;
    ibe = swap_number64(ile);

    TEST_ASSERT_EQUAL_DOUBLE(ile, swap_number64(ibe));
}

static void test_uint24_be_to_uint32(void) {
    uint24_be ile;
    uint32 ibe;

    ile.b[2] = 0x33;
    ile.b[1] = 0x22;
    ile.b[0] = 0x11;
    ibe = uint24_be_to_uint32(ile);

    TEST_ASSERT_EQUAL_UINT32(0x00112233U, ibe);
}

static void test_uint32_to_uint24_be(void) {
    uint32 ile;
    uint24_be ibe;

    ile = 0x00112233U;
    ibe = uint32_to_uint24_be(ile);

    TEST_ASSERT_EQUAL_UINT8(0x11, ibe.b[0]);
    TEST_ASSERT_EQUAL_UINT8(0x22, ibe.b[1]);
    TEST_ASSERT_EQUAL_UINT8(0x33, ibe.b[2]);
}

static void test_uint32_to_uint24_be_truncate(void) {
    uint32 ile;
    uint24_be ibe;

    ile = 0x11223344U;
    ibe = uint32_to_uint24_be(ile);

    TEST_ASSERT_EQUAL_UINT8(0x22, ibe.b[0]);
    TEST_ASSERT_EQUAL_UINT8(0x33, ibe.b[1]);
    TEST_ASSERT_EQUAL_UINT8(0x44, ibe.b[2]);
}

/**
    FLV tags
*/
static void test_flv_tag_get_timestamp_short(void) {
    flv_tag tag;
    uint32 val;

    tag.timestamp = uint32_to_uint24_be(0x00332211U);
    tag.timestamp_extended = 0x00;
    val = flv_tag_get_timestamp(tag);

    TEST_ASSERT_EQUAL_UINT32(0x00332211U, val);
}

static void test_flv_tag_get_timestamp_extended(void) {
    flv_tag tag;
    uint32 val;

    tag.timestamp = uint32_to_uint24_be(0x00332211U);
    tag.timestamp_extended = 0x44;
    val = flv_tag_get_timestamp(tag);

    TEST_ASSERT_EQUAL_UINT32(0x44332211U, val);
}

static void test_flv_tag_set_timestamp_short(void) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x00112233U);
    val = uint24_be_to_uint32(tag.timestamp);

    TEST_ASSERT_EQUAL_UINT32(0x00112233U, val);
    TEST_ASSERT_EQUAL_UINT8(0x00, tag.timestamp_extended);
}

static void test_flv_tag_set_timestamp_extended(void) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x44332211U);
    val = uint24_be_to_uint32(tag.timestamp);

    TEST_ASSERT_EQUAL_UINT32(0x00332211U, val);
    TEST_ASSERT_EQUAL_UINT8(0x44, tag.timestamp_extended);
}

static void write_flv_header(FILE * file) {
    flv_header header;
    uint32_be previous_tag_size;

    header.signature[0] = 'F';
    header.signature[1] = 'L';
    header.signature[2] = 'V';
    header.version = 1;
    header.flags = 0;
    header.offset = swap_uint32(FLV_HEADER_SIZE);

    TEST_ASSERT_EQUAL_size_t(1, flv_write_header(file, &header));

    previous_tag_size = swap_uint32(0);
    TEST_ASSERT_EQUAL_size_t(1, fwrite(&previous_tag_size, sizeof(previous_tag_size), 1, file));
}

static void write_flv_video_tag(FILE * file, uint8_t frame_type, byte fourcc[FLV_VIDEO_FOURCC_SIZE]) {
    flv_tag tag;
    flv_video_tag video_tag;
    uint32 body_length;

    video_tag.video_tag = frame_type;
    body_length = sizeof(frame_type);
    if (flv_video_tag_is_ext_header(&video_tag)) {
        body_length += FLV_VIDEO_FOURCC_SIZE;
    }

    tag.type = FLV_TAG_TYPE_VIDEO;
    tag.body_length = uint32_to_uint24_be(body_length);
    tag.timestamp = uint32_to_uint24_be(0);
    tag.timestamp_extended = 0;
    tag.stream_id = uint32_to_uint24_be(0);

    TEST_ASSERT_EQUAL_size_t(1, flv_write_tag(file, &tag));
    TEST_ASSERT_EQUAL_size_t(1, fwrite(&frame_type, sizeof(frame_type), 1, file));

    if (flv_video_tag_is_ext_header(&video_tag)) {
       TEST_ASSERT_EQUAL_size_t(1, fwrite(fourcc, FLV_VIDEO_FOURCC_SIZE, 1, file));
    }
}

static void test_flv_reader_no_extended(void) {
    flv_header header;
    flv_tag tag;
    flv_video_tag vt;
    flv_stream * stream;
    FILE * file;
    int result;
    int retval;
    char path[FLVMETA_TEST_PATH_SIZE];
    byte fourcc_av1[FLV_VIDEO_FOURCC_SIZE] = {'a', 'v', '0', '1'};

    vt.fourcc = 101;     /* set to random value */

    file = create_temp_file("flvmeta_no_extended.flv", path, sizeof(path));
    write_flv_header(file);
    write_flv_video_tag(file, 0x07, fourcc_av1);
    TEST_ASSERT_EQUAL_INT(0, fclose(file));

    stream = flv_open(path);
    TEST_ASSERT_NOT_NULL(stream);

    result = flv_read_header(stream, &header);
    TEST_ASSERT_EQUAL_INT(FLV_OK, result);
    retval = flv_read_tag(stream, &tag);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);
    TEST_ASSERT_EQUAL_UINT8(FLV_TAG_TYPE_VIDEO, tag.type);

    retval = flv_read_video_tag(stream, &vt);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);

    /* Confirm codec was not set */
    TEST_ASSERT_EQUAL_UINT32(101, vt.fourcc);

    flv_close(stream);
    TEST_ASSERT_EQUAL_INT(0, remove(path));
}

static void test_flv_reader_av1(void) {
    flv_header header;
    flv_tag tag;
    flv_video_tag vt;
    flv_stream * stream;
    FILE * file;
    int result;
    int retval;
    char path[FLVMETA_TEST_PATH_SIZE];
    byte fourcc_av1[FLV_VIDEO_FOURCC_SIZE] = {'a', 'v', '0', '1'};

    file = create_temp_file("flvmeta_av1.flv", path, sizeof(path));
    write_flv_header(file);
    write_flv_video_tag(file, 0x87, fourcc_av1);
    TEST_ASSERT_EQUAL_INT(0, fclose(file));

    stream = flv_open(path);
    TEST_ASSERT_NOT_NULL(stream);

    result = flv_read_header(stream, &header);
    TEST_ASSERT_EQUAL_INT(FLV_OK, result);
    retval = flv_read_tag(stream, &tag);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);
    TEST_ASSERT_EQUAL_UINT8(FLV_TAG_TYPE_VIDEO, tag.type);

    retval = flv_read_video_tag(stream, &vt);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);
    TEST_ASSERT_EQUAL_UINT32(FLV_VIDEO_FOURCC_AV1, vt.fourcc);

    flv_close(stream);
    TEST_ASSERT_EQUAL_INT(0, remove(path));
}

static void test_flv_reader_hevc(void) {
    flv_header header;
    flv_tag tag;
    flv_video_tag vt;
    flv_stream * stream;
    FILE * file;
    int result;
    int retval;
    char path[FLVMETA_TEST_PATH_SIZE];
    byte fourcc_hevc[FLV_VIDEO_FOURCC_SIZE] = {'h', 'v', 'c', '1'};

    file = create_temp_file("flvmeta_hevc.flv", path, sizeof(path));
    write_flv_header(file);
    write_flv_video_tag(file, 0x87, fourcc_hevc);
    TEST_ASSERT_EQUAL_INT(0, fclose(file));

    stream = flv_open(path);
    TEST_ASSERT_NOT_NULL(stream);

    result = flv_read_header(stream, &header);
    TEST_ASSERT_EQUAL_INT(FLV_OK, result);
    retval = flv_read_tag(stream, &tag);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);
    TEST_ASSERT_EQUAL_UINT8(FLV_TAG_TYPE_VIDEO, tag.type);

    retval = flv_read_video_tag(stream, &vt);
    TEST_ASSERT_EQUAL_INT(FLV_OK, retval);
    TEST_ASSERT_EQUAL_UINT32(FLV_VIDEO_FOURCC_HEVC, vt.fourcc);

    flv_close(stream);
    TEST_ASSERT_EQUAL_INT(0, remove(path));
}

void run_flv_tests(void) {
    UnitySetTestFile(__FILE__);

    RUN_TEST(test_swap_uint16);
    RUN_TEST(test_swap_uint16_neg);
    RUN_TEST(test_swap_sint16);
    RUN_TEST(test_swap_sint16_neg);
    RUN_TEST(test_swap_uint32);
    RUN_TEST(test_swap_uint32_neg);
    RUN_TEST(test_swap_number64);
    RUN_TEST(test_swap_number64_neg);
    RUN_TEST(test_uint24_be_to_uint32);
    RUN_TEST(test_uint32_to_uint24_be);
    RUN_TEST(test_uint32_to_uint24_be_truncate);
    RUN_TEST(test_flv_tag_get_timestamp_short);
    RUN_TEST(test_flv_tag_get_timestamp_extended);
    RUN_TEST(test_flv_tag_set_timestamp_short);
    RUN_TEST(test_flv_tag_set_timestamp_extended);
    RUN_TEST(test_flv_reader_no_extended);
    RUN_TEST(test_flv_reader_av1);
    RUN_TEST(test_flv_reader_hevc);
}
