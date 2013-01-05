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
#ifndef __DUMP_H__
#define __DUMP_H__

#include "flvmeta.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* common FLV strings */
const char * dump_string_get_tag_type(flv_tag * tag);
const char * dump_string_get_video_codec(flv_video_tag tag);
const char * dump_string_get_video_frame_type(flv_video_tag tag);
const char * dump_string_get_sound_type(flv_audio_tag tag);
const char * dump_string_get_sound_size(flv_audio_tag tag);
const char * dump_string_get_sound_rate(flv_audio_tag tag);
const char * dump_string_get_sound_format(flv_audio_tag tag);

/* dump metadata from a FLV file */
int dump_metadata(const flvmeta_opts * options);

/* dump the full contents of an FLV file */
int dump_flv_file(const flvmeta_opts * options);

/* dump AMF data directly */
int dump_amf_data(const amf_data * data, const flvmeta_opts * options);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __DUMP_H__ */
