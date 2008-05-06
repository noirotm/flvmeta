#include <check.h>
#include <stdlib.h>
#include <string.h>
#include "amf.h"

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
    fail_if(data == NULL,
        "data should not be NULL");    
    
    fail_unless(amf_data_get_type(data) == AMF_TYPE_NUMBER,
        "invalid data type: expected %d, got %d", AMF_TYPE_NUMBER, amf_data_get_type(data));
    
    /* AMF number size == 1(header) + 8(data) -> 9 bytes */
    fail_unless(amf_data_size(data) == 9,
        "invalid data size: expected 9, got %d", amf_data_size(data));
    
    fail_unless(amf_number_get_value(data) == 0,
        "invalid data value: expected 3.14159, got %f", amf_number_get_value(data));
}
END_TEST

START_TEST(test_amf_number_set_value) {
    amf_number_set_value(data, -512.78);
    
    fail_unless(amf_number_get_value(data) == -512.78,
        "invalid data value: expected -512.78, got %f", amf_number_get_value(data));
}
END_TEST

START_TEST(test_amf_number_null) {
    fail_unless(amf_number_get_value(NULL) == 0,
        "invalid data value: expected 0, got %f", amf_number_get_value(NULL));
    
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
    fail_if(data == NULL,
        "data should not be NULL");    
    
    fail_unless(amf_data_get_type(data) == AMF_TYPE_BOOLEAN,
        "invalid data type: expected %d, got %d", AMF_TYPE_BOOLEAN, amf_data_get_type(data));
    
    /* AMF boolean size == 1(header) + 1(data) -> 2 bytes */
    fail_unless(amf_data_size(data) == 2,
        "invalid data size: expected 2, got %d", amf_data_size(data));
    
    fail_unless(amf_boolean_get_value(data) == 1,
        "invalid data value: expected 1, got %d", amf_boolean_get_value(data));
}
END_TEST

START_TEST(test_amf_boolean_set_value) {
    amf_boolean_set_value(data, 0);
    fail_unless(amf_boolean_get_value(data) == 0,
        "invalid data value: expected 0, got %d", amf_boolean_get_value(data));
}
END_TEST

START_TEST(test_amf_boolean_null) {
    fail_unless(amf_boolean_get_value(NULL) == 0,
        "invalid data value: expected 0, got %d", amf_boolean_get_value(data));

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
    
    fail_if(data == NULL,
        "data should not be NULL");
    
    fail_unless(amf_data_get_type(data) == AMF_TYPE_STRING,
        "invalid data type: expected %d, got %d", AMF_TYPE_STRING, amf_data_get_type(data));
    
    /* AMF string size == 1(header) + 2(string length) + length */
    fail_unless(amf_data_size(data) == 3 + length,
        "invalid data size: expected %d, got %d", 3 + length, amf_data_size(data));
    
    fail_unless(amf_string_get_size(data) == length,
        "invalid string length: expected %d, got %f", length, amf_string_get_size(data));
    
    fail_unless(strcmp(amf_string_get_bytes(data), str) == 0,
        "invalid string contents");
}
END_TEST

START_TEST(test_amf_str_null) {
    data = amf_str(NULL);
    
    fail_unless(amf_string_get_size(data) == 0,
        "invalid string length: expected 0, got %f", amf_string_get_size(data));
    
    fail_unless(amf_string_get_bytes(data) == NULL,
        "string data should be NULL");
    
    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_new) {
    char * str = "hello world";
    data = amf_string_new(str, 5);
    
    fail_unless(amf_string_get_size(data) == 5,
        "invalid string length: expected 5, got %f", amf_string_get_size(data));
    
    fail_unless(strncmp(amf_string_get_bytes(data), str, 5) == 0,
        "invalid string contents");
    
    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_new_null) {
    data = amf_string_new(NULL, 12);
    
    fail_unless(amf_string_get_size(data) == 0,
        "invalid string length: expected 0, got %f", amf_string_get_size(data));
    
    fail_unless(amf_string_get_bytes(data) == NULL,
        "string data should be NULL");
    
    amf_data_free(data);
}
END_TEST

START_TEST(test_amf_string_null) {
    fail_unless(amf_string_get_size(NULL) == 0,
        "invalid string length: expected 0, got %f", amf_string_get_size(data));

    fail_unless(amf_string_get_bytes(NULL) == NULL,
        "string data should be NULL");
}
END_TEST

/**
    AMF Types Suite
*/
Suite * amf_types_suite(void) {
    Suite * s = suite_create ("AMF types");

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

int main(void) {
    int number_failed;
    SRunner * sr = srunner_create(amf_types_suite());
    /* srunner_set_log (sr, "check_amf.log"); */
    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);
    
    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
