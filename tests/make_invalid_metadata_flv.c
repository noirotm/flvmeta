/* Generate the small wrong-type onMetaData fixture for issue #30. */
#include <stdio.h>
#if defined(_WIN32)
#include <process.h>
#else
#include <unistd.h>
#endif

#ifndef FLVMETA_TEST_TMP_DIR
#define FLVMETA_TEST_TMP_DIR "."
#endif

int main(void) {
    FILE * file;
    char path[1024];
    unsigned long process_id;
    int written;
    int failed;
    static const unsigned char fixture[] = {
        /* FLV v1, no media streams, nine-byte header, PreviousTagSize0. */
        'F', 'L', 'V', 1, 0, 0, 0, 0, 9, 0, 0, 0, 0,
        /* Script tag: 16-byte body, timestamp zero, stream ID zero. */
        18, 0, 0, 16, 0, 0, 0, 0, 0, 0, 0,
        /* AMF event name followed by an empty string, not an ECMA array. */
        2, 0, 10, 'o', 'n', 'M', 'e', 't', 'a', 'D', 'a', 't', 'a',
        2, 0, 0,
        /* PreviousTagSize = 11-byte tag header + 16-byte body. */
        0, 0, 0, 27
    };

#if defined(_WIN32)
    process_id = (unsigned long)_getpid();
#else
    process_id = (unsigned long)getpid();
#endif
    written = snprintf(path, sizeof(path),
        "%s/flvmeta-test-%lu-invalid-metadata.flv",
        FLVMETA_TEST_TMP_DIR, process_id);
    if (written < 0 || (size_t)written >= sizeof(path)) {
        return 1;
    }
    file = fopen(path, "wb");
    if (file == NULL) {
        return 1;
    }
    failed = fwrite(fixture, 1, sizeof(fixture), file) != sizeof(fixture);
    if (fclose(file) != 0) {
        failed = 1;
    }
    if (failed) {
        remove(path);
        return 1;
    }
    /* The CMake driver uses this process-specific path and removes it. */
    return puts(path) == EOF ? 1 : 0;
}
