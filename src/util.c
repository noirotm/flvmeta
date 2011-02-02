/*
    $Id: util.c 144 2009-12-10 10:55:31Z marc.noirot $

    FLV Metadata updater

    Copyright (C) 2007-2011 Marc Noirot <marc.noirot AT gmail.com>

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
#ifdef WIN32
# define WIN32_LEAN_AND_MEAN
# include <windows.h>
#else /* !WIN32 */
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
#endif /* WIN32 */

#include "util.h"

int same_file(const char * file1, const char * file2) {
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
