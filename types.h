/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008, 2009 Marc Noirot <marc.noirot AT gmail.com>

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
#ifndef __TYPES_H__
#define __TYPES_H__

#include "flvmeta.h"

#ifdef HAVE_INTTYPES_H
#include <inttypes.h>
#endif

typedef uint8_t byte, uint8, uint8_bitmask;

typedef uint16_t uint16, uint16_be, uint16_le;

typedef int16_t sint16, sint16_be, sint16_le;

typedef uint32_t uint32, uint32_be, uint32_le;

typedef int32_t sint32, sint32_be, sint32_le;

#pragma pack(push, 1)
typedef struct __uint24 {
    uint8 b0, b1, b2;
} uint24, uint24_be, uint24_le;
#pragma pack(pop)

typedef uint64_t uint64, uint64_le, uint64_be;

typedef int64_t sint64, sint64_le, sint64_be;

typedef
#if SIZEOF_FLOAT == 8
float
#elif SIZEOF_DOUBLE == 8
double
#elif SIZEOF_LONG_DOUBLE == 8
long double
#else
uint64_t
#endif
number64, number64_le, number64_be;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef WORDS_BIGENDIAN

#  define swap_uint16(x) (x)
#  define swap_sint16(x) (x)
#  define swap_uint32(x) (x)
#  define swap_number64(x) (x)

/* convert big endian 24 bits integers to native integers */
#  define uint24_be_to_uint32(x) ((uint32)(((x).b2 << 16) | \
    ((x).b1 << 8) | (x).b0))

#else /* !defined WORDS_BIGENDIAN */

/* swap 16 bits integers */
#  define swap_uint16(x) ((uint16)((((x) & 0x00FFU) << 8) | \
    (((x) & 0xFF00U) >> 8)))
#  define swap_sint16(x) ((sint16)((((x) & 0x00FF) << 8) | \
    (((x) & 0xFF00) >> 8)))

/* swap 32 bits integers */
#  define swap_uint32(x) ((uint32)((((x) & 0x000000FFU) << 24) | \
    (((x) & 0x0000FF00U) << 8)  | \
    (((x) & 0x00FF0000U) >> 8)  | \
    (((x) & 0xFF000000U) >> 24)))

/* swap 64 bits doubles */
number64 swap_number64(number64);

/* convert big endian 24 bits integers to native integers */
#  define uint24_be_to_uint32(x) ((uint32)(((x).b0 << 16) | \
    ((x).b1 << 8) | (x).b2))

#endif /* WORDS_BIGENDIAN */

/* convert native integers into 24 bits big endian integers */
uint24_be uint32_to_uint24_be(uint32);

/* large file support */
#ifdef HAVE_FSEEKO
#  define flvmeta_ftell ftello
#  define flvmeta_fseek fseeko
#else
#  define flvmeta_ftell ftell
#  define flvmeta_fseek fseek
#  ifndef off_t
#    define off_t long
#  endif
#endif /* HAVE_FSEEKO */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TYPES_H__ */
