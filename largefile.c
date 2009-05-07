/*
    $Id$

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
#include "largefile.h"

/*
    These functions assume fpos_t is a 64-bit signed integer
*/

file_offset_t ftello(FILE * stream) {
    fpos_t p;
    if (fgetpos(stream, &p) == 0) {
        return (file_offset_t)p;
    }
    else {
        return -1LL;
    }
}

int fseeko(FILE * stream, file_offset_t offset, int whence) {
    fpos_t p;
    if (fgetpos(stream, &p) == 0) {
        switch (whence) {
            case SEEK_CUR: p += offset; break;
            case SEEK_SET: p = offset; break;
            /*case SEEK_END:; not implemented here */
            default:
                return -1;
        }
        fsetpos(stream, &p);
        return 0;
    }
    else {
        return -1;
    }
}
