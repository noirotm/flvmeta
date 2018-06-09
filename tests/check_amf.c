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
#include "src/amf.h"

amf_data * data;

void teardown(void) {
    amf_data_free(data);
}

/**
    AMF number
*/
void setup_amf_number(void) {
    data = amf_number_new(0);
}

START_TEST(test_amf_number_new) {
    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(amf_data_get_type(data), AMF_TYPE_NUMBER);
    /* AMF number size == 1(header) + 8(data) -> 9 bytes */
    ck_assert_int_eq(amf_data_size(data), 9);
    ck_assert_int_eq(amf_number_get_value(data), 0);
}
END_TEST

START_TEST(test_amf_number_set_value) {
    amf_number_set_value(data, -512.78);
    ck_assert_double_eq(amf_number_get_value(data), -512.78);
}
END_TEST

START_TEST(test_amf_number_null) {
    ck_assert_int_eq(amf_number_get_value(NULL), 0);
    /* just making sure we don't core dump */
    amf_number_set_value(NULL, 12);
}
END_TEST

/**
    AMF boolean
*/
void setup_amf_boolean(void) {
    data = amf_boolean_new(1);
}

START_TEST(test_amf_boolean_new) {
    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(amf_data_get_type(data), AMF_TYPE_BOOLEAN);
    /* AMF boolean size == 1(header) + 1(data) -> 2 bytes */
    ck_assert_int_eq(amf_data_size(data), 2);
    ck_assert_int_eq(amf_boolean_get_value(data), 1);
}
END_TEST

START_TEST(test_amf_boolean_set_value) {
    amf_boolean_set_value(data, 0);
    ck_assert_int_eq(amf_boolean_get_value(data), 0);
}
END_TEST

START_TEST(test_amf_boolean_null) {
    ck_assert_int_eq(amf_boolean_get_value(NULL), 0);
    /* just making sure we don't core dump */
    amf_boolean_set_value(NULL, 12);
}
END_TEST

/**
    AMF string
*/
START_TEST(test_amf_str) {
    char * str = "hello world";
    int length = strlen(str);
    data = amf_str(str);

    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(amf_data_get_type(data), AMF_TYPE_STRING);
    /* AMF string size == 1(header) + 2(string length) + length */
    ck_assert_int_eq(amf_data_size(data), 3 + length);
    ck_assert_int_eq(amf_string_get_size(data), length);
    ck_assert_str_eq(amf_string_get_bytes(data), str);
}
END_TEST

START_TEST(test_amf_str_null) {
    data = amf_str(NULL);

    ck_assert_int_eq(amf_string_get_size(data), 0);
    ck_assert_str_eq(amf_string_get_bytes(data), "");

    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_new) {
    char * str = "hello world";
    data = amf_string_new(str, 5);

    ck_assert_int_eq(amf_string_get_size(data), 5);
    ck_assert_str_eq(amf_string_get_bytes(data), "hello");

    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_new_null) {
    data = amf_string_new(NULL, 12);

    ck_assert_ptr_nonnull(data);
    ck_assert_int_eq(amf_string_get_size(data), 0);
    ck_assert_str_eq(amf_string_get_bytes(data), "");

    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_null) {
    ck_assert_int_eq(amf_string_get_size(NULL), 0);
    ck_assert_ptr_null(amf_string_get_bytes(NULL));
}
END_TEST

/**
    AMF Types Suite
*/
Suite * amf_types_suite(void) {
    Suite * s = suite_create("AMF types");

    /* AMF number test case */
    TCase * tc_number = tcase_create("AMF number");
    tcase_add_checked_fixture(tc_number, setup_amf_number, teardown);
    tcase_add_test(tc_number, test_amf_number_new);
    tcase_add_test(tc_number, test_amf_number_set_value);
    tcase_add_test(tc_number, test_amf_number_null);
    suite_add_tcase(s, tc_number);

    /* AMF boolean test case */
    TCase * tc_boolean = tcase_create("AMF boolean");
    tcase_add_checked_fixture(tc_boolean, setup_amf_boolean, teardown);
    tcase_add_test(tc_boolean, test_amf_boolean_new);
    tcase_add_test(tc_boolean, test_amf_boolean_set_value);
    tcase_add_test(tc_boolean, test_amf_boolean_null);
    suite_add_tcase(s, tc_boolean);

    /* AMF string test case */
    TCase * tc_string = tcase_create("AMF string");
    tcase_add_test(tc_string, test_amf_str);
    tcase_add_test(tc_string, test_amf_str_null);
    tcase_add_test(tc_string, test_amf_string_new);
    tcase_add_test(tc_string, test_amf_string_new_null);
    tcase_add_test(tc_string, test_amf_string_null);
    suite_add_tcase(s, tc_string);

    return s;
}
