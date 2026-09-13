/* Shared test utilities. */
#ifndef FLVMETA_TEST_UTIL_H
#define FLVMETA_TEST_UTIL_H

#include "src/types.h"

/* Build encoded AMF bytes directly, without using the reader or writer under
   test, and without recursing in the fixture generator itself.

   kind: 0 = strict array, 1 = object, 2 = ECMA array, 3 = alternating types.
   A nonempty container contains [true, child], or {s: true, a: child} for
   named containers. The sibling ensures error unwinding has already allocated
   data to free before it encounters the over-depth child.

   depth counts containers, including the root. Normally the final child is
   boolean true; empty_leaf instead makes the innermost container empty.
   empty_name replaces the child property name with an empty string to exercise
   the ECMA reader's special termination handling.

   Returns an allocated buffer owned by the caller; size receives its length. */
byte * nested_amf(unsigned int depth, unsigned int kind,
    int empty_name, int empty_leaf, size_t * size);

#endif
