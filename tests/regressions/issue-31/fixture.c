/* https://github.com/noirotm/flvmeta/issues/31 */
#include "fixture.h"

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const unsigned char metadata[] = {
        8,              /* ECMA array */
        0, 0, 0, 0,     /* No properties */
        0, 0, 9         /* End of array */
    };
    /* The SPS contains an offset_for_ref_frame value with 47 leading zero
       bits. The old decoder shifted a 32-bit integer by 47 when reading it.
       The PPS padding lets this reproduce even before the video-length fix. */
    static const unsigned char video[] = {
        0x17,                           /* AVC keyframe */
        0x00, 0x00, 0x00, 0x00,         /* Sequence header, composition time zero */
        0x01, 0x42, 0x00, 0x1E, 0xFF,   /* AVC configuration */
        0xE1, 0x00, 0x15,               /* One SPS, 21 bytes long */
        /* SPS: baseline profile, pic_order_cnt_type=1, one reference offset. */
        0x67, 0x42, 0x00, 0x1E, 0xD3, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x28, 0x3F, 0x20,
        0x01, 0x00, 0x08,               /* One PPS, eight bytes long */
        0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };
    if (fseek(file, 0, SEEK_SET) != 0 || !fixture_header(file, 1)
    || !fixture_metadata_header(file, sizeof(metadata))
    || fwrite(metadata, 1, sizeof(metadata), file) != sizeof(metadata)
    || !fixture_metadata_footer(file, sizeof(metadata))
    || !fixture_tag(file, 9, video, sizeof(video))) {
        return 1;
    }
    return 0;
}
