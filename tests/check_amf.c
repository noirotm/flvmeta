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

static amf_data * data = NULL;

void amf_tests_teardown(void) {
    amf_data_free(data);
    data = NULL;
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
}
