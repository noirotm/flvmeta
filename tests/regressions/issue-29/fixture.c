/* Issue #29: nested metadata followed by a recognizable recovery tag. */
#include <stdlib.h>
#include <string.h>
#include "src/amf.h"
#include "util.h"
#include "fixture.h"

int write_flv_fixture(FILE * file, int argc, char ** argv) {
    byte * buffer;
    size_t size;
    size_t value_size;
    unsigned int depth, kind;
    /* Names map to the container types accepted by nested_amf(). */
    static const char * kinds[] = {"strict-array", "object", "ecma-array", "mixed"};
    int success;
    /* Wrap the generated value in an ECMA array {nested: value}. */
    static const byte prefix[] = {
        8,                              /* ECMA array */
        0, 0, 0, 1,                     /* One property */
        0, 6, 'n', 'e', 's', 't', 'e', 'd' /* Property name */
    };
    static const byte suffix[] = {0, 0, 9}; /* End of array */
    /* Second tag: ECMA array {recovery: true}. */
    static const byte recovery_tag[] = {
        8,                              /* ECMA array */
        0, 0, 0, 1,                     /* One property */
        0, 8, 'r', 'e', 'c', 'o', 'v', 'e', 'r', 'y',
        1, 1,                           /* Boolean: true */
        0, 0, 9                         /* End of array */
    };

    if (argc != 2) return 1;
    depth = (unsigned int)strtoul(argv[0], NULL, 10);
    for (kind = 0; kind < sizeof(kinds) / sizeof(kinds[0]); kind++) {
        if (!strcmp(argv[1], kinds[kind])) {
            break;
        }
    }
    if (depth == 0 || depth > 70000 || kind == sizeof(kinds) / sizeof(kinds[0])) return 1;
    /* Reserve one level for the enclosing ECMA array. */
    buffer = nested_amf(depth - 1, kind, 0, 0, &size);
    if (buffer == NULL) return 1;
    value_size = sizeof(prefix) + size + sizeof(suffix);
    success = fixture_metadata_header(file, value_size) &&
        fwrite(prefix, 1, sizeof(prefix), file) == sizeof(prefix) &&
        fwrite(buffer, 1, size, file) == size &&
        fwrite(suffix, 1, sizeof(suffix), file) == sizeof(suffix) &&
        fixture_metadata_footer(file, value_size) &&
        fixture_metadata_header(file, sizeof(recovery_tag)) &&
        fwrite(recovery_tag, 1, sizeof(recovery_tag), file) == sizeof(recovery_tag) &&
        fixture_metadata_footer(file, sizeof(recovery_tag));
    free(buffer);
    return success ? 0 : 1;
}
