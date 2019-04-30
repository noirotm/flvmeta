/*
    FLVMeta - FLV Metadata Editor

    Copyright (C) 2007-2019 Marc Noirot <marc.noirot AT gmail.com>

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
#ifndef __DUMP_YAML_H__
#define __DUMP_YAML_H__

#include "flvmeta.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* YAML dumping functions */
void dump_yaml_setup_metadata_dump(flv_parser * parser);
int dump_yaml_file(flv_parser * parser, const flvmeta_opts * options);
int dump_yaml_amf_data(const amf_data * data);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DUMP_YAML_H__ */
