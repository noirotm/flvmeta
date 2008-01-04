/*
    $Id: amf.c 5 2007-09-21 14:51:58Z marc.noirot $

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
#include "mmap.h"

#ifdef WIN32 /* WIN32 mmap() compatibility layer */

#include <windows.h>
#include <io.h>
#include <errno.h>

void *mmap(void *start, size_t len, int prot, int flags, int fd, off_t offset) {
    /*
      This is a minimal implementation of the *NIX mmap syscall for WIN32.
      It supports the following protections flags :
      PROT_NONE, PROT_READ, PROT_WRITE, PROT_EXEC
      It handles the following mapping flags:
      MAP_FIXED, MAP_SHARED, MAP_PRIVATE (POSIX) and MAP_ANONYMOUS (SVID)
    */

    void *ret = MAP_FAILED;
    HANDLE hmap = NULL;
    long wprot = 0, wflags = 0;
    HANDLE hfile = (HANDLE)_get_osfhandle(fd);

    if (((intptr_t)hfile == -1) && (~flags & MAP_ANONYMOUS)) {
        /* non-file-backed mapping is only allowed with MAP_ANONYMOUS */
        errno = EBADF;
        return MAP_FAILED;
    }

    /* map *NIX protections and flags to their WIN32 equivalents */
    if ((prot & PROT_READ) && (~prot & PROT_WRITE)) {
        /* read only, maybe exec */
        wprot = (prot & PROT_EXEC) ? (PAGE_EXECUTE_READ) : (PAGE_READONLY);
        wflags = (prot & PROT_EXEC) ? (FILE_MAP_EXECUTE) : (FILE_MAP_READ);
    }
    else if (prot & PROT_WRITE) {
        /* read/write, maybe exec */
        if ((flags & MAP_SHARED) && (~flags & MAP_PRIVATE)) {
            /* changes are committed to the file */
            wprot = (prot & PROT_EXEC) ? (PAGE_EXECUTE_READWRITE) : (PAGE_READWRITE);
            wflags = (prot & PROT_EXEC) ? (FILE_MAP_EXECUTE) : (FILE_MAP_WRITE);
        }
        else if ((flags & MAP_PRIVATE) && (~flags & MAP_SHARED)) {
            /* does not affect the original file */
            wprot = PAGE_WRITECOPY;
            wflags = FILE_MAP_COPY;
        }
        else {
            /* MAP_PRIVATE + MAP_SHARED is not allowed, abort */
            errno = EINVAL;
            return MAP_FAILED;
        }
    }

    /* create the windows map object */
    hmap = CreateFileMapping(hfile, NULL, wprot, 0, (DWORD)len, NULL);

    if (hmap == NULL) {
        /* the fd was checked before, so it must have bad access rights */
        errno = EPERM;
        return MAP_FAILED;
    }

    /* create the map itself */
    ret = MapViewOfFileEx(hmap, wflags, 0, offset, len,
        (flags & MAP_FIXED) ? (start) : (NULL));

    /* Drop the map, it will not be deleted until last 'view' is closed */
    CloseHandle(hmap);

    if (ret == NULL) {
        /* if MAP_FIXED was set, the address was probably wrong */
        errno = (flags & MAP_FIXED) ? (EINVAL) : (ENOMEM);
        return MAP_FAILED;
    }

    return ret;
}

int munmap(void *start, size_t len) {
    /*
      This is a minimal implementation of the *NIX munmap syscall for WIN32.
      The size parameter is ignored under Win32.
    */
    if (start == NULL) {
        errno = EINVAL;
        return -1;
    }
    
    return UnmapViewOfFile(start) ? 0 : -1;
}

#endif /* WIN32 */
