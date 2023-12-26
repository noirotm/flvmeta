/**
    bit buffer handling
*/

#include <stdlib.h>
#include <stdio.h>
#include "bitstream.h"

void skip_bits(bit_buffer * bb, size_t nbits) {
    bb->current = bb->current + (nbits + bb->read_bits) / 8;
    bb->read_bits = (uint8)((bb->read_bits + nbits) % 8);
}

sint8 get_bit(bit_buffer * bb) {
    sint8 ret;

    if (bb->current - bb->start > (ptrdiff_t)(bb->size - 1)) {
        return -1;
    }

    ret = (*(bb->current) >> (7 - bb->read_bits)) & 0x1;
    if (bb->read_bits == 7) {
        bb->read_bits = 0;
        bb->current++;
    }
    else {
        bb->read_bits++;
    }
    return ret;
}

uint8 get_bits(bit_buffer * bb, size_t nbits, uint32 * ret) {
    uint32 i;
    sint8 bit;

	if (nbits > sizeof(uint32) * 8) {
        nbits = sizeof(uint32) * 8;
	}

    *ret = 0;
    for (i = 0; i < nbits; i++) {
        bit = get_bit(bb);
        if (bit == -1) {
            return 0;
        }

        *ret = (*ret << 1) + bit;
    }
    return 1;
}

uint32 exp_golomb_ue(bit_buffer * bb) {
    sint8 bit;
    uint8 significant_bits;
    uint32 bits;

    significant_bits = 0;

    do {
        bit = get_bit(bb);
        if (bit == -1) {
            return 0;
        }
        if (bit == 0) {
            significant_bits++;
        }
    } while (bit == 0);

    if (!get_bits(bb, significant_bits, &bits))
        return 0;

    return (1 << significant_bits) + bits - 1;
}

sint32 exp_golomb_se(bit_buffer * bb) {
    sint32 ret;
    ret = exp_golomb_ue(bb);
    if ((ret & 0x1) == 0) {
        return -(ret >> 1);
    }

    return (ret + 1) >> 1;
}
