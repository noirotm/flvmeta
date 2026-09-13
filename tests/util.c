/* Shared test utilities. */
#include <stdlib.h>
#include "util.h"
#include "src/amf.h"

byte * nested_amf(unsigned int depth, unsigned int kind,
    int empty_name, int empty_leaf, size_t * size) {
    size_t capacity = (size_t)depth * 18 + 2;
    byte * buffer = (byte *)malloc(capacity);
    byte * p = buffer;
    unsigned int i;
    unsigned int type;
    int empty;

    /* 18 bytes per level safely covers the largest container encoding,
       including its property names, scalar sibling, and closing marker.
       The final boolean needs two more bytes. Callers use bounded depths. */
    if (buffer == NULL) {
        return NULL;
    }
    /* Emit outer-to-inner headers, leaving named containers open. */
    for (i = 0; i < depth; ++i) {
        type = kind == 3 ? i % 3 : kind;
        empty = empty_leaf && i + 1 == depth;
        *p++ = type == 0 ? AMF_TYPE_ARRAY :
            (type == 1 ? AMF_TYPE_OBJECT : AMF_TYPE_ASSOCIATIVE_ARRAY);
        if (type != 1) {
            /* Strict and ECMA arrays have a four-byte big-endian count. */
            *p++ = 0; *p++ = 0; *p++ = 0; *p++ = empty ? 0 : 2;
        }
        if (!empty) {
            if (type != 0) {
                /* Property names are length-prefixed strings without a type byte. */
                *p++ = 0; *p++ = 1; *p++ = 's';
            }
            *p++ = AMF_TYPE_BOOLEAN; *p++ = 1;
            if (type != 0) {
                *p++ = 0; *p++ = empty_name ? 0 : 1;
                if (!empty_name) *p++ = 'a';
            }
        }
    }
    /* Complete the deepest child, unless the innermost container is empty. */
    if (!empty_leaf || depth == 0) {
        *p++ = AMF_TYPE_BOOLEAN; *p++ = 1;
    }
    /* Close from inside out. Objects and ECMA arrays end with an empty
       property name followed by AMF_TYPE_END; strict arrays use their count. */
    for (i = depth; i > 0; --i) {
        type = kind == 3 ? (i - 1) % 3 : kind;
        if (type != 0) {
            *p++ = 0; *p++ = 0; *p++ = AMF_TYPE_END;
        }
    }
    *size = (size_t)(p - buffer);
    return buffer;
}
