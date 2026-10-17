/* https://github.com/noirotm/flvmeta/issues/33 */
#include "fixture.h"
#include <string.h>

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const struct {
        const char * name;
        unsigned char milliseconds[8];
    } cases[] = {
        {"nan",      {0x7F, 0xF8, 0, 0, 0, 0, 0, 0}},
        {"negative-nan", {0xFF, 0xF8, 0, 0, 0, 0, 0, 0}},
        {"infinity", {0x7F, 0xF0, 0, 0, 0, 0, 0, 0}},
        {"negative-infinity", {0xFF, 0xF0, 0, 0, 0, 0, 0, 0}},
        {"maximum",  {0x7F, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
        {"minimum",  {0xFF, 0xEF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}},
        /* 1000 * 2^63 milliseconds: the exclusive signed 64-bit limit. */
        {"signed-limit", {0x44, 0x7F, 0x40, 0, 0, 0, 0, 0}},
        {"epoch",    {0, 0, 0, 0, 0, 0, 0, 0}},
        /* 1500 milliseconds: fractional seconds still truncate to one. */
        {"ordinary", {0x40, 0x97, 0x70, 0, 0, 0, 0, 0}}
    };
    unsigned char metadata[] = {
        8,                          /* ECMA array */
        0, 0, 0, 1,                 /* One property */
        0, 4, 'd', 'a', 't', 'e',   /* Property name */
        11,                         /* AMF date */
        0, 0, 0, 0, 0, 0, 0, 0,    /* Milliseconds, big-endian IEEE 754 */
        0, 0,                       /* Timezone */
        0, 0, 9                     /* End of array */
    };
    size_t i;

    if (argc != 1) {
        return 1;
    }
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        if (!strcmp(argv[0], cases[i].name)) {
            memcpy(metadata + 12, cases[i].milliseconds, 8);
            return !(fixture_metadata_header(file, sizeof(metadata))
                && fwrite(metadata, 1, sizeof(metadata), file) == sizeof(metadata)
                && fixture_metadata_footer(file, sizeof(metadata)));
        }
    }
    return 1;
}
