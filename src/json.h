/*
    $Id: json.h 231 2011-06-27 13:46:19Z marc.noirot $

    FLV Metadata updater

    Copyright (C) 2007-2012 Marc Noirot <marc.noirot AT gmail.com>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
*/

#ifndef __JSON_H__
#define __JSON_H__

#include "types.h"

/**
    This is a basic JSON emitter.
    It prints JSON-formatted data to stdout without creating
    an in-memory tree.
*/

/* json emitter structure */
typedef struct __json_emitter {
    uint32 comma_level;
    uint32 object_level;
} json_emitter;


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

void json_emit_init(json_emitter * je);

void json_emit_object_start(json_emitter * je);

void json_emit_object_end(json_emitter * je);

void json_emit_array_start(json_emitter * je);

void json_emit_array_end(json_emitter * je);

void json_emit_boolean(json_emitter * je, byte value);

void json_emit_null(json_emitter * je);

void json_emit_number(json_emitter * je, number64 value);

void json_emit_string(json_emitter * je, const char * str, size_t bytes);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __JSON_H__ */
