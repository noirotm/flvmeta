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
#include <string.h>
#include "src/amf.h"
#include "util.h"

/* Exercise the reader's default policy without exposing it in the public API. */
#define TEST_DEPTH_LIMIT 128

static amf_data * data = NULL;
static byte * nested_buffer = NULL;

void amf_tests_teardown(void) {
    amf_data_free(data);
    data = NULL;
    free(nested_buffer);
    nested_buffer = NULL;
}

/**
    AMF number
*/
static void test_amf_number_new(void) {
    data = amf_number_new(0);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(AMF_TYPE_NUMBER, amf_data_get_type(data));
    /* AMF number size == 1(header) + 8(data) -> 9 bytes */
    TEST_ASSERT_EQUAL_size_t(9, amf_data_size(data));
    TEST_ASSERT_EQUAL_DOUBLE(0, amf_number_get_value(data));
}

static void test_amf_number_set_value(void) {
    data = amf_number_new(0);

    amf_number_set_value(data, -512.78);
    TEST_ASSERT_EQUAL_DOUBLE(-512.78, amf_number_get_value(data));
}

static void test_amf_number_null(void) {
    TEST_ASSERT_EQUAL_DOUBLE(0, amf_number_get_value(NULL));
    /* just making sure we don't core dump */
    amf_number_set_value(NULL, 12);
}

/**
    AMF boolean
*/
static void test_amf_boolean_new(void) {
    data = amf_boolean_new(1);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(AMF_TYPE_BOOLEAN, amf_data_get_type(data));
    /* AMF boolean size == 1(header) + 1(data) -> 2 bytes */
    TEST_ASSERT_EQUAL_size_t(2, amf_data_size(data));
    TEST_ASSERT_EQUAL_INT(1, amf_boolean_get_value(data));
}

static void test_amf_boolean_set_value(void) {
    data = amf_boolean_new(1);

    amf_boolean_set_value(data, 0);
    TEST_ASSERT_EQUAL_INT(0, amf_boolean_get_value(data));
}

static void test_amf_boolean_null(void) {
    TEST_ASSERT_EQUAL_INT(0, amf_boolean_get_value(NULL));
    /* just making sure we don't core dump */
    amf_boolean_set_value(NULL, 12);
}

/**
    AMF string
*/
static void test_amf_str(void) {
    const char * str = "hello world";
    const size_t length = strlen(str);
    data = amf_str(str);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_INT(AMF_TYPE_STRING, amf_data_get_type(data));
    /* AMF string size == 1(header) + 2(string length) + length */
    TEST_ASSERT_EQUAL_size_t(3 + length, amf_data_size(data));
    TEST_ASSERT_EQUAL_UINT16(length, amf_string_get_size(data));
    TEST_ASSERT_EQUAL_STRING(str, amf_string_get_bytes(data));
}

static void test_amf_str_null(void) {
    data = amf_str(NULL);

    TEST_ASSERT_EQUAL_UINT16(0, amf_string_get_size(data));
    TEST_ASSERT_EQUAL_STRING("", amf_string_get_bytes(data));
}

static void test_amf_string_new(void) {
    byte str[] = "hello world";
    data = amf_string_new(str, 5);

    TEST_ASSERT_EQUAL_UINT16(5, amf_string_get_size(data));
    TEST_ASSERT_EQUAL_STRING("hello", amf_string_get_bytes(data));
}

static void test_amf_string_new_null(void) {
    data = amf_string_new(NULL, 12);

    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQUAL_UINT16(0, amf_string_get_size(data));
    TEST_ASSERT_EQUAL_STRING("", amf_string_get_bytes(data));
}

static void test_amf_string_null(void) {
    TEST_ASSERT_EQUAL_UINT16(0, amf_string_get_size(NULL));
    TEST_ASSERT_NULL(amf_string_get_bytes(NULL));
}

/* Check one container kind and leaf shape below, at, and above the limit. */
static void check_amf_depth_boundaries(unsigned int kind, int empty_leaf) {
    unsigned int depth;
    size_t size;
    amf_data * copy;

    for (depth = TEST_DEPTH_LIMIT - 1; depth <= TEST_DEPTH_LIMIT + 1; ++depth) {
        nested_buffer = nested_amf(depth, kind, 0, empty_leaf, &size);
        TEST_ASSERT_NOT_NULL(nested_buffer);
        data = amf_data_buffer_read(nested_buffer, size);
        TEST_ASSERT_NOT_NULL(data);
        TEST_ASSERT_EQUAL_INT(depth <= TEST_DEPTH_LIMIT ? AMF_ERROR_OK :
            AMF_ERROR_DEPTH_LIMIT, amf_data_get_error_code(data));
        if (depth <= TEST_DEPTH_LIMIT) {
            /* Accepted trees must also be safe for the recursive
               operations used after parsing, not just the reader. */
            copy = amf_data_clone(data);
            TEST_ASSERT_NOT_NULL(copy);
            TEST_ASSERT_EQUAL_size_t(size, amf_data_size(copy));
            amf_data_free(copy);
            TEST_ASSERT_EQUAL_size_t(size,
                amf_data_buffer_write(data, nested_buffer, size));
        }
        /* Free each tree and reset state before the next independent
           read. A depth counter must not accumulate between reads. */
        amf_tests_teardown();
    }
}

/* Each named test checks scalar leaves and empty innermost containers.
   Neither leaf shape should cause an off-by-one rejection at the limit. */
static void test_amf_strict_array_depth_boundaries(void) {
    check_amf_depth_boundaries(0, 0);
    check_amf_depth_boundaries(0, 1);
}

static void test_amf_object_depth_boundaries(void) {
    check_amf_depth_boundaries(1, 0);
    check_amf_depth_boundaries(1, 1);
}

static void test_amf_ecma_array_depth_boundaries(void) {
    check_amf_depth_boundaries(2, 0);
    check_amf_depth_boundaries(2, 1);
}

static void test_amf_mixed_depth_boundaries(void) {
    check_amf_depth_boundaries(3, 0);
    check_amf_depth_boundaries(3, 1);
}

static void test_amf_depth_empty_ecma_name(void) {
    size_t size;
    /* The empty-name termination shortcut must not turn a child depth error
       into a successfully parsed partial ECMA array. */
    nested_buffer = nested_amf(TEST_DEPTH_LIMIT + 1, 2, 1, 0, &size);
    TEST_ASSERT_NOT_NULL(nested_buffer);
    data = amf_data_buffer_read(nested_buffer, size);
    TEST_ASSERT_EQUAL_INT(AMF_ERROR_DEPTH_LIMIT, amf_data_get_error_code(data));
}

static void test_amf_depth_truncated(void) {
    size_t size;
    /* Remove the final boolean's value byte at an otherwise allowed depth.
       This must remain EOF, rather than being confused with the depth limit. */
    nested_buffer = nested_amf(TEST_DEPTH_LIMIT, 0, 0, 0, &size);
    TEST_ASSERT_NOT_NULL(nested_buffer);
    data = amf_data_buffer_read(nested_buffer, size - 1);
    TEST_ASSERT_EQUAL_INT(AMF_ERROR_EOF, amf_data_get_error_code(data));
}

static void test_amf_depth_wide_array(void) {
    byte buffer[5 + TEST_DEPTH_LIMIT + 1];
    /* One strict array with more elements than the nesting limit: breadth
       must not consume depth. Its five-byte header precedes null elements. */
    memset(buffer, AMF_TYPE_NULL, sizeof(buffer));
    buffer[0] = AMF_TYPE_ARRAY;
    buffer[1] = buffer[2] = buffer[3] = 0;
    buffer[4] = TEST_DEPTH_LIMIT + 1;
    data = amf_data_buffer_read(buffer, sizeof(buffer));
    TEST_ASSERT_EQUAL_INT(AMF_ERROR_OK, amf_data_get_error_code(data));
    TEST_ASSERT_EQUAL_UINT32(TEST_DEPTH_LIMIT + 1, amf_array_size(data));
}

void run_amf_tests(void) {
    UnitySetTestFile(__FILE__);

    RUN_TEST(test_amf_number_new);
    RUN_TEST(test_amf_number_set_value);
    RUN_TEST(test_amf_number_null);
    RUN_TEST(test_amf_boolean_new);
    RUN_TEST(test_amf_boolean_set_value);
    RUN_TEST(test_amf_boolean_null);
    RUN_TEST(test_amf_str);
    RUN_TEST(test_amf_str_null);
    RUN_TEST(test_amf_string_new);
    RUN_TEST(test_amf_string_new_null);
    RUN_TEST(test_amf_string_null);
    RUN_TEST(test_amf_strict_array_depth_boundaries);
    RUN_TEST(test_amf_object_depth_boundaries);
    RUN_TEST(test_amf_ecma_array_depth_boundaries);
    RUN_TEST(test_amf_mixed_depth_boundaries);
    RUN_TEST(test_amf_depth_empty_ecma_name);
    RUN_TEST(test_amf_depth_truncated);
    RUN_TEST(test_amf_depth_wide_array);
}
