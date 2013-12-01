/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2013 Marc Noirot <marc.noirot AT gmail.com>

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
#else /* !WIN32 */
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif /* WIN32 */

#include "util.h"

int flvmeta_same_file(const char * file1, const char * file2) {
#ifdef WIN32
    /* in Windows, we have to open the files and use GetFileInformationByHandle */
    HANDLE h1, h2;
    BY_HANDLE_FILE_INFORMATION info1, info2;

    h1 = CreateFile(file1, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h1 == INVALID_HANDLE_VALUE) {
        return 0;
    }
    GetFileInformationByHandle(h1, &info1);
    CloseHandle(h1);

    h2 = CreateFile(file2, 0, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h2 == INVALID_HANDLE_VALUE) {
        return 0;
    }
    GetFileInformationByHandle(h2, &info2);
    CloseHandle(h2);

    return (info1.dwVolumeSerialNumber == info2.dwVolumeSerialNumber
        && info1.nFileIndexHigh == info2.nFileIndexHigh
        && info1.nFileIndexLow == info2.nFileIndexLow);
#else /* !WIN32 */
    /* if not in Windows, we must stat each file and compare device and inode numbers */
    struct stat s1, s2;
    if (stat(file1, &s1) != 0) {
        return 0;
    }
    if (stat(file2, &s2) != 0) {
        return 0;
    }
    return (s1.st_dev == s2.st_dev && s1.st_ino == s2.st_ino);

#endif /* WIN32 */
}

#ifdef WIN32
FILE * flvmeta_tmpfile(void) {
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

int flvmeta_filesize(const char *filename, file_offset_t *filesize) {
#ifdef WIN32
    BOOL result;
    WIN32_FILE_ATTRIBUTE_DATA fad;

    result = GetFileAttributesEx(filename, GetFileExInfoStandard, &fad);
    if (result) {
        *filesize = ((file_offset_t)fad.nFileSizeHigh << 32) + fad.nFileSizeLow;
        return 1;
    }
    else {
        return 0;
    }
#else /* !WIN32 */
    struct stat fs;
    int result;

    result = stat(filename, &fs);
    if (result == 0) {
        *filesize = fs.st_size;
        return 1;
    }
    else {
        return 0;
    }
#endif /* WIN32 */
}
