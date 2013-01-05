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
#ifndef __WIN32_TMPFILE_H__
#define __WIN32_TMPFILE_H__

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/*
    Returns a descriptor to a temporary file.
    This function is meant as a Windows replacement
    to the broken standard tmpfile() function.
*/
FILE * win32_tmpfile(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __WIN32_TMPFILE_H__ */
