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

/* decode a number */
static amf_data * amf_number_decode(byte * buffer, size_t maxbytes, size_t * size) {
    number64_be val;
    if (maxbytes >= sizeof(number64_be)) {
        memcpy(&val, buffer, sizeof(number64_be));
        *size += sizeof(number64_be);
        return amf_number_new(swap_number64(val));
    }
    return NULL;
}

/* decode a boolean */
static amf_data * amf_boolean_decode(byte * buffer, size_t maxbytes, size_t * size) {
    uint8 val;
    if (maxbytes >= sizeof(uint8)) {
        memcpy(&val, buffer, sizeof(uint8));
        *size += sizeof(uint8);
        return amf_boolean_new(val);
    }
    return NULL;
}

/* decode a string */
static amf_data * amf_string_decode(byte * buffer, size_t maxbytes, size_t * size) {
    uint16 strsize;
    if (maxbytes >= sizeof(uint16)) {
        memcpy(&strsize, buffer, sizeof(uint16));
        strsize = swap_uint16(strsize);
        if ((maxbytes-sizeof(uint16)) >= strsize) {
            *size += (sizeof(uint16)+strsize);
            return amf_string_new(buffer+sizeof(uint16), strsize);
        }
    }
    return NULL;
}

/* decode an object */
static amf_data * amf_object_decode(byte * buffer, size_t maxbytes, size_t * size) {
    amf_data * data = amf_object_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        size_t s = 0;
        while (1) {
            name = amf_string_decode(buffer+s, maxbytes-s, size);
            if (name != NULL) {
                s += amf_string_get_size(name) + sizeof(uint16);
                size_t tmp_size = *size;
                element = amf_data_decode(buffer+s, maxbytes-s, size);
                s += *size - tmp_size;
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

/* decode an associative array */
static amf_data * amf_associative_array_decode(byte * buffer, size_t maxbytes, size_t * size) {
    amf_data * data = amf_associative_array_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        if (maxbytes >= sizeof(uint32)) {
            size_t s = sizeof(uint32); /* we ignore the 32 bits array size marker */
            *size += s;
            while(1) {
                name = amf_data_decode(buffer+s, maxbytes-s, size);
                if (name != NULL) {
                    s += amf_string_get_size(name) + sizeof(uint16);
                    size_t tmp_size = *size;
                    element = amf_data_decode(buffer+s, maxbytes-s, size);
                    s += *size - tmp_size;
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

/* decode an array */
static amf_data * amf_array_decode(byte * buffer, size_t maxbytes, size_t * size) {
    amf_data * data = amf_array_new();
    if (data != NULL) {
        uint32 array_size;
        if (maxbytes >= sizeof(uint32)) {
            memcpy(&array_size, buffer, sizeof(uint32));
            array_size = swap_uint32(array_size);

            size_t i;
            size_t s = sizeof(uint32);
            *size += s;

            amf_data * element;
            for (i = 0; i < array_size; ++i) {
                size_t tmp_size = *size;
                element = amf_data_decode(buffer+s, maxbytes-s, size);
                s += *size - tmp_size;

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

/* decode a date */
static amf_data * amf_date_decode(byte * buffer, size_t maxbytes, size_t * size) {
    number64_be milliseconds;
    sint16_be timezone;
    if (maxbytes >= sizeof(number64_be)+sizeof(sint16_be)) {
        memcpy(&milliseconds, buffer, sizeof(number64_be));
        memcpy(&timezone, buffer+sizeof(number64_be), sizeof(sint16_be));
        *size += sizeof(number64_be)+sizeof(sint16_be);
        return amf_date_new(swap_number64(milliseconds), swap_sint16(timezone));
    }
    else {
        return NULL;
    }
}

/* load AMF data from buffer */
amf_data * amf_data_decode(byte * buffer, size_t maxbytes, size_t * size) {
    amf_data * data = NULL;
    if (buffer != NULL && maxbytes > 0) {
        *size += sizeof(byte);
        switch (buffer[0]) {
            case AMF_TYPE_NUMBER:
                data = amf_number_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_BOOLEAN:
                data = amf_boolean_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_STRING:
                data = amf_string_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_OBJECT:
                data = amf_object_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_UNDEFINED:
                data = amf_undefined_new();
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                data = amf_associative_array_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_ARRAY:
                data = amf_array_decode(buffer+1, maxbytes-1, size);
            case AMF_TYPE_DATE:
                data = amf_date_decode(buffer+1, maxbytes-1, size);
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML:
            case AMF_TYPE_CLASS:
            case AMF_TYPE_TERMINATOR:
                break; /* end of composite object */
            default:
                break;
        }
    }
    return data;
}

/* read a number */
static amf_data * amf_number_read(FILE * stream) {
    number64_be val;
    if (fread(&val, sizeof(number64_be), 1, stream) == 1) {
        return amf_number_new(swap_number64(val));
    }
    return NULL;
}

/* read a boolean */
static amf_data * amf_boolean_read(FILE * stream) {
    uint8 val;
    if (fread(&val, sizeof(uint8), 1, stream) == 1) {
        return amf_boolean_new(val);
    }
    return NULL;
}

/* read a string */
static amf_data * amf_string_read(FILE * stream) {
    uint16_be strsize;
    byte * buffer;
    if (fread(&strsize, sizeof(uint16_be), 1, stream) == 1) {
        strsize = swap_uint16(strsize);
        buffer = (byte*)calloc(strsize, sizeof(byte));
        if (buffer != NULL && fread(buffer, sizeof(byte), strsize, stream) == strsize) {
            amf_data * data = amf_string_new(buffer, strsize);
            free(buffer);
            return data;
        }
    }
    return NULL;
}

/* read an object */
static amf_data * amf_object_read(FILE * stream) {
    amf_data * data = amf_object_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        while (1) {
            name = amf_string_read(stream);
            if (name != NULL) {
                element = amf_data_read(stream);
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
static amf_data * amf_associative_array_read(FILE * stream) {
    amf_data * data = amf_associative_array_new();
    if (data != NULL) {
        amf_data * name;
        amf_data * element;
        if (fseek(stream, sizeof(uint32), SEEK_CUR) == 0) {
            /* we ignore the 32 bits array size marker */
            while(1) {
                name = amf_string_read(stream);
                if (name != NULL) {
                    element = amf_data_read(stream);
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
static amf_data * amf_array_read(FILE * stream) {
    amf_data * data = amf_array_new();
    if (data != NULL) {
        uint32 array_size;
        if (fread(&array_size, sizeof(uint32), 1, stream) == 1) {
            array_size = swap_uint32(array_size);

            size_t i;
            amf_data * element;
            for (i = 0; i < array_size; ++i) {
                element = amf_data_read(stream);

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
static amf_data * amf_date_read(FILE * stream) {
    number64_be milliseconds;
    sint16_be timezone;
    if (fread(&milliseconds, sizeof(number64_be), 1, stream) == 1 &&
        fread(&timezone, sizeof(sint16_be), 1, stream) == 1) {
        return amf_date_new(swap_number64(milliseconds), swap_sint16(timezone));
    }
    else {
        return NULL;
    }
}

/* load AMF data from stream */
amf_data * amf_data_read(FILE * stream) {
    byte type;
    if (fread(&type, sizeof(byte), 1, stream) == 1) {
        switch (type) {
            case AMF_TYPE_NUMBER:
                return amf_number_read(stream);
            case AMF_TYPE_BOOLEAN:
                return amf_boolean_read(stream);
            case AMF_TYPE_STRING:
                return amf_string_read(stream);
            case AMF_TYPE_OBJECT:
                return amf_object_read(stream);
            case AMF_TYPE_UNDEFINED:
                return amf_undefined_new();
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                return amf_associative_array_read(stream);
            case AMF_TYPE_ARRAY:
                return amf_array_read(stream);
            case AMF_TYPE_DATE:
                return amf_date_read(stream);
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

/* encode a number */
static size_t amf_number_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    number64_be val;
    if (maxbytes >= sizeof(number64_be)) {
        val = swap_number64(data->number_data);
        memcpy(buffer, &val, sizeof(number64_be));
        return sizeof(number64_be);
    }
    return 0;
}

/* encode a boolean */
static size_t amf_boolean_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    if (maxbytes >= sizeof(uint8)) {
        memcpy(buffer, &(data->boolean_data), sizeof(uint8));
        return sizeof(uint8);
    }
    return 0;
}

/* encode a string */
static size_t amf_string_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    uint16_be len;
    if (maxbytes >= sizeof(uint16)) {
        len = swap_uint16(data->string_data.size);
        memcpy(buffer, &len, sizeof(uint16_be));
        if (maxbytes-sizeof(uint16) >= data->string_data.size) {
            memcpy(buffer+sizeof(uint16), data->string_data.mbstr, data->string_data.size);
            return data->string_data.size;
        }
    }
    return 0;
}

/* encode an object */
static size_t amf_object_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    amf_node * node;
    size_t size = 0;
    size_t s;
    uint16_be filler = swap_uint16(0);

    node = amf_object_first(data);
    while (node != NULL) {
        s = amf_string_encode(amf_object_get_name(node), buffer+size, maxbytes-size);
        if (s == 0) {
            return 0;
        }
        size += s;
        s = amf_data_encode(amf_object_get_data(node), buffer+size, maxbytes-size);
        if (s == 0) {
            return 0;
        }
        size += s;
        node = amf_object_next(node);
    }
    
    if (maxbytes > size+sizeof(uint16_be)+sizeof(uint8)) {
        /* empty string is the last element */
        memcpy(buffer+size, &filler, sizeof(uint16_be));
        /* an object ends with 0x09 */
        buffer[size+sizeof(uint16_be)] = AMF_TYPE_TERMINATOR;
        size += sizeof(uint16_be) + sizeof(uint8);
    }
    else {
        size = 0;
    }

    return size;
}

/* encode an associative array */
static size_t amf_associative_array_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    amf_node * node;
    size_t size = 0;
    size_t s;
    uint32_be len;
    uint16_be filler = swap_uint16(0);

    len = swap_uint32(data->list_data.size) / 2;

    if (maxbytes >= sizeof(uint32_be)) {
        memcpy(buffer, &len, sizeof(uint32_be));
        size = sizeof(uint32_be);

        node = amf_object_first(data);
        while (node != NULL) {
            s = amf_string_encode(amf_object_get_name(node), buffer+size, maxbytes-size);
            if (s == 0) {
                return 0;
            }
            size += s;
            s = amf_data_encode(amf_object_get_data(node), buffer+size, maxbytes-size);
            if (s == 0) {
                return 0;
            }
            size += s;
            node = amf_object_next(node);
        }
        
        if (maxbytes > size+sizeof(uint16_be)+sizeof(uint8)) {
            /* empty string is the last element */
            memcpy(buffer+size, &filler, sizeof(uint16_be));
            /* an object ends with 0x09 */
            buffer[size+sizeof(uint16_be)] = AMF_TYPE_TERMINATOR;
            size += sizeof(uint16_be) + sizeof(uint8);
        }
        else {
            size = 0;
        }
    }
    return size;
}

/* encode an array */
static size_t amf_array_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    amf_node * node;
    size_t size = 0;
    size_t s;
    uint32_be len;

    len = swap_uint32(data->list_data.size);

    if (maxbytes >= sizeof(uint32_be)) {
        memcpy(buffer, &len, sizeof(uint32_be));
        size = sizeof(uint32_be);
        
        node = amf_array_first(data);
        while (node != NULL) {
            s = amf_data_encode(amf_array_get(node), buffer+size, maxbytes-size);
            if (s == 0) {
                return 0;
            }
            size += s;
            node = amf_array_next(node);
        }
    }
    return size;
}

/* encode a date */
static size_t amf_date_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    number64_be milliseconds = swap_number64(data->date_data.milliseconds);
    sint16_be timezone = swap_sint16(data->date_data.timezone);

    if (maxbytes >= sizeof(number64_be)+sizeof(sint16_be)) {
        memcpy(buffer, &milliseconds, sizeof(number64_be));
        memcpy(buffer+sizeof(number64_be), &timezone, sizeof(sint16_be));
        return sizeof(number64_be)+sizeof(sint16_be);
    }
    return 0;
}


/* AMF data encoding */
size_t amf_data_encode(amf_data * data, byte * buffer, size_t maxbytes) {
    size_t size = 0;
    if (maxbytes > 0 && data != NULL && buffer != NULL) {
        buffer[0] = data->type;
        ++size;
        switch (data->type) {
            case AMF_TYPE_NUMBER:
                size = amf_number_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_BOOLEAN:
                size = amf_boolean_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_STRING:
                size = amf_string_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_OBJECT:
                size = amf_object_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_UNDEFINED:
                break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                size = amf_associative_array_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_ARRAY:
                size = amf_array_encode(data, buffer+1, maxbytes-1);
            case AMF_TYPE_DATE:
                size = amf_date_encode(data, buffer+1, maxbytes-1);
            /*case AMF_TYPE_SIMPLEOBJECT:*/
            case AMF_TYPE_XML:
            case AMF_TYPE_CLASS:
            case AMF_TYPE_TERMINATOR:
                break; /* end of composite object */
            default:
                break;
        }
    }
    return size;
}


/* write a number */
static size_t amf_number_write(amf_data * data, FILE * stream) {
    number64 n = swap_number64(data->number_data);
    return fwrite(&n, sizeof(number64_be), 1, stream) * sizeof(number64_be);
}

/* write a boolean */
static size_t amf_boolean_write(amf_data * data, FILE * stream) {
    return fwrite(&(data->boolean_data), sizeof(uint8), 1, stream) * sizeof(uint8);
}

/* write a string */
static size_t amf_string_write(amf_data * data, FILE * stream) {
    uint16 s;
    size_t w = 0;
    
    s = swap_uint16(data->string_data.size);
    w = fwrite(&s, sizeof(uint16_be), 1, stream) * sizeof(uint16_be);
    if (data->string_data.size > 0) {
        w += fwrite(data->string_data.mbstr, sizeof(byte), (size_t)(data->string_data.size), stream) * sizeof(byte);
    }
    
    return w;
}

/* write an object */
static size_t amf_object_write(amf_data * data, FILE * stream) {
    amf_node * node;
    size_t w = 0;
    uint16_be filler = swap_uint16(0);
    uint8 terminator = AMF_TYPE_TERMINATOR;

    node = amf_object_first(data);
    while (node != NULL) {
        w += amf_string_write(amf_object_get_name(node), stream);
        w += amf_data_write(amf_object_get_data(node), stream);
        node = amf_object_next(node);
    }
    
    /* empty string is the last element */
    w += fwrite(&filler, sizeof(uint16_be), 1, stream) * sizeof(uint16_be);
    /* an object ends with 0x09 */
    w += fwrite(&terminator, sizeof(uint8), 1, stream) * sizeof(uint8);

    return w;
}

/* write an associative array */
static size_t amf_associative_array_write(amf_data * data, FILE * stream) {
    amf_node * node;
    size_t w = 0;
    uint32_be s;
    uint16_be filler = swap_uint16(0);
    uint8 terminator = AMF_TYPE_TERMINATOR;

    s = swap_uint32(data->list_data.size) / 2;
    w += fwrite(&s, sizeof(uint32_be), 1, stream) * sizeof(uint32_be);
    node = amf_associative_array_first(data);
    while (node != NULL) {
        w += amf_string_write(amf_associative_array_get_name(node), stream);
        w += amf_data_write(amf_associative_array_get_data(node), stream);
        node = amf_associative_array_next(node);
    }
    
    /* empty string is the last element */
    w += fwrite(&filler, sizeof(uint16_be), 1, stream) * sizeof(uint16_be);
    /* an object ends with 0x09 */
    w += fwrite(&terminator, sizeof(uint8), 1, stream) * sizeof(uint8);

    return w;
}

/* write an array */
static size_t amf_array_write(amf_data * data, FILE * stream) {
    amf_node * node;
    size_t w = 0;
    uint32_be s;

    s = swap_uint32(data->list_data.size);
    w += fwrite(&s, sizeof(uint32_be), 1, stream) * sizeof(uint32_be);
    node = amf_array_first(data);
    while (node != NULL) {
        w += amf_data_write(amf_array_get(node), stream);
        node = amf_array_next(node);
    }

    return w;
}

/* write a date */
static size_t amf_date_write(amf_data * data, FILE * stream) {
    size_t w = 0;
    number64_be milli;
    sint16_be tz;
    
    milli = swap_number64(data->date_data.milliseconds);
    w += fwrite(&milli, sizeof(number64_be), 1, stream) * sizeof(number64_be);
    tz = swap_sint16(data->date_data.timezone);
    w += fwrite(&tz, sizeof(sint16_be), 1, stream) * sizeof(sint16_be);

    return w;
}

/* write amf data to stream */
size_t amf_data_write(amf_data * data, FILE * stream) {
    size_t s = 0;
    if (data != NULL) {
        s += fwrite(&(data->type), sizeof(byte), 1, stream) * sizeof(byte);
        switch (data->type) {
            case AMF_TYPE_NUMBER:
                s += amf_number_write(data, stream);
                break;
            case AMF_TYPE_BOOLEAN:
                s += amf_boolean_write(data, stream);
                break;
            case AMF_TYPE_STRING:
                s += amf_string_write(data, stream);
                break;
            case AMF_TYPE_OBJECT:
                s += amf_object_write(data, stream);
                break;
            case AMF_TYPE_UNDEFINED:
                break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY:
                s += amf_associative_array_write(data, stream);
                break;
            case AMF_TYPE_ARRAY:
                s += amf_array_write(data, stream);
                break;
            case AMF_TYPE_DATE:
                s += amf_date_write(data, stream);
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

/* data freeing functions */
static void amf_string_free(amf_data * data) {
    if (data->string_data.mbstr != NULL) {
        free(data->string_data.mbstr);
    }
}

static void amf_object_free(amf_data * data) {
    amf_list_clear(&data->list_data);
}

static void amf_associative_array_free(amf_data * data) {
    amf_list_clear(&data->list_data);
}

static void amf_array_free(amf_data * data) {
    amf_list_clear(&data->list_data);
}

void amf_data_free(amf_data * data) {
    if (data != NULL) {
        switch (data->type) {
            case AMF_TYPE_NUMBER: break;
            case AMF_TYPE_BOOLEAN: break;
            case AMF_TYPE_STRING: amf_string_free(data); break;
            case AMF_TYPE_OBJECT: amf_object_free(data); break;
            case AMF_TYPE_UNDEFINED: break;
            /*case AMF_TYPE_REFERENCE:*/
            case AMF_TYPE_ASSOCIATIVE_ARRAY: amf_associative_array_free(data); break;
            case AMF_TYPE_ARRAY: amf_array_free(data); break;
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
