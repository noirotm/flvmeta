#include <stdio.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct __bit_buffer {
    byte * start;
    size_t size;
    byte * current;
    uint8 read_bits;
} bit_buffer;

void skip_bits(bit_buffer * bb, size_t nbits);
sint8 get_bit(bit_buffer * bb);
uint8 get_bits(bit_buffer * bb, size_t nbits, uint32 * ret);
uint32 exp_golomb_ue(bit_buffer * bb);
sint32 exp_golomb_se(bit_buffer * bb);

#ifdef __cplusplus
}
#endif /* __cplusplus */
