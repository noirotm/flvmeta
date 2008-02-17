/*
    $Id$

    FLV Metadata updater

    Copyright (C) 2007, 2008 Marc Noirot <marc.noirot AT gmail.com>

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
#include <string.h>

#include "flvmeta.h"
#include "amf.h"
#include "types.h"

/* function common to all array types */
static void amf_list_init(amf_list * list) {
    if (list != NULL) {
        list->size = 0;
        list->first_element = NULL;
        list->last_element = NULL;
    }
}

static amf_data * amf_list_push(amf_list * list, amf_data * data) {
    amf_node * node = (amf_node*)malloc(sizeof(amf_node));
    if (node != NULL) {
        node->data = data;
        node->next = NULL;
        node->prev = NULL;
        if (list->size == 0) {
            list->first_element = node;
            list->last_element = node;
        }
        else {
            list->last_element->next = node;
            node->prev = list->last_element;
            list->last_element = node;
        }
        ++(list->size);
        return data;
    }
    return NULL;
}

static amf_data * amf_list_insert_before(amf_list * list, amf_node * node, amf_data * data) {
    if (node != NULL) {
        amf_node * new_node = (amf_node*)malloc(sizeof(amf_node));
        if (new_node != NULL) {
            new_node->next = node;
            new_node->prev = node->prev;

            if (node->prev != NULL) {
                node->prev->next = new_node;
                node->prev = new_node;
            }
            if (node == list->first_element) {
                list->first_element = new_node;
            }
            ++(list->size);
            new_node->data = data;
            return data;
        }
    }
    return NULL;
}

static amf_data * amf_list_insert_after(amf_list * list, amf_node * node, amf_data * data) {
    if (node != NULL) {
        amf_node * new_node = (amf_node*)malloc(sizeof(amf_node));
        if (new_node != NULL) {
            new_node->next = node->next;
            new_node->prev = node;

            if (node->next != NULL) {
                node->next->prev = new_node;
                node->next = new_node;
            }
            if (node == list->last_element) {
                list->last_element = new_node;
            }
            ++(list->size);
            new_node->data = data;
            return data;
        }
    }
    return NULL;
}

static amf_data * amf_list_delete(amf_list * list, amf_node * node) {
    amf_data * data = NULL;
    if (node != NULL) {
        if (node->next != NULL) {
            node->next->prev = node->prev;
        }
        if (node->prev != NULL) {
            node->prev->next = node->next;
        }
        if (node == list->first_element) {
            list->first_element = node->next;
        }
        if (node == list->last_element) {
            list->last_element = node->prev;
        }
        data = node->data;
        free(node);
        --(list->size);
    }
    return data;
}

static amf_data * amf_list_get_at(amf_list * list, uint32 n) {
    if (n < list->size) {
        uint32 i;
        amf_node * node = list->first_element;
        for (i = 0; i < n; ++i) {
            node = node->next;
        }
        return node->data;
    }
    return NULL;
}

static amf_data * amf_list_pop(amf_list * list) {
    return amf_list_delete(list, list->last_element);
}

static amf_node * amf_list_first(amf_list * list) {
    return list->first_element;
}

static amf_node * amf_list_last(amf_list * list) {
    return list->last_element;
}

static void amf_list_clear(amf_list * list) {
    amf_node * node = list->first_element;
    while (node != NULL) {
        amf_data_free(node->data);
        amf_node * tmp = node;
        node = node->next;
        free(tmp);
    }
    list->size = 0;
}

/* structure used to mimic a stream with a memory buffer */
typedef struct __buffer_context {
    byte * start_address;
    byte * current_address;
    size_t buffer_size;
} buffer_context;

/* callback function to mimic fread using a memory buffer */
static size_t buffer_read(void * out_buffer, size_t size, void * user_data) {
    buffer_context * ctxt = (buffer_context *)user_data;
    if (ctxt->current_address >= ctxt->start_address &&
        ctxt->current_address + size <= ctxt->start_address + ctxt->buffer_size) {
        
        memcpy(out_buffer, ctxt->current_address, size);
        ctxt->current_address += size;
        return size;
    }
    else {
        return 0;
    }
}

/* callback function to mimic fwrite using a memory buffer */
static size_t buffer_write(const void * in_buffer, size_t size, void * user_data) {
    buffer_context * ctxt = (buffer_context *)user_data;
    if (ctxt->current_address >= ctxt->start_address &&
        ctxt->current_address + size <= ctxt->start_address + ctxt->buffer_size) {
        
        memcpy(ctxt->current_address, in_buffer, size);
        ctxt->current_address += size;
        return size;
    }
    else {
        return 0;
    }
}

/* read AMF data from buffer */
amf_data * amf_data_buffer_read(byte * buffer, size_t maxbytes) {
    buffer_context ctxt;
    ctxt.start_address = ctxt.current_address = buffer;
    ctxt.buffer_size = maxbytes;
    return amf_data_read(buffer_read, &ctxt);
}

/* write AMF data to buffer */
size_t amf_data_buffer_write(amf_data * data, byte * buffer, size_t maxbytes) {
    buffer_context ctxt;
    ctxt.start_address = ctxt.current_address = buffer;
    ctxt.buffer_size = maxbytes;
    return amf_data_write(data, buffer_write, &ctxt);
}

/* callback function to read data from a file stream */
static size_t file_read(void * out_buffer, size_t size, void * user_data) {
    return fread(out_buffer, sizeof(byte), size, (FILE *)user_data);
}

/* callback function to write data to a file stream */
static size_t file_write(const void * in_buffer, size_t size, void * user_data) {
    return fwrite(in_buffer, sizeof(byte), size, (FILE *)user_data);
}

/* load AMF data from a file stream */
amf_data * amf_data_file_read(FILE * stream) {
    return amf_data_read(file_read, stream);
}

/* write AMF data into a file stream */
size_t amf_data_file_write(amf_data * data, FILE * stream) {
    return amf_data_write(data, file_write, stream);
}

/* read a number */
static amf_data * amf_number_read(amf_read_proc read_proc, void * user_data) {
    number64_be val;
    if (read_proc(&val, sizeof(number64_be), user_data) == sizeof(number64_be)) {
        return amf_number_new(swap_number64(val));
    }
    return NULL;
}

/* read a boolean */
static amf_data * amf_boolean_read(amf_read_proc read_proc, void * user_data) {
    uint8 val;
    if (read_proc(&val, sizeof(uint8), user_data) == sizeof(uint8)) {
        return amf_boolean_new(val);
    }
    return NULL;
}

/* read a string */
static amf_data * amf_string_read(amf_read_proc read_proc, void * user_data) {
    uint16_be strsize;
    byte * buffer;
    if (read_proc(&strsize, sizeof(uint16_be), user_data) == sizeof(uint16_be)) {
        strsize = swap_uint16(strsize);
        buffer = (byte*)calloc(strsize, sizeof(byte));
        if (buffer != NULL && read_proc(buffer, strsize, user_data) == strsize) {
            amf_data * data = amf_string_new(buffer, strsize);
            free(buffer);
            return data;
        }
    }
    return NULL;
}

/* read an object */
static amf_data * amf_object_read(amf_read_proc read_proc, void * user_data) {
    amf_data * data = amf_object_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        while (1) {
            name = amf_string_read(read_proc, user_data);
            if (name != NULL) {
                element = amf_data_read(read_proc, user_data);
                if (element != NULL) {
                    if (amf_object_add(data, name, element) == NULL) {
                        amf_data_free(name);
                        amf_data_free(element);
                        amf_data_free(data);
                        return NULL;
                    }
                }
                else {
                    amf_data_free(name);
                    break;
                }
            }
            else {
                /* invalid name: error */
                amf_data_free(data);
                return NULL;
            }
        }
    }
    return data;
}

/* read an associative array */
static amf_data * amf_associative_array_read(amf_read_proc read_proc, void * user_data) {
    amf_data * data = amf_associative_array_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        uint32_be size;
        if (read_proc(&size, sizeof(uint32_be), user_data) == sizeof(uint32_be)) {
            /* we ignore the 32 bits array size marker */
            while(1) {
                name = amf_string_read(read_proc, user_data);
                if (name != NULL) {
                    element = amf_data_read(read_proc, user_data);
                    if (element != NULL) {
                        if (amf_associative_array_add(data, name, element) == NULL) {
                            amf_data_free(name);
                            amf_data_free(element);
                            amf_data_free(data);
                            return NULL;
                        }
                    }
                    else {
                        amf_data_free(name);
                        break;
                    }
                }
                else {
                    /* invalid name: error */
                    amf_data_free(data);
                    return NULL;
                }
            }
        }
        else {
            amf_data_free(data);
            return NULL;
        }
    }
    return data;
}

/* read an array */
static amf_data * amf_array_read(amf_read_proc read_proc, void * user_data) {
    amf_data * data = amf_array_new();
    if (data != NULL) {
        uint32 array_size;
        if (read_proc(&array_size, sizeof(uint32), user_data) == sizeof(uint32)) {
            array_size = swap_uint32(array_size);

            size_t i;
            amf_data * element;
            for (i = 0; i < array_size; ++i) {
                element = amf_data_read(read_proc, user_data);

                if (element != NULL) {
                    if (amf_array_push(data, element) == NULL) {
                        amf_data_free(element);
                        amf_data_free(data);
                        return NULL;
                    }
                }
                else {
                    amf_data_free(data);
                    return NULL;
                }
            }
        }
        else {
            amf_data_free(data);
            return NULL;
        }
    }
    return data;
}

/* read a date */
static amf_data * amf_date_read(amf_read_proc read_proc, void * user_data) {
    number64_be milliseconds;
    sint16_be timezone;
    if (read_proc(&milliseconds, sizeof(number64_be), user_data) == sizeof(number64_be) &&
        read_proc(&timezone, sizeof(sint16_be), user_data) == sizeof(sint16_be)) {
        return amf_date_new(swap_number64(milliseconds), swap_sint16(timezone));
    }
    else {
        return NULL;
    }
}

/* load AMF data from stream */
amf_data * amf_data_read(amf_read_proc read_proc, void * user_data) {
    byte type;
    if (read_proc(&type, sizeof(byte), user_data) == sizeof(byte)) {
        switch (type) {
            case AMF_TYPE_NUMBER:
                return amf_number_read(read_proc, user_data);
            case AMF_TYPE_BOOLEAN:
                return amf_boolean_read(read_proc, user_data);
            case AMF_TYPE_STRING:
                return amf_string_read(read_proc, user_data);
            case AMF_TYPE_OBJECT:
                return amf_object_read(read_proc, user_data);
            case AMF_TYPE_UNDEFINED:
                return amf_undefined_new();
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                return amf_associative_array_read(read_proc, user_data);
            case AMF_TYPE_ARRAY:
                return amf_array_read(read_proc, user_data);
            case AMF_TYPE_DATE:
                return amf_date_read(read_proc, user_data);
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML:
            case AMF_TYPE_CLASS:
            case AMF_TYPE_TERMINATOR:
                return NULL; /* end of composite object */
            default:
                break;
        }
    }
    return NULL;
}

/* determines the size of the given AMF data */
size_t amf_data_size(amf_data * data) {
    size_t s = 0;
    amf_node * node;
    if (data != NULL) {
        s += sizeof(byte);
        switch (data->type) {
            case AMF_TYPE_NUMBER:
                s += sizeof(number64_be);
                break;
            case AMF_TYPE_BOOLEAN:
                s += sizeof(uint8);
                break;
            case AMF_TYPE_STRING:
                s += sizeof(uint16) + (size_t)amf_string_get_size(data);
                break;
            case AMF_TYPE_OBJECT:
                node = amf_object_first(data);
                while (node != NULL) {
                    s += sizeof(uint16) + (size_t)amf_string_get_size(amf_object_get_name(node));
                    s += (size_t)amf_data_size(amf_object_get_data(node));
                    node = amf_object_next(node);
                }
                s += sizeof(uint16) + sizeof(uint8);
                break;
            case AMF_TYPE_UNDEFINED:
                break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                s += sizeof(uint32);
                node = amf_associative_array_first(data);
                while (node != NULL) {
                    s += sizeof(uint16) + (size_t)amf_string_get_size(amf_associative_array_get_name(node));
                    s += (size_t)amf_data_size(amf_associative_array_get_data(node));
                    node = amf_associative_array_next(node);
                }
                s += sizeof(uint16) + sizeof(uint8);
                break;
            case AMF_TYPE_ARRAY:
                s += sizeof(uint32);
                node = amf_array_first(data);
                while (node != NULL) {
                    s += (size_t)amf_data_size(amf_array_get(node));
                    node = amf_array_next(node);
                }
                break;
            case AMF_TYPE_DATE:
                s += sizeof(number64) + sizeof(sint16);
                break;
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML:
            case AMF_TYPE_CLASS:
            case AMF_TYPE_TERMINATOR:
                break; /* end of composite object */
            default:
                break;
        }
    }
    return s;
}

/* write a number */
static size_t amf_number_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    number64 n = swap_number64(data->number_data);
    return write_proc(&n, sizeof(number64_be), user_data);
}

/* write a boolean */
static size_t amf_boolean_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    return write_proc(&(data->boolean_data), sizeof(uint8), user_data);
}

/* write a string */
static size_t amf_string_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    uint16 s;
    size_t w = 0;
    
    s = swap_uint16(data->string_data.size);
    w = write_proc(&s, sizeof(uint16_be), user_data);
    if (data->string_data.size > 0) {
        w += write_proc(data->string_data.mbstr, (size_t)(data->string_data.size), user_data);
    }
    
    return w;
}

/* write an object */
static size_t amf_object_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    amf_node * node;
    size_t w = 0;
    uint16_be filler = swap_uint16(0);
    uint8 terminator = AMF_TYPE_TERMINATOR;

    node = amf_object_first(data);
    while (node != NULL) {
        w += amf_string_write(amf_object_get_name(node), write_proc, user_data);
        w += amf_data_write(amf_object_get_data(node), write_proc, user_data);
        node = amf_object_next(node);
    }
    
    /* empty string is the last element */
    w += write_proc(&filler, sizeof(uint16_be), user_data);
    /* an object ends with 0x09 */
    w += write_proc(&terminator, sizeof(uint8), user_data);

    return w;
}

/* write an associative array */
static size_t amf_associative_array_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    amf_node * node;
    size_t w = 0;
    uint32_be s;
    uint16_be filler = swap_uint16(0);
    uint8 terminator = AMF_TYPE_TERMINATOR;

    s = swap_uint32(data->list_data.size) / 2;
    w += write_proc(&s, sizeof(uint32_be), user_data);
    node = amf_associative_array_first(data);
    while (node != NULL) {
        w += amf_string_write(amf_associative_array_get_name(node), write_proc, user_data);
        w += amf_data_write(amf_associative_array_get_data(node), write_proc, user_data);
        node = amf_associative_array_next(node);
    }
    
    /* empty string is the last element */
    w += write_proc(&filler, sizeof(uint16_be), user_data);
    /* an object ends with 0x09 */
    w += write_proc(&terminator, sizeof(uint8), user_data);

    return w;
}

/* write an array */
static size_t amf_array_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    amf_node * node;
    size_t w = 0;
    uint32_be s;

    s = swap_uint32(data->list_data.size);
    w += write_proc(&s, sizeof(uint32_be), user_data);
    node = amf_array_first(data);
    while (node != NULL) {
        w += amf_data_write(amf_array_get(node), write_proc, user_data);
        node = amf_array_next(node);
    }

    return w;
}

/* write a date */
static size_t amf_date_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    size_t w = 0;
    number64_be milli;
    sint16_be tz;
    
    milli = swap_number64(data->date_data.milliseconds);
    w += write_proc(&milli, sizeof(number64_be), user_data);
    tz = swap_sint16(data->date_data.timezone);
    w += write_proc(&tz, sizeof(sint16_be), user_data);

    return w;
}

/* write amf data to stream */
size_t amf_data_write(amf_data * data, amf_write_proc write_proc, void * user_data) {
    size_t s = 0;
    if (data != NULL) {
        s += write_proc(&(data->type), sizeof(byte), user_data);
        switch (data->type) {
            case AMF_TYPE_NUMBER:
                s += amf_number_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_BOOLEAN:
                s += amf_boolean_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_STRING:
                s += amf_string_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_OBJECT:
                s += amf_object_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_UNDEFINED:
                break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                s += amf_associative_array_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_ARRAY:
                s += amf_array_write(data, write_proc, user_data);
                break;
            case AMF_TYPE_DATE:
                s += amf_date_write(data, write_proc, user_data);
                break;
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML:
            case AMF_TYPE_CLASS:
            case AMF_TYPE_TERMINATOR:
                break; /* end of composite object */
            default:
                break;
        }
    }
    return s;
}

/* data type */
byte amf_data_get_type(amf_data * data) {
    return (data != NULL) ? data->type : AMF_TYPE_UNDEFINED;
}

/* free AMF data */
void amf_data_free(amf_data * data) {
    if (data != NULL) {
        switch (data->type) {
            case AMF_TYPE_NUMBER: break;
            case AMF_TYPE_BOOLEAN: break;
            case AMF_TYPE_STRING: 
                if (data->string_data.mbstr != NULL) {
                    free(data->string_data.mbstr);
                } break;
            case AMF_TYPE_UNDEFINED: break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_OBJECT:
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
            case AMF_TYPE_ARRAY: amf_list_clear(&data->list_data); break;
            case AMF_TYPE_DATE: break;
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML: break;
            case AMF_TYPE_CLASS: break;
            default: break;
        }
        free(data);
    }
}

/* dump AMF data into a stream as text */
void amf_data_dump(FILE * stream, amf_data * data, int indent_level) {
    if (data != NULL) {
        amf_node * node;
        time_t time;
        struct tm * t;
        char datestr[64];
        switch (data->type) {
            case AMF_TYPE_NUMBER:
                fprintf(stream, "%f", data->number_data);
                break;
            case AMF_TYPE_BOOLEAN:
                fprintf(stream, "%s", (data->boolean_data) ? "true" : "false");
                break;
            case AMF_TYPE_STRING:
                fprintf(stream, "%s", data->string_data.mbstr);
                break;
            case AMF_TYPE_OBJECT:
                node = amf_object_first(data);
                fprintf(stream, "{\n");
                while (node != NULL) {
                    fprintf(stream, "%*s", (indent_level+1)*4, "");
                    amf_data_dump(stream, amf_object_get_name(node), indent_level+1);
                    fprintf(stream, ": ");
                    amf_data_dump(stream, amf_object_get_data(node), indent_level+1);
                    node = amf_object_next(node);
                    fprintf(stream, "\n");
                }
                fprintf(stream, "%*s", indent_level*4 + 1, "}");
                break;
            case AMF_TYPE_UNDEFINED:
                fprintf(stream, "undefined");
                break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                node = amf_associative_array_first(data);
                fprintf(stream, "{\n");
                while (node != NULL) {
                    fprintf(stream, "%*s", (indent_level+1)*4, "");
                    amf_data_dump(stream, amf_associative_array_get_name(node), indent_level+1);
                    fprintf(stream, " => ");
                    amf_data_dump(stream, amf_associative_array_get_data(node), indent_level+1);
                    node = amf_associative_array_next(node);
                    fprintf(stream, "\n");
                }
                fprintf(stream, "%*s", indent_level*4 + 1, "}");
                break;
            case AMF_TYPE_ARRAY:
                node = amf_array_first(data);
                fprintf(stream, "[\n");
                while (node != NULL) {
                    fprintf(stream, "%*s", (indent_level+1)*4, "");
                    amf_data_dump(stream, node->data, indent_level+1);
                    node = amf_array_next(node);
                    fprintf(stream, "\n");
                }
                fprintf(stream, "%*s", indent_level*4 + 1, "]");
                break;
            case AMF_TYPE_DATE:
                time = amf_date_to_time_t(data);
                t = gmtime(&time);
                strftime(datestr, sizeof(datestr), "%a, %d %b %Y %H:%M:%S %z", t);
                fprintf(stream, "%s", datestr);
                break;
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML: break;
            case AMF_TYPE_CLASS: break;
            default: break;
        }
    }
}

/* number functions */
amf_data * amf_number_new(number64 value) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_NUMBER;
        data->number_data = value;
    }
    return data;
}

number64 amf_number_get_value(amf_data * data) {
    return (data != NULL) ? data->number_data : 0;
}

void amf_number_set_value(amf_data * data, number64 value) {
    if (data != NULL) {
        data->number_data = value;
    }
}

/* boolean functions */
amf_data * amf_boolean_new(uint8 value) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_BOOLEAN;
        data->boolean_data = value;
    }
    return data;
}

uint8 amf_boolean_get_value(amf_data * data) {
    return (data != NULL) ? data->boolean_data : 0;
}

void amf_boolean_set_value(amf_data * data, uint8 value) {
    if (data != NULL) {
        data->boolean_data = value;
    }
}

/* string functions */
amf_data * amf_string_new(byte * str, uint16 size) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_STRING;
        data->string_data.size = size;

        if (size > 0) {
            data->string_data.mbstr = (byte*)calloc(size+1, sizeof(byte));
            if (data->string_data.mbstr != NULL) {
                memcpy(data->string_data.mbstr, str, size);
            }
            else {
                free(data);
                return NULL;
            }
        }
        else {
            data->string_data.mbstr = NULL;
        }
    }
    return data;
}

uint16 amf_string_get_size(amf_data * data) {
    return (data != NULL) ? data->string_data.size : 0;
}

byte * amf_string_get_bytes(amf_data * data) {
    return (data != NULL) ? data->string_data.mbstr : NULL;
}

/* object functions */
amf_data * amf_object_new(void) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_OBJECT;
        amf_list_init(&data->list_data);
    }
    return data;
}

uint32 amf_object_size(amf_data * data) {
    return (data != NULL) ? data->list_data.size / 2 : 0;
}

amf_data * amf_object_add(amf_data * data, amf_data * name, amf_data * element) {
    if (data != NULL) {
        if (amf_list_push(&data->list_data, name) != NULL) {
            if (amf_list_push(&data->list_data, element) != NULL) {
                return element;
            }
            else {
                amf_data_free(amf_list_pop(&data->list_data));
            }
        }
    }
    return NULL;
}

amf_data * amf_object_get(amf_data * data, const char * name) {
    if (data != NULL) {
        amf_node * node = amf_list_first(&(data->list_data));
        while (node != NULL) {
            node = node->next;
            if (strncmp((char*)(node->data->string_data.mbstr), name, (size_t)(node->data->string_data.size)) == 0) {
                node = node->next;
                return (node != NULL) ? node->data : NULL;
            }
            node = node->next;
        }
    }
    return NULL;
}

amf_data * amf_object_delete(amf_data * data, const char * name) {
    if (data != NULL) {
        amf_node * node = amf_list_first(&data->list_data);
        while (node != NULL) {
            node = node->next;
            if (strncmp((char*)(node->data->string_data.mbstr), name, (size_t)(node->data->string_data.size)) == 0) {
                amf_node * data_node = node->next;
                amf_data_free(amf_list_delete(&data->list_data, node));
                return amf_list_delete(&data->list_data, data_node);
            }
            else {
                node = node->next;
            }
        }
    }
    return NULL;
}

amf_node * amf_object_first(amf_data * data) {
    return (data != NULL) ? amf_list_first(&data->list_data) : NULL;
}

amf_node * amf_object_last(amf_data * data) {
    if (data != NULL) {
        amf_node * node = amf_list_last(&data->list_data);
        if (node != NULL) {
            return node->prev;
        }
    }
    return NULL;
}

amf_node * amf_object_next(amf_node * node) {
    if (node != NULL) {
        amf_node * next = node->next;
        if (next != NULL) {
            return next->next;
        }
    }
    return NULL;
}

amf_node * amf_object_prev(amf_node * node) {
    if (node != NULL) {
        amf_node * prev = node->prev;
        if (prev != NULL) {
            return prev->prev;
        }
    }
    return NULL;
}

amf_data * amf_object_get_name(amf_node * node) {
    return (node != NULL) ? node->data : NULL;
}

amf_data * amf_object_get_data(amf_node * node) {
    if (node != NULL) {
        amf_node * next = node->next;
        if (next != NULL) {
            return next->data;
        }
    }
    return NULL;
}


/* undefined functions */
amf_data * amf_undefined_new(void) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_UNDEFINED;
    }
    return data;
}

/* associative array functions */
amf_data * amf_associative_array_new(void) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_ASSOCIATIVE_ARRAY;
        amf_list_init(&data->list_data);
    }
    return data;
}

/* array functions */
amf_data * amf_array_new(void) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_ARRAY;
        amf_list_init(&data->list_data);
    }
    return data;
}

uint32 amf_array_size(amf_data * data) {
    return (data != NULL) ? data->list_data.size : 0;
}

amf_data * amf_array_push(amf_data * data, amf_data * element) {
    return (data != NULL) ? amf_list_push(&data->list_data, element) : NULL;
}

amf_data * amf_array_pop(amf_data * data) {
    return (data != NULL) ? amf_list_pop(&data->list_data) : NULL;
}

amf_node * amf_array_first(amf_data * data) {
    return (data != NULL) ? amf_list_first(&data->list_data) : NULL;
}

amf_node * amf_array_last(amf_data * data) {
    return (data != NULL) ? amf_list_last(&data->list_data) : NULL;
}

amf_node * amf_array_next(amf_node * node) {
    return (node != NULL) ? node->next : NULL;
}

amf_node * amf_array_prev(amf_node * node) {
    return (node != NULL) ? node->prev : NULL;
}

amf_data * amf_array_get(amf_node * node) {
    return (node != NULL) ? node->data : NULL;
}

amf_data * amf_array_get_at(amf_data * data, uint32 n) {
    return (data != NULL) ? amf_list_get_at(&data->list_data, n) : NULL;
}

amf_data * amf_array_delete(amf_data * data, amf_node * node) {
    return (data != NULL) ? amf_list_delete(&data->list_data, node) : NULL;
}

amf_data * amf_array_insert_before(amf_data * data, amf_node * node, amf_data * element) {
    return (data != NULL) ? amf_list_insert_before(&data->list_data, node, element) : NULL;
}

amf_data * amf_array_insert_after(amf_data * data, amf_node * node, amf_data * element) {
    return (data != NULL) ? amf_list_insert_after(&data->list_data, node, element) : NULL;
}

/* date functions */
amf_data * amf_date_new(number64 milliseconds, sint16 timezone) {
    amf_data * data = (amf_data*)malloc(sizeof(amf_data));
    if (data != NULL) {
        data->type = AMF_TYPE_DATE;
        data->date_data.milliseconds = milliseconds;
        data->date_data.timezone = timezone;
    }
    return data;
}

number64 amf_date_get_milliseconds(amf_data * data) {
    return (data != NULL) ? data->date_data.milliseconds : 0.0;
}

sint16 amf_date_get_timezone(amf_data * data) {
    return (data != NULL) ? data->date_data.timezone : 0;
}

time_t amf_date_to_time_t(amf_data * data) {
    return  (time_t)((data != NULL) ? data->date_data.milliseconds / 1000 + data->date_data.timezone * 60 : 0);
}
