/*
    $Id: win32_tmpfile.c 231 2011-06-27 13:46:19Z marc.noirot $

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
#ifdef WIN32

# define WIN32_LEAN_AND_MEAN
# include <windows.h>
# include <io.h>

# include "win32_tmpfile.h"

FILE * win32_tmpfile(void) {
    DWORD path_len;
    TCHAR path_name[MAX_PATH + 1];
    TCHAR file_name[MAX_PATH + 1];
    HANDLE handle;
    int fd;
    FILE *fp;

    path_len = GetTempPath(MAX_PATH, path_name);
    if (path_len <= 0 || path_len >= MAX_PATH) {
	    return NULL;
    }

    if (GetTempFileName(path_name, TEXT("flv"), 0, file_name) == 0) {
	    return NULL;
    }

    handle = CreateFile(file_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE,
        NULL
    );
    if (handle == INVALID_HANDLE_VALUE) {
	    return NULL;
    }

    fd = _open_osfhandle((intptr_t)handle, 0);
    if (fd == -1) {
	    CloseHandle(handle);
	    return NULL;
    }

    fp = _fdopen(fd, "w+b");
    if (fp == NULL) {
	    _close(fd);
	    return NULL;
    }

    return fp;
}

#endif /* WIN32 */
