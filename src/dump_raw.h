/*
    $Id: dump_raw.h 170 2010-02-12 16:05:27Z marc.noirot $

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
#ifndef __DUMP_RAW_H__
#define __DUMP_RAW_H__

#include "flvmeta.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* raw dumping functions */
void dump_raw_setup_metadata_dump(flv_parser * parser);
void dump_raw_setup_file_dump(flv_parser * parser);
int dump_raw_amf_data(const amf_data * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DUMP_RAW_H__ */
