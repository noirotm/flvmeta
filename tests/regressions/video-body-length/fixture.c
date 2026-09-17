#include "fixture.h"
#include <string.h>

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const unsigned char metadata[] = {
        8,              /* ECMA array */
        0, 0, 0, 0,     /* No properties */
        0, 0, 9         /* End of array */
    };
    static const unsigned char video[] = {
        0x17,                       /* AVC keyframe: one-byte FLV video header */
        0x00, 0x00, 0x00, 0x00,     /* Sequence header, composition time zero */
        0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, /* One baseline SPS */
        0x00, 0x09,                 /* Nine-byte SPS describing 320 x 240 */
        0x67, 0x42, 0x00, 0x1E, 0xD3, 0x54, 0x0A, 0x0F, 0xC8,
        0x00                        /* No PPS in this configuration record */
    };
    size_t video_size;

    video_size = sizeof(video);
    if (argc == 1 && !strcmp(argv[0], "header-only")) {
        video_size = 1;
    }
    if (fseek(file, 0, SEEK_SET) != 0 || !fixture_header(file, 1)
    || !fixture_metadata_header(file, sizeof(metadata))
    || fwrite(metadata, 1, sizeof(metadata), file) != sizeof(metadata)
    || !fixture_metadata_footer(file, sizeof(metadata))
    || !fixture_tag(file, 9, video, video_size)) {
        return 1;
    }
    return 0;
}
