/* https://github.com/noirotm/flvmeta/issues/31 */
#include "fixture.h"

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const unsigned char metadata[] = {8, 0, 0, 0, 0, 0, 0, 9};
    /* AVC sequence header followed by a baseline SPS. pic_order_cnt_type=1,
       one offset_for_ref_frame, encoded with 47 leading zeros and a zero
       suffix. Enough PPS bytes follow to avoid the unrelated video-header
       length accounting bug and reach Exp-Golomb decoding. */
    static const unsigned char video[] = {
        0x17, 0x00, 0x00, 0x00, 0x00, 0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1,
        0x00, 0x15, 0x67, 0x42, 0x00, 0x1E, 0xD3, 0x40, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x28, 0x3F,
        0x20, 0x01, 0x00, 0x08, 0x68, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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
