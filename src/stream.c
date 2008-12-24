/*
    $Id: $

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
#include "stream.h"
#include "types.h"
#include <stdio.h>
#include <malloc.h>

/* callback function to read data from a file stream */
static size_t file_read(void * out_buffer, size_t size, void * user_data) {
    return fread(out_buffer, sizeof(byte), size, (FILE *)user_data);
}

/* callback function to seek in a file stream */
static size_t file_seek(long offset, int whence, void * user_data) {
    return fseek((FILE *)user_data, offset, whence);
}

/* callback function to close a file stream */
static int file_close(void * user_data) {
    return fclose((FILE *)user_data);
}

/* open a binary read-only file stream */
stream * stream_file_open(const char * file) {
    stream * s;
    FILE * f;
    f = fopen(file, "rb");
    if (f == NULL) {
        return NULL;
    }
    s = malloc(sizeof(stream));
    if (s == NULL) {
        fclose(f);
        return NULL;
    }
    s->user_data = f;
    s->read_proc = file_read;
    s->seek_proc = file_seek;
    s->close_proc = file_close;
    return s;
}

/* structure used to mimic a stream with a memory buffer */
typedef struct __buffer_stream {
    byte * start_address;
    byte * current_address;
    size_t size;
} buffer_stream;

/* callback function to mimic fread using a memory buffer */
static size_t buffer_read(void * out_buffer, size_t size, void * user_data) {
    buffer_stream * stream = (buffer_stream *)user_data;
    if (stream->current_address + size > stream->start_address + stream->size) {
        size = stream->size - (stream->current_address - stream->start_address);
    }  
    memcpy(out_buffer, stream->current_address, size);
    stream->current_address += size;
    return size;
}

/* callback function to mimic fseek using a memory buffer */
static size_t buffer_seek(long offset, int whence, void * user_data) {
    buffer_stream * stream = (buffer_stream *)user_data;
    byte * new_address = NULL;
    switch (whence) {
        case SEEK_SET:
            new_address = stream->start_address + offset;
            break;
        case SEEK_CUR:
            new_address = stream->current_address + offset;
            break;
        case SEEK_END:
            new_address = stream->start_address + stream->size + 1 + offset;
            break;
    }
    if (new_address >= stream->start_address &&
        new_address <= stream->start_address + stream->size) {

        stream->current_address = new_address;
        return 0;
    }
    else {
        return -1;
    }
}

/* callback function to close a memory stream */
static int buffer_close(void * user_data) {
    free(user_data);
    return 0;
}

stream * stream_mem_open(void * start_addr, size_t size) {
    stream * s;
    buffer_stream * bs;
    bs = malloc(sizeof(buffer_stream));
    if (bs == NULL) {
        return NULL;
    }
    s = malloc(sizeof(stream));
    if (s == NULL) {
        free(bs);
        return NULL;
    }
    bs->start_address = bs->current_address = start_addr;
    bs->size = size;
    s->user_data = bs;
    s->read_proc = buffer_read;
    s->seek_proc = buffer_seek;
    s->close_proc = buffer_close;
    return s;
}

size_t stream_read(stream * s, void * out_buffer, size_t size) {
    return s->read_proc(out_buffer, size, s->user_data);
}

size_t stream_seek(stream * s, long offset, int whence) {
    return s->seek_proc(offset, whence, s->user_data);
}

int stream_close(stream * s) {
    int status = s->close_proc(s->user_data);
    free(s);
    return status;
}
