#include "fixture.h"
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

static int put_uint(FILE * file, size_t value, unsigned int bytes) {
    while (bytes > 0) {
        --bytes;
        if (fputc((int)((value >> (bytes * 8)) & 255), file) == EOF) {
            return 0;
        }
    }
    return 1;
}

int fixture_metadata_header(FILE * file, size_t value_size) {
    static const unsigned char name[] = {
        2, 0, 10, 'o', 'n', 'M', 'e', 't', 'a', 'D', 'a', 't', 'a'
    };
    /* The body length is 24 bits. Check before adding the event name. */
    if (value_size > 0xFFFFFFUL - sizeof(name)) {
        return 0;
    }
    return fputc(18, file) != EOF &&
        put_uint(file, sizeof(name) + value_size, 3) &&
        put_uint(file, 0, 4) && /* timestamp including extension byte */
        put_uint(file, 0, 3) && /* stream ID */
        fwrite(name, 1, sizeof(name), file) == sizeof(name);
}

int fixture_metadata_footer(FILE * file, size_t value_size) {
    if (value_size > 0xFFFFFFUL - 13) {
        return 0;
    }
    /* PreviousTagSize = tag header + encoded event name + AMF value. */
    return put_uint(file, 11 + 13 + value_size, 4);
}

/* Arguments: existing output directory, followed by issue-specific arguments.
   Print the generated path for the CMake driver to inspect and clean up. */
int main(int argc, char ** argv) {
    FILE * file;
    char path[1024];
    unsigned long process_id;
    int written;
    int failed;
    static const unsigned char header[] = {
        /* FLV v1, no media streams, nine-byte header, PreviousTagSize0. */
        'F', 'L', 'V', 1, 0, 0, 0, 0, 9, 0, 0, 0, 0
    };

    if (argc < 2) {
        return 1;
    }
#if defined(_WIN32)
    process_id = (unsigned long)_getpid();
#else
    process_id = (unsigned long)getpid();
#endif
    written = snprintf(path, sizeof(path), "%s/flvmeta-test-%lu.flv", argv[1], process_id);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return 1;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    failed = fwrite(header, 1, sizeof(header), file) != sizeof(header);
    if (!failed) {
        failed = write_flv_fixture(file, argc - 2, argv + 2) != 0;
    }
    if (ferror(file)) {
        failed = 1;
    }
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (!failed && (puts(path) == EOF || fflush(stdout) == EOF)) {
        failed = 1;
    }
    if (failed) {
        remove(path);
    }
    return failed ? 1 : 0;
}
