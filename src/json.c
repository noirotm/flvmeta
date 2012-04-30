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

static void json_print_string(const char * str, size_t bytes) {
    size_t i;
    printf("\"");
    for (i = 0; i < bytes; ++i) {
        switch (str[i]) {
            case '\"': printf("\\\""); break;
            case '\\': printf("\\\\"); break;
            case '/':  printf("\\/");  break;
            case '\b': printf("\\b");  break;
            case '\f': printf("\\f");  break;
            case '\n': printf("\\n");  break;
            case '\r': printf("\\r");  break;
            case '\t': printf("\\t");  break;
            default: printf("%c", str[i]);
        }
    }
    printf("\"");
}

static void json_print_comma(json_emitter * je) {
    if (je->print_comma != 0) {
        printf(",");
        je->print_comma = 0;
    }
}

void json_emit_init(json_emitter * je) {
    je->print_comma = 0;
}

void json_emit_object_start(json_emitter * je) {
    json_print_comma(je);
    printf("{");
}

void json_emit_object_key(json_emitter * je, const char * str, size_t bytes) {
    json_print_comma(je);
    json_print_string(str, bytes);
    printf(":");
    je->print_comma = 0;
}

void json_emit_object_end(json_emitter * je) {
    printf("}");
    je->print_comma = 1;
}

void json_emit_array_start(json_emitter * je) {
    json_print_comma(je);
    printf("[");
}

void json_emit_array_end(json_emitter * je) {
    printf("]");
    je->print_comma = 1;
}

void json_emit_boolean(json_emitter * je, byte value) {
    json_print_comma(je);
    printf("%s", value != 0 ? "true" : "false");
    je->print_comma = 1;
}

void json_emit_null(json_emitter * je) {
    json_print_comma(je);
    printf("null");
    je->print_comma = 1;
}

void json_emit_number(json_emitter * je, number64 value) {
    json_print_comma(je);
    printf("%.12g", value);
    je->print_comma = 1;
}

void json_emit_string(json_emitter * je, const char * str, size_t bytes) {
    json_print_comma(je);
    json_print_string(str, bytes);
    je->print_comma = 1;
}
