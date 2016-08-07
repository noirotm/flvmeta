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
    fail_if(ibe != 0x2211U);
#endif
    fail_if(swap_uint16(ibe) != ile);
}
END_TEST

START_TEST(test_swap_uint16_neg) {
    uint16 ile;
    uint16_be ibe;
    
    ile = 0xFFFEU;
    ibe = swap_uint16(ile);
#ifndef WORDS_BIGENDIAN
    fail_if(ibe != 0xFEFFU);
#endif
    fail_if(swap_uint16(ibe) != ile);
}
END_TEST

START_TEST(test_swap_sint16) {
    sint16 ile;
    sint16_be ibe;
    
    ile = 0x1122;
    ibe = swap_sint16(ile);
#ifndef WORDS_BIGENDIAN
    fail_if(ibe != 0x2211);
#endif
    fail_if(swap_sint16(ibe) != ile);
}
END_TEST

START_TEST(test_swap_sint16_neg) {
    sint16 ile;
    sint16_be ibe;
    
    ile = 0xFF00;
    ibe = swap_sint16(ile);
#ifndef WORDS_BIGENDIAN
    fail_if(ibe != (0x00FF));
#endif
    fail_if(swap_sint16(ibe) != ile);
}
END_TEST

START_TEST(test_swap_uint32) {
    uint32 ile;
    uint32_be ibe;
    
    ile = 0x11223344U;
    ibe = swap_uint32(ile);
#ifndef WORDS_BIGENDIAN
    fail_if(ibe != 0x44332211U);
#endif
    fail_if(swap_uint32(ibe) != ile);
}
END_TEST

START_TEST(test_swap_uint32_neg) {
    uint32 ile;
    uint32_be ibe;
    
    ile = 0xFFFEFDFCU;
    ibe = swap_uint32(ile);
#ifndef WORDS_BIGENDIAN
    fail_if(ibe != 0xFCFDFEFFU);
#endif
    fail_if(swap_uint32(ibe) != ile);
}
END_TEST

START_TEST(test_swap_number64) {
    number64 ile;
    number64_be ibe;
    
    ile = 3.14159;
    ibe = swap_number64(ile);
    fail_if(swap_number64(ibe) != ile);
}
END_TEST

START_TEST(test_swap_number64_neg) {
    number64 ile;
    number64_be ibe;
    
    ile = -3.14159;
    ibe = swap_number64(ile);
    fail_if(swap_number64(ibe) != ile);
}
END_TEST

START_TEST(test_uint24_be_to_uint32) {
    uint24_be ile;
    uint32 ibe;
    
    ile.b[2] = 0x33;
    ile.b[1] = 0x22;
    ile.b[0] = 0x11;
    ibe = uint24_be_to_uint32(ile);
    fail_if(ibe != 0x00112233);
}
END_TEST

START_TEST(test_uint32_to_uint24_be) {
    uint32 ile;
    uint24_be ibe;
    
    ile = 0x00112233;
    ibe = uint32_to_uint24_be(ile);

    fail_if(ibe.b[0] != 0x11);
    fail_if(ibe.b[1] != 0x22);
    fail_if(ibe.b[2] != 0x33);
}
END_TEST

START_TEST(test_uint32_to_uint24_be_truncate) {
    uint32 ile;
    uint24_be ibe;
    
    ile = 0x11223344;
    ibe = uint32_to_uint24_be(ile);

    fail_if(ibe.b[0] != 0x22);
    fail_if(ibe.b[1] != 0x33);
    fail_if(ibe.b[2] != 0x44);
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
    fail_if(val != 0x00332211, "expected 0x00332211, got 0x%X", val);
}
END_TEST

START_TEST(test_flv_tag_get_timestamp_extended) {
    flv_tag tag;
    uint32 val;
    
    tag.timestamp = uint32_to_uint24_be(0x00332211);
    tag.timestamp_extended = 0x44;
    val = flv_tag_get_timestamp(tag);
    fail_if(val != 0x44332211, "expected 0x%X, got 0x%X", 0x44332211, val);
}
END_TEST

START_TEST(test_flv_tag_set_timestamp_short) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x00112233);
    val = uint24_be_to_uint32(tag.timestamp);
    fail_if(val != 0x00112233,
        "expected 0x00112233, got 0x%X", val);
    fail_if(tag.timestamp_extended != 0x00,
        "expected 0x00, got 0x%hhX", tag.timestamp_extended);
}
END_TEST

START_TEST(test_flv_tag_set_timestamp_extended) {
    flv_tag tag;
    uint32 val;

    flv_tag_set_timestamp(&tag, 0x44332211);
    val = uint24_be_to_uint32(tag.timestamp);
    fail_if(val != 0x00332211,
        "expected 0x00332211, got 0x%X", val);
    fail_if(tag.timestamp_extended != 0x44,
        "expected 0x44, got 0x%hhX", tag.timestamp_extended);
}
END_TEST

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
    suite_add_tcase(s, tc_flv_tag);
    return s;
}
