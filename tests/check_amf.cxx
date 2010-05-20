#include "MiniCppUnit.hxx"
#include "amf.h"

/**
    AMF number
*/
class TestAmfNumber: public TestFixture<TestAmfNumber> {
public:
    TEST_FIXTURE(TestAmfNumber) {
        TEST_CASE(test_amf_number_new);
        TEST_CASE(test_amf_number_set_value);
        TEST_CASE(test_amf_number_null);
    }
    
    void setUp() {
        data = amf_number_new(0);
    }
    
    void tearDown() {
        amf_data_free(data);
    }

    void test_amf_number_new() {
        ASSERT(data != NULL);
        ASSERT_EQUALS(amf_data_get_type(data), AMF_TYPE_NUMBER);
        /* AMF number size == 1(header) + 8(data) -> 9 bytes */
        ASSERT_EQUALS((int)amf_data_size(data), 9);
        ASSERT_EQUALS(amf_number_get_value(data), 0);
    }

    void test_amf_number_set_value() {
        amf_number_set_value(data, -512.78);
        ASSERT_EQUALS(amf_number_get_value(data), -512.78);
    }

    void test_amf_number_null() {
        ASSERT_EQUALS(amf_number_get_value(NULL), 0);
        /* just making sure we don't core dump */
        amf_number_set_value(NULL, 12);
    }
private:
    amf_data * data;
};
REGISTER_FIXTURE(TestAmfNumber);

/**
    AMF Boolean
*/
class TestAmfBoolean: public TestFixture<TestAmfBoolean> {
public:
    TEST_FIXTURE(TestAmfBoolean) {
        
    }
private:
    amf_data * data;
};
REGISTER_FIXTURE(TestAmfBoolean);
