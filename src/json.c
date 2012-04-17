/*
    $Id: json.c 231 2011-06-27 13:46:19Z marc.noirot $

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

#include <stdio.h>
#include "json.h"

void json_emit_init(json_emitter * je) {
    je->comma_level = 0;
    je->object_level = 0;
}

void json_emit_object_start(json_emitter * je) {
    if (je->comma_level > 0) {
        printf(",");
        --je->comma_level;
    }
    printf("{");
    ++je->object_level;
}

void json_emit_object_end(json_emitter * je) {
    printf("}");
    --je->object_level;
}

void json_emit_array_start(json_emitter * je) {
}

void json_emit_array_end(json_emitter * je) {
}

void json_emit_boolean(json_emitter * je, byte value) {
}

void json_emit_null(json_emitter * je) {
}

void json_emit_number(json_emitter * je, number64 value) {
}

void json_emit_string(json_emitter * je, const char * str, size_t bytes) {
}
