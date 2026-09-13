/* Issue #29: nested metadata followed by a recognizable recovery tag. */
#include <stdlib.h>
#include "src/amf.h"
#include "util.h"
#include "fixture.h"

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    byte * buffer;
    size_t size;
    size_t value_size;
    unsigned int depth, kind;
    int success;
    /* Wrap the generated value in an ECMA array {nested: value}. */
    static const byte prefix[] = {8,0,0,0,1,0,6,'n','e','s','t','e','d'};
    static const byte suffix[] = {0,0,9};
    /* Second tag: ECMA array {recovery: true}. */
    static const byte tail[] = {8,0,0,0,1,0,8,'r','e','c','o','v','e','r','y',1,1,0,0,9};

    if (argc != 2) return 1;
    depth = (unsigned int)strtoul(argv[0], NULL, 10);
    kind = (unsigned int)strtoul(argv[1], NULL, 10);
    if (depth == 0 || depth > 70000 || kind > 3) return 1;
    /* Reserve one level for the enclosing ECMA array. */
    buffer = nested_amf(depth - 1, kind, 0, 0, &size);
    if (buffer == NULL) return 1;
    value_size = sizeof(prefix) + size + sizeof(suffix);
    success = fixture_metadata_header(file, value_size) &&
        fwrite(prefix, 1, sizeof(prefix), file) == sizeof(prefix) &&
        fwrite(buffer, 1, size, file) == size &&
        fwrite(suffix, 1, sizeof(suffix), file) == sizeof(suffix) &&
        fixture_metadata_footer(file, value_size) &&
        fixture_metadata_header(file, sizeof(tail)) &&
        fwrite(tail, 1, sizeof(tail), file) == sizeof(tail) &&
        fixture_metadata_footer(file, sizeof(tail));
    free(buffer);
    return success ? 0 : 1;
}
