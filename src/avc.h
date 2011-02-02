/*
    $Id: avc.h 214 2011-02-02 16:51:31Z marc.noirot $

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
#ifndef __AVC_H__
#define __AVC_H__

#include <stdio.h>

#include "types.h"
#include "flv.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

int read_avc_resolution(flv_stream * f, uint32 body_length, uint32 * width, uint32 * height);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __AVC_H__ */
