/*
    $Id: $

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
#ifndef __STREAM_H__
#define __STREAM_H__

#include <stdlib.h>

/* Generic stream structure, implemented with callbacks */
typedef size_t (*stream_read_proc) (void * out_buffer, size_t size, void * user_data);
/*typedef size_t (*stream_write_proc)(const void * in_buffer, size_t size, void * user_data);*/
typedef size_t (*stream_seek_proc) (long offset, int whence, void * user_data);
typedef int    (*stream_close_proc)(void * user_data);

typedef struct __stream_t {
    void * user_data;
    stream_read_proc read_proc;
    /*stream_write_proc write_proc;*/
    stream_seek_proc seek_proc;
    stream_close_proc close_proc;
} stream;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

stream * stream_file_open(const char * file);
stream * stream_mem_open(void * start_addr, size_t size);

size_t stream_read(stream * s, void * out_buffer, size_t size);
/*size_t stream_write(stream * s, const void * in_buffer, size_t size);*/
size_t stream_seek(stream * s, long offset, int whence);
int    stream_close(stream * s);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STREAM_H__ */
