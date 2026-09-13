/* Generate disposable nested metadata fixtures without recursive construction. */
#include <stdio.h>
#include <stdlib.h>
#include "src/amf.h"
#include "util.h"

/* FLV lengths and offsets are big-endian, including the three-byte tag size. */
static void put_uint(FILE * file, size_t value, unsigned int bytes) {
    while (bytes > 0) {
        --bytes;
        fputc((int)((value >> (bytes * 8)) & 255), file);
    }
}

static void put_tag(FILE * file, const byte * value, size_t size, int wrap) {
    /* AMF string: type, two-byte length, then "onMetaData". */
    static const byte name[] = {2, 0, 10, 'o','n','M','e','t','a','D','a','t','a'};
    /* ECMA array: type, four-byte count of one, then property "nested".
       Wrapping gives the checker the conventional onMetaData root type. */
    static const byte prefix[] = {8,0,0,0,1,0,6,'n','e','s','t','e','d'};
    size_t body_size = sizeof(name) + size + (wrap ? sizeof(prefix) + 3 : 0);
    /* Eleven-byte FLV script-tag header: type, body size, timestamp (with
       extension byte), and stream ID. All timestamps and stream IDs are zero. */
    fputc(18, file);
    put_uint(file, body_size, 3);
    put_uint(file, 0, 4);
    put_uint(file, 0, 3);
    fwrite(name, 1, sizeof(name), file);
    if (wrap) fwrite(prefix, 1, sizeof(prefix), file);
    fwrite(value, 1, size, file);
    if (wrap) { fputc(0, file); fputc(0, file); fputc(AMF_TYPE_END, file); }
    /* PreviousTagSize includes the tag header and body, but not this field. */
    put_uint(file, 11 + body_size, 4);
}

int main(int argc, char ** argv) {
    FILE * file;
    byte * buffer;
    size_t size;
    unsigned int depth, kind;
    /* Second tag: ECMA array {recovery: true}, including its end marker.
       Its recognizable property proves the reader reached the following tag. */
    static const byte tail[] = {8,0,0,0,1,0,8,'r','e','c','o','v','e','r','y',1,1,0,0,9};
    /* Arguments: output path, total container depth, fixture kind (0..3). */
    if (argc != 4) return 1;
    depth = (unsigned int)strtoul(argv[2], NULL, 10);
    kind = (unsigned int)strtoul(argv[3], NULL, 10);
    if (depth == 0 || depth > 70000 || kind > 3) return 1;
    /* put_tag adds one enclosing ECMA array, so reserve a level for it. */
    buffer = nested_amf(depth - 1, kind, 0, 0, &size);
    if (buffer == NULL) return 1;
    file = fopen(argv[1], "wb");
    if (file == NULL) { free(buffer); return 1; }
    /* FLV v1, no audio/video flags, nine-byte header, then PreviousTagSize0.
       This minimal file intentionally contains metadata only. */
    fwrite("FLV\1\0\0\0\0\11", 1, 9, file);
    put_uint(file, 0, 4);
    put_tag(file, buffer, size, 1);
    put_tag(file, tail, sizeof(tail), 0);
    free(buffer);
    if (ferror(file)) { fclose(file); return 1; }
    return fclose(file) == 0 ? 0 : 1;
}
