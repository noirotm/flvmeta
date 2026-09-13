/* Issue #30: an empty AMF string instead of an associative-array root. */
#include "fixture.h"

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    static const unsigned char value[] = {2, 0, 0};
    (void)argv;
    if (argc != 0) return 1;
    return fixture_metadata_header(file, sizeof(value)) &&
        fwrite(value, 1, sizeof(value), file) == sizeof(value) &&
        fixture_metadata_footer(file, sizeof(value)) ? 0 : 1;
}
