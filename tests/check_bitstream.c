#include "unity.h"
#include "src/bitstream.h"
#include <string.h>

static bit_buffer buffer(byte * data, size_t size) {
    bit_buffer bb;
    bb.start = bb.current = data;
    bb.size = size;
    bb.read_bits = 0;
    return bb;
}

/* Encode a test value: write value + 1 in binary, preceded by one fewer zero
   than its number of bits. For example, 5 becomes 110, then 00110.
   Store the bits from left to right in data and return the bytes used. */
static size_t encode_ue(byte * data, uint32 value) {
    uint64 code;
    unsigned int width, i, position;
    code = (uint64)value + 1;
    width = 0;
    for (i = 0; i < 33; i++) {
        if (code >> i) {
            width = i + 1;
        }
    }
    memset(data, 0, 9);
    position = width - 1;
    for (i = width; i > 0; i--) {
        if ((code >> (i - 1)) & 1) {
            data[position / 8] |= (byte)(1U << (7 - position % 8));
        }
        position++;
    }
    return (position + 7) / 8;
}

static void test_unsigned_boundaries(void) {
    static const uint32 values[] = {0, 1, 2, 3, 0x7FFFFFFFUL, 0x80000000UL,
        0xFFFFFFFEUL, 0xFFFFFFFFUL};
    byte data[9];
    bit_buffer bb;
    size_t i, size;
    for (i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        size = encode_ue(data, values[i]);
        bb = buffer(data, size);
        TEST_ASSERT_EQUAL_UINT32(values[i], exp_golomb_ue(&bb));
    }
}

static void test_signed_boundaries(void) {
    static const uint32 codes[] = {0, 1, 2, 0x7FFFFFFFUL, 0xFFFFFFFDUL, 0xFFFFFFFEUL};
    static const sint32 values[] = {0, 1, -1, 1073741824, 2147483647, -2147483647};
    byte data[9];
    bit_buffer bb;
    size_t i, size;
    for (i = 0; i < sizeof(codes) / sizeof(codes[0]); i++) {
        size = encode_ue(data, codes[i]);
        bb = buffer(data, size);
        TEST_ASSERT_EQUAL_INT32(values[i], exp_golomb_se(&bb));
    }
    size = encode_ue(data, 0xFFFFFFFFUL);
    bb = buffer(data, size);
    TEST_ASSERT_EQUAL_INT32(0, exp_golomb_se(&bb));
}

static void test_invalid_codes(void) {
    byte data[40];
    bit_buffer bb;
    size_t size;
    memset(data, 0, sizeof(data));
    /* Reported 47-zero prefix, and a prefix long enough to wrap the old counter. */
    data[5] = 1;
    bb = buffer(data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT32(0, exp_golomb_ue(&bb));
    memset(data, 0, sizeof(data));
    data[39] = 1;
    bb = buffer(data, sizeof(data));
    TEST_ASSERT_EQUAL_UINT32(0, exp_golomb_ue(&bb));
    TEST_ASSERT_TRUE((size_t)(bb.current - bb.start) <= 5);
    /* A representable-width code with an out-of-range suffix. */
    size = encode_ue(data, 0xFFFFFFFFUL);
    data[8] = 0x80;
    bb = buffer(data, size);
    TEST_ASSERT_EQUAL_UINT32(0, exp_golomb_ue(&bb));
    /* Truncated suffix and prefix. */
    data[0] = 1;
    bb = buffer(data, 1);
    TEST_ASSERT_EQUAL_UINT32(0, exp_golomb_ue(&bb));
    data[0] = 0;
    bb = buffer(data, 1);
    TEST_ASSERT_EQUAL_UINT32(0, exp_golomb_ue(&bb));
}

void run_bitstream_tests(void) {
    RUN_TEST(test_unsigned_boundaries);
    RUN_TEST(test_signed_boundaries);
    RUN_TEST(test_invalid_codes);
}
