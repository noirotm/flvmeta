/* https://github.com/noirotm/flvmeta/issues/32 */
#include "fixture.h"
#include <string.h>

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const struct {
        const char * name;
        unsigned long audio, video;
    } cases[] = {
        {"positive", 0x80000000UL, 255},
        {"negative", 255, 0x80000000UL},
        {"cross-positive", 0x80000000UL, 0x7FFFFFFFUL},
        {"cross-negative", 0x7FFFFFFFUL, 0x80000000UL},
        {"maximum", 0xFFFFFFFFUL, 0},
        {"minimum", 0, 0xFFFFFFFFUL},
        {"zero", 0xFFFFFFFFUL, 0xFFFFFFFFUL},
        {"ordinary", 2000, 1000}
    };
    /* audiodelay=42 deliberately disagrees with every case, so the checker
       must print the expected value even for a zero or one-millisecond delay. */
    static const unsigned char metadata[] = {
        8, 0, 0, 0, 1, 0, 10, 'a', 'u', 'd', 'i', 'o', 'd', 'e', 'l', 'a', 'y',
        0, 0x40, 0x45, 0, 0, 0, 0, 0, 0, 0, 0, 9
    };
    static const unsigned char audio[] = {0x2F, 0};
    /* Screen video header with a 320 x 240 resolution. */
    static const unsigned char video[] = {0x13, 0x01, 0x40, 0x00, 0xF0};
    size_t i;

    if (argc != 1) {
        return 1;
    }
    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        if (!strcmp(argv[0], cases[i].name)) {
            break;
        }
    }
    if (i == sizeof(cases) / sizeof(cases[0])) {
        return 1;
    }
    if (fseek(file, 0, SEEK_SET) != 0 || !fixture_header(file, 5)
    || !fixture_metadata_header(file, sizeof(metadata))
    || fwrite(metadata, 1, sizeof(metadata), file) != sizeof(metadata)
    || !fixture_metadata_footer(file, sizeof(metadata))) {
        return 1;
    }
    /* Keep file order chronological for both signs of the delay. */
    if (cases[i].audio <= cases[i].video) {
        return !(fixture_tag_at(file, 8, audio, sizeof(audio), cases[i].audio)
            && fixture_tag_at(file, 9, video, sizeof(video), cases[i].video));
    }
    return !(fixture_tag_at(file, 9, video, sizeof(video), cases[i].video)
        && fixture_tag_at(file, 8, audio, sizeof(audio), cases[i].audio));
}
