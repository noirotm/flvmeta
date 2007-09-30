/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007 Marc Noirot <marc.noirot AT gmail.com>

    This file is part of FLVMeta.

    FLVMeta is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    FLVMeta is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FLVMeta; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "types.h"

#ifndef WORDS_BIGENDIAN
/* swap 16 bits integers */
uint16 swap_uint16(uint16 s) {
    return (((s & 0x00FFU) << 8) |
            ((s & 0xFF00U) >> 8));
}

sint16 swap_sint16(sint16 s) {
    return (((s & 0x00FF) << 8) |
            ((s & 0xFF00) >> 8));
}

/* swap 32 bits integers */
uint32 swap_uint32(uint32 l) {
    return (((l & 0x000000FFU) << 24) |
            ((l & 0x0000FF00U) << 8)  |
            ((l & 0x00FF0000U) >> 8)  |
            ((l & 0xFF000000U) >> 24));
}

/* swap 64 bits doubles */
typedef union __convert_u {
    uint64 i;
    number64 f;
} convert_u;

number64 swap_number64(number64 n) {
    convert_u c;
    c.f = n;
    c.i = (((c.i & 0x00000000000000FFULL) << 56) |
           ((c.i & 0x000000000000FF00ULL) << 40) |
           ((c.i & 0x0000000000FF0000ULL) << 24) |
           ((c.i & 0x00000000FF000000ULL) << 8)  |
           ((c.i & 0x000000FF00000000ULL) >> 8)  |
           ((c.i & 0x0000FF0000000000ULL) >> 24) |
           ((c.i & 0x00FF000000000000ULL) >> 40) |
           ((c.i & 0xFF00000000000000ULL) >> 56));
    return c.f;
}
#endif /* !WORDS_BIGENDIAN */

/* convert big endian 24 bits integers to native integers */
uint32 uint24_be_to_uint32(uint24_be l) {
#ifdef WORDS_BIGENDIAN
    return (((l.b2) << 16) |
            ((l.b1) << 8)  |
             (l.b0));
#else
    return (((l.b0) << 16) |
            ((l.b1) << 8)  |
             (l.b2));
#endif
}

/* convert native integers into 24 bits big endian integers */
uint24_be uint32_to_uint24_be(uint32 l) {
    uint24_be r;
#ifdef WORDS_BIGENDIAN
    r.b0 = (uint8) (l & 0x000000FFU);
    r.b1 = (uint8)((l & 0x0000FF00U) >> 8);
    r.b2 = (uint8)((l & 0x00FF0000U) >> 16);
#else
    r.b0 = (uint8)((l & 0x00FF0000U) >> 16);
    r.b1 = (uint8)((l & 0x0000FF00U) >> 8);
    r.b2 = (uint8) (l & 0x000000FFU);
#endif
    return r;
}
